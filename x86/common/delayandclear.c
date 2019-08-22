/*
 * $Id: delayandclear.c,v 1.1.1.1 2003/06/10 22:41:42 iceblink Exp $
 *
 * wait some time and clear memory while doing it
 */

#include <common/systype.h>
#include <common/delay.h>
#include <common/pci.h>
#include <common/lcd.h>

#include <io.h>
#include <stdlib.h>
#include <printf.h>

extern unsigned int mem_size;
static unsigned int top_cleared;

static void mem_setup(void)
{
	unsigned char val;

	top_cleared = 32;

	/* mask the NMI (you figure it out)
	 * because we may get errors */
	outb(inb(0x70) | 0x80, 0x70);
		
	/* turn off parity NMIs */
	outb(inb(0x61) | 0xc, 0x61);

	/* disable ECC */
	val = pci_readb(0, PCI_DEVFN(0,0), 0xe0);
	pci_writeb(0, PCI_DEVFN(0,0), 0xe0, val & ~0x4);
}

void udelay_and_clear( int usecs )
{
    unsigned long long start_ticks = delay_get_ticks();

    switch( systype() )
    {
	case SYSTYPE_3K:
	    udelay( usecs );
	    break;

	case SYSTYPE_5K:
	    if( ! top_cleared )
		mem_setup();
	    
	    while( ((int)delay_ticks2usec( delay_get_ticks() - start_ticks ) < usecs) &&
		   (top_cleared < (mem_size >> 20)) )
	    {
		memset((void *)(top_cleared++*1024*1024), 0, 1024 * 1024);
	    }

		/* wait if we are done with memory */
	    if( top_cleared >= (mem_size >> 20) )
	    {
		start_ticks = delay_get_ticks() - start_ticks;
		if( (int)delay_ticks2usec(start_ticks) < usecs )
		    udelay( usecs - (int)delay_ticks2usec(start_ticks) );
	    }

	    break;
    }
}

void delay_clear_rest( int pbar )
{
    unsigned int old_top = top_cleared;
    
    switch( systype() )
    {
	case SYSTYPE_3K:
	    break;
	    
	case SYSTYPE_5K:
	    if (!top_cleared)
		mem_setup();

	    while( top_cleared < (mem_size >> 20) ) {
		memset((void *)(top_cleared++*1024*1024), 0, 1024 * 1024);
		if (pbar && (top_cleared % 32 == 0) ) {
		    lcd_write_progress_bar( ((top_cleared-old_top) * 100) /
					    ((mem_size >> 20) - old_top) );
		}
	    }
	    
	    {
		unsigned char val;
		

		    /* clear any ECC errors we've incurred */
		val = pci_readb(0, PCI_DEVFN(0,0), 0x47);
		pci_writeb(0, PCI_DEVFN(0,0), 0x47, val);
		
		    /* turn on NMI generation from PERR/SERR */
		outb(inb(0xc14) & ~0x0c, 0xc14);
		
		    /* clear parity NMI bits, and enable parity NMIs */
		outb(inb(0x61) | 0xc, 0x61);
		outb(inb(0x61) & ~0xc, 0x61);
		
		    /* re-enable ECC */
		val = pci_readb(0, PCI_DEVFN(0,0), 0xe0);
		pci_writeb(0, PCI_DEVFN(0,0), 0xe0, val | 0x4);
	    
		/* unmask NMI - leave this for the kernel to re-enable*/
		/*outb(inb(0x70) & ~0x80, 0x70);*/
	    }
	    break;
    }
}

unsigned int delay_clear_amount( void )
{
    switch( systype() )
    {
	case SYSTYPE_3K:
	    top_cleared = mem_size >> 20;
	    break;
	    
	case SYSTYPE_5K:
	    if (!top_cleared)
		mem_setup();
    }

    return mem_size - (top_cleared << 20);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
