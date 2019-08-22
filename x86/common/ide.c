/* $Id: ide.c,v 1.1.1.1 2003/06/10 22:41:47 iceblink Exp $
 * Copyright 1999-2000 Cobalt Networks, Inc.
 * Copyright 2001 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#include <io.h>
#include <printf.h>
#include <string.h>

#include "common/rom.h"
#include "common/ide.h"
#include "common/delay.h"
#include "common/cmos.h"
#include "common/pci.h"
#include "common/pci_regs.h"
#include "common/systype.h"
#include "common/i2c.h"
#include "common/serial.h"
#include "common/lcd.h"

#ifndef DONT_USE_SWITCHES
#define DONT_USE_SWITCHES 0
#endif

char ide_drives_present = 0;
static int ide_check_drive(unsigned int base, int drive, 
			   const unsigned int timeout);
static int ide_wait_for_idle(unsigned int base, int timeout, int drq);
static int ide_init_ruler(void);
static void ide_scan(void);
static void ide_idle_drive( int base, int slave );

static int ide0_base0 = IDE0_BASE0;
static int ide0_base1 = IDE0_BASE1;
static int ide1_base0 = IDE1_BASE0;
static int ide1_base1 = IDE1_BASE1;

int init_ide(void)
{    

    pci_dev *ide_dev;
    unsigned int dma_base;

    printf("Initializing IDE: ");

    if( (ide_dev = pci_lookup(PCI_VENDOR_AL, PCI_DEVICE_AL_M5229)) )
    {
	printf("found ALI M5229 at %02x:%02x\n",
		   ide_dev->bus->busnum, ide_dev->devfn);
    }
    else if( (ide_dev = pci_lookup(PCI_VENDOR_SERVERWORKS,
				  PCI_DEVICE_SERVERWORKS_OSB4IDE)) )
    {
	    printf("found ServerWorks OSB4 at %02x:%02x\n",
		   ide_dev->bus->busnum, ide_dev->devfn);
    }
    else if( (ide_dev = pci_lookup(PCI_VENDOR_SERVERWORKS,
				  PCI_DEVICE_SERVERWORKS_CSB5IDE)) )
    {
	printf("found ServerWorks CSB5 at %02x:%02x\n",
	       ide_dev->bus->busnum, ide_dev->devfn);
    }

    if( !ide_dev )
    {
	printf("No controller installed.\n");
	return -1;
    }

	/* read base addrs */
    ide0_base0 = pci_dev_readl(ide_dev, PCI_BAR_ADDR(0)) & ~1;
    ide0_base1 = pci_dev_readl(ide_dev, PCI_BAR_ADDR(1)) & ~1;
    ide1_base0 = pci_dev_readl(ide_dev, PCI_BAR_ADDR(2)) & ~1;
    ide1_base1 = pci_dev_readl(ide_dev, PCI_BAR_ADDR(3)) & ~1;

	/* read DMA base address */
    dma_base = pci_dev_readl(ide_dev, PCI_BAR_ADDR(4)) & ~1;

    /* some systems have staggered startup for power reduction @ boot */
    if (boardtype() == BOARDTYPE_CARMEL || boardtype() == BOARDTYPE_ALPINE) {
	int i;

        /* make sure ~5.5 seconds has passed since power on */
        printf("  spinning up second channel: ");
        while (delay_ticks2usec(delay_get_ticks()) < 5500000);
            
	if (boardtype() == BOARDTYPE_CARMEL) {
            pci_dev *pmu_dev = pci_lookup(0x10b9, 0x7101);
            /* 
	     * init the second ide channel by raising the high bit on the flash
             * page register.
             * NOTE: this is the same line used for the web LED on the RaQs
             */
            pci_dev_write_config(pmu_dev, 0xb8,
                pci_dev_read_config(pmu_dev, 0xb8, 2) | 0x0080 , 2);
        } else if (boardtype() == BOARDTYPE_ALPINE) {
	    /*
	     * spinup the second IDE channel by GPO60
	     */
            outb(inb(0x610) | 1, 0x610);
	}
	for (i = 0; i < 5; i++) {
		printf(">");
		delay(1000);
	}
	printf("\b\b\b\b\bdone \n");
    }

	/* 
	 * set all drives as DMA capable
	 * clear interrupt and/or error conditions
	 */
    outb(inb(dma_base+0x2) | 0x66, dma_base+0x2);
    outb(inb(dma_base+0xa) | 0x66, dma_base+0xa);
    
    if( boardtype() == BOARDTYPE_MONTEREY )
    {
	    /* initialize highpoint controllers */	    
	if( ide_init_ruler() < 0 )
	    ide_scan(); /* if that fails look at the south bridge */
    }
    else
    {
	ide_scan();
    }

    return 0;
}

void
list_ide(void)
{
	if (ide_drives_present & 0x1) {
		if (ide_drives_present & 0x2)
			printf(", slave");
		printf("\n");
	}
	if (ide_drives_present & 0x4) {
		printf("     found: secondary master");
		if (ide_drives_present & 0x8)
			printf(", slave");
		printf("\n");
	}
}

static int
ide_check_drive (unsigned int base, int drive, const unsigned int timeout)
{
	unsigned int val;
	
	    /* look for drives */
/*	printf("looking for drives...\n"); */

	/* set master */
	outb(IDE_CMD_MASTER | ((drive&1)<<4), base|IDE_DRIVE_HEAD_OFFSET);

	/* send identity command */
	outb(IDE_CMD_IDENTIFY, base|IDE_COMMAND_REG_OFFSET);
    
	if (ide_wait_for_idle(base, timeout*100, 1))
		return 0;

	val = inw(base|IDE_DATA_OFFSET);
	if (val & 0x8000) /* this bit is defined as 0 */
		return 0;

	val = inw(base|IDE_DATA_OFFSET);
	if (val==0) /* no cylinders */
		return 0; 

	return 1;
}

static int
ide_wait_for_idle (unsigned int base, int timeout, int drq)
{
	int err;
	drq = drq ? 8 : 0;

	/* wait for idle */
	while ( ((err = inb(base|IDE_STATUS_REG_OFFSET)) & 0x88) != drq) {
		if (timeout--)
			delay(1);
		else
			return -1;
	}

	return (err&1) ? -1 : 0;
}

static void ide_scan(void)
{
    int i;

    lcd_write_progress_bar( 0 );
    printf("  scanning ide0: ");

	/* check ide0 master */
    if (ide_check_drive(ide0_base0, 0, 50)) {
	ide_drives_present |= 0x1;
	printf("master ");
    }
    lcd_write_progress_bar( (5*100)/45 );

    /* check ide0 slave */
    if (ide_check_drive(ide0_base0, 1, 2)) {
	ide_drives_present |= 0x2;
	printf("slave");
    }
    lcd_write_progress_bar( (10*100)/45 );
    
	/* check ide1 master */
    printf("\n  scanning ide1: ");

    if (ide_check_drive(ide1_base0, 0, 2)) {
	ide_drives_present |= 0x4;
	printf("master ");
    }	
    lcd_write_progress_bar( (15*100)/45 );

	    /* check ide1 slave */
    if (ide_check_drive(ide1_base0, 1, 2)) {
	ide_drives_present |= 0x8;
	printf("slave");
    }
    lcd_write_progress_bar( (20*100)/45 );

    if (ide_drives_present) {
	    printf("\nIDE: stabilizing spinup:    ");
	    for (i = 0; i < 100; i++) {
		    unsigned char val;
		    if (!serial_inb_timeout((char *)&val, 3))
			    break;
		    printf("\b\b\b%2d%%", i);
		    delay_and_clear(100); /* .1 sec */
		    if( !(i%4) )
			    lcd_write_progress_bar( ((20+(i/4)) * 100)/ 45 );
	    }
	    printf("\b\b\b100%%\n");

	    for( i=0 ; i<4 ; i++ ) {
		    if( ide_drives_present & (0x1 << i) ) {
			    ide_idle_drive((i<2)?ide0_base0:ide1_base1, i%2 );
		    }
	    }

    } else 
	    lcd_write_progress_bar(100);
}

static void ide_idle_drive( int base, int slave )
{
	/* select slave or master */
    outb(IDE_CMD_MASTER | ((slave&1)<<4), base|IDE_DRIVE_HEAD_OFFSET);

	/* shut the busy light off */
    outb(WIN_IDLEIMMEDIATE, base|IDE_COMMAND_REG_OFFSET );
}



/*
 * turn on each drive channel in series
 * wait ~5 seconds between each powerup
 */
static int
ide_init_ruler(void)
{
	pci_dev *hptdev = NULL;
	byte switches, portmask = 0;	/* bitmap of powered on ports for I2C control */
	int port = 0, have_sleds = 0;
	int i;
	int powerup_secs = 25;
	int waited=0;
	int wait_canceled = 0;
	
	/* read in drive switches. even if we don't use the value,
	 * we need to read it to set up the interrupt state correctly. 
	 *
	 * NOTE: this check will fail if someone sticks in an HPT370
	 * in the other slot. */
	switches = i2c_read_byte(I2C_DEV_DRIVE_SWITCH, 0);
	if ((switches & 0xf0) == 0xf0) /* no ide ruler detected */
		return -1;

	switches &= 0xf;
	DPRINTF("IDE: ruler detected (settings=0x%x)\n", switches);

#if DONT_USE_SWITCHES
	switches = 0;
#endif
	if( !(switches & 0x1) )
	    powerup_secs += 5;
	if( !(switches & 0x2) )
	    powerup_secs += 5;
	if( !(switches & 0x4) )
	    powerup_secs += 5;
	if( !(switches & 0x8) )
	    powerup_secs += 5;

	while ((hptdev = pci_lookup_dev(PCI_VENDOR_HIGHPOINT,
					PCI_DEVICE_HIGHPOINT_HPT370,
					hptdev)) != NULL)
	{
		byte trireg, resetmask;
		byte reg, j;

		printf("IDE: found HighPoint HPT370 at %02x:%02x\n",
		       hptdev->bus->busnum, PCI_SLOT(hptdev->devfn));

		/* strobe bit 0 of register 0x5B to latch CBLID state */
		pci_dev_writeb(hptdev, 0x5b, 0x21);
		pci_dev_writeb(hptdev, 0x5b, 0x20);

		/* read P_CBLID and S_CBLID from register 0x5A of highpoint */
		reg = pci_dev_readb(hptdev, 0x5a);

		for (j=2, trireg = 0x52, resetmask = 0x40; j>0; 
		     j--, trireg = 0x56, resetmask = 0x80) {
			port++;

			/* mask with bitmap of ports that have been
                           powered already */
			portmask |= I2C_DEV_RULER_PORT(port);

			/* set the reset line. tristate the bus. */
			if (switches & (1 << (port - 1))) {
				printf("     Port %d: sled not found\n", port);
				i2c_write_byte(I2C_DEV_RULER, 0, portmask);
				i = pci_dev_readb(hptdev, 0x59) & 0xff;
				pci_dev_writeb(hptdev, 0x59, i | resetmask);
				i = pci_dev_readw(hptdev, trireg) & 0xffff;
				pci_dev_writew(hptdev, trireg, i | 0x8000);
				continue;
			}

			printf("     Port %d ATA-%s Cable - powering up:  ",
			       port, (reg & j) ? "33" : "66");

			/* write bitmap to i2c bus to powerup drives */
			have_sleds = 1;
			i2c_write_byte(I2C_DEV_RULER, 0, portmask);

			/* wait a few seconds for the drive to spin up */
			for (i=0; i<5; i++) {
			    unsigned char val;
			    if( wait_canceled )
				break;
			    delay_and_clear(1000); /* 1 sec */
			    lcd_write_progress_bar( (++waited * 100)/
						    powerup_secs );
			    printf( ">" );
			    if( !serial_inb_timeout((char *)&val, 3 ))
			    {
				wait_canceled = 1;
				break;
			    }
			}
			printf("\b\b\b\b\b done\n");
		}
	}

	if (have_sleds) {
		/* require at least 30 seconds (25 + 5 above) for drives 
		   to spin up */
		printf("IDE: stabilizing spinup:    ");
		for (i = 0; i < 100; i++) {
			unsigned char val;
			if( wait_canceled )
			    break;
			printf("\b\b\b%2d%%", i);
			delay_and_clear(250); /* 1/2 sec */
			if( !(i%4) )
			    lcd_write_progress_bar( (++waited * 100)/
						    powerup_secs );
			if (!serial_inb_timeout((char *)&val, 3))
				break;
		}
		printf("\b\b\b100%%\n");
	}
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
