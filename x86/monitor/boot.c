/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: boot.c,v 1.3 2004/01/23 01:40:50 thockin Exp $
 */

#include <string.h>
#include <io.h>

#include "common/rom.h"
#include "common/boot.h"
#include "common/x86-boot.h"
#include "common/cache.h"
#include "common/pci.h"
#include "common/cmos.h"
#include "common/delay.h"
#include "common/lcd.h"
#include "common/systype.h"
#include "common/isapnp.h"
#include "common/acpi.h"
#include "common/eth.h"
#include "common/alloc.h"

#include "boot.h"

#define MAX_PARAM_LEN 1024

int init_delay_func(int argc, char *argv[] __attribute ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	(void)delay_init();

	printf("CPU: Approx. %dMHz\n", (unsigned int)ticks_per_us);

	return 0;
}

/* generic boot function */
int bfX_func(int argc, char *argv[])
{
	int i;

	if (argc > 3)
		return EBADNUMARGS;
    
	/* get kernel source */
	if (argc > 1) {
		if (*argv[1] == 'd')
			i = CMOS_BOOT_KERN_DEV;
		else if (*argv[1] == 'r')
			i = CMOS_BOOT_KERN_ROM;
		else if (*argv[1] == 'n')
			i = CMOS_BOOT_KERN_NET;
		else
			i = 0;
		if (i)
			set_boot_opt(CMOS_BOOT_KERN_MASK, i);
	}

	/* get root fs */
	if (argc > 2) {
		if (*argv[2] == 'd')
			i = CMOS_BOOT_FS_DEV;
		else if (*argv[2] == 'n')
			i = CMOS_BOOT_FS_NET;
		else
			i = 0;
		if (i)
			set_boot_opt(CMOS_BOOT_FS_MASK, i);
	} 

	return do_bfX(BOOT_OPT_OTHER);
}

int bfr_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
    
	return do_bfX(BOOT_OPT_BFR);
}

int boot_netbsd_func(int argc, char *argv[] __attribute__ ((unused)))
{
	switch (argc) {
	case 1:
		set_kernel_image(BOOT_KERNEL_NETBSD);
		break;
	case 2:
		set_kernel_image(argv[1]);
		break;
	default:
		return EBADNUMARGS;
	}

	return do_bfd_netbsd(0);
}

int bfd_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc == 2)
		set_kernel_image(argv[1]);

	return do_bfX(BOOT_OPT_BFD);
}

int bfn_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	return do_bfX(BOOT_OPT_BFN);
}

int bfnn_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	return do_bfX(BOOT_OPT_BFNN);
}

int load_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
   
	dl_kernel();
	bunzip2_kernel();
	boot_kernel_linux();
    
	return 0;
}

int dl_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
   
	dl_kernel();

	return 0;
}

int bunzip2_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
   
	bunzip2_kernel();

	return 0;
}

int net_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
   
	net_boot(BOOT_STAGE_RUN);
	return 0;
}

int boot_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
   
	boot_kernel_linux();

	return 0;
}

#ifdef MD5
int checksum_kernel_func(int argc, char *argv[] __attribute__ ((unused)))
{
	unsigned int i;
	unsigned char *load_addr = (unsigned char *)kCompAddr;
	unsigned char md5buffer[16];

	if (argc != 1)
		return EBADNUMARGS;

	md5_mem(load_addr, vmlinux_gz_size, md5buffer);

	for (i=0; i<16; i++)
		printf ("%2x", md5buffer[i]);

	printf("\n");

	return 0;
}
#endif

int set_params_func(int argc, char *argv[])
{
	if (argc != 2)
		return EBADNUMARGS;

	if (boot_params) {
		free(boot_params);
		boot_params = NULL;
	}
	boot_params = strdup(argv[1]);
	return 0;
}

int set_load_params_func(int argc, char *argv[])
{
	if (argc != 2)
		return EBADNUMARGS;

	if (load_params) {
		free(load_params);
		load_params = NULL;
	}
	load_params = strdup(argv[1]);
	return 0;
}

int reboot_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	lcd_clear();
	lcd_write_str_at(0, "Rebooting...");

	switch (systype()) {
	case SYSTYPE_3K: {
		/*
		 * kick the watchdog and force a reboot
		 * newer boards (1543 ver A1-E) have a 
		 * builtin watchdog timer in the PMU
		 */
		pci_dev *pmu;
		byte reg;

		pmu = pci_lookup_dev(0x10b9, 0x7101, NULL);
		if (!pmu) {
			printf("Unable to find PMU [10b9:7101]!\n\n");
			return EBADARG;
		}

		printf("Rebooting - please wait");
	
		reg = pci_dev_readb(pmu, 0x92);
		pci_dev_writeb(pmu, 0x92, reg | 3);

		/* check for old boards
		 * use the onboard PIC for them */
		reg = pci_readb(0, PCI_DEVFN(7,0), 0x5e);
		if ((reg & 0x1e) != 0x12) {
			reg = pci_dev_readb(pmu, 0x7e);
			reg ^= 0x20; /* toggle DOGB (bit 6) */
			pci_dev_writeb(pmu, 0x7e, reg);
		}

		/* repeat until watchdog timer kicks in */
		for (;;) {
			printf(".");
			delay(100);
		}
		break;
	}

	case SYSTYPE_5K:
		/*
		 * use the "turbo/reset control" register (0xCF9)
		 *  1) write 0 to clear 'Hard Reset Enable'
		 *  2) 0->1 transition of bit 2 will cause reset
		 */
		printf("Rebooting - please wait...\n\n");
		delay(100);
		outb(2, 0xCF9);
		outb(6, 0xCF9);
		break;

	default:
	}

	return 0;
}

int halt_func(int argc, char *argv[] __attribute__ ((unused)))
{
	word addr;
	byte val;

	if (argc != 1)
		return EBADNUMARGS;

	/* set up for wake-on-lan */
	eth_wol_setup();

	switch (systype()) {
	case SYSTYPE_3K: 
		printf("This system does not support software power-off.\n\n");
		break;

	case SYSTYPE_5K:
		/* use the superio power-off register */
		printf("Halting system - please wait.\n\n");
		delay(100);

		switch (boardtype()) {
		case BOARDTYPE_MONTEREY:

			outb(PNP_NS_LOGICAL_DEV, PNP_NS_INDEX_PORT);
			outb(PNP_NS_DEV_RAQXTR_RTC, PNP_NS_DATA_PORT);
			outb(PNP_NS_ADDR_HIGH, PNP_NS_INDEX_PORT);

			addr = inb(PNP_NS_DATA_PORT) << 8;
			outb(PNP_NS_ADDR_LOW, PNP_NS_INDEX_PORT);
			addr |= inb(PNP_NS_DATA_PORT);

			if (addr) {
				/* set up bank 2 */
				outb(PNP_NS_RTC_CRA, addr);
				val = inb(addr + 1) & 0x8f;
				outb(val | PNP_NS_RTC_BANK_2, addr + 1);
			
				/* power off the machine with APCR1 */
				outb(PNP_NS_APCR1, addr);
				val = inb(addr + 1);
				outb(0x20 | val, addr + 1);
			}
			break;

		case BOARDTYPE_ALPINE:
			/* clear status bits, or it may wake up again! */
			for (val = 0; val < 4; val++) {
				outb(0xff, 0x540+val);
			}
			/* write sleep-state 5 to PM1b_CNT_HIGH on superio */
			outb(0x34, 0x531);
			break;

		default:
		}
		break;

	default:
	}

	return 0;
}

/* LCD is 1 param
 * console is 2 params
 */
int set_boot_method_func(int argc, char *argv[])
{
	switch (argc) {
	case 1:
		if (! strcmp(argv[0], "From disk")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_DISK);
			boot_method = CMOS_BOOT_METHOD_DISK;
			DPRINTF("Boot method is set to %s\n", "disk");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[0], "From ROM")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_ROM);
			boot_method = CMOS_BOOT_METHOD_ROM;
			DPRINTF("Boot method is set to %s\n", "rom");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[0], "From network")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_NET);
			boot_method = CMOS_BOOT_METHOD_NET;
			DPRINTF("Boot method is set to %s\n", "net");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[0], "From net kernel")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_NNET);
			boot_method = CMOS_BOOT_METHOD_NNET;
			DPRINTF("Boot method is set to %s\n", "net kernel");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[0], "From settings")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_OTHER);
			boot_method = CMOS_BOOT_METHOD_OTHER;
			DPRINTF("Boot method is set to %s\n", "other");
			cmos_set_checksum();
		}
		else return EBADARG;

		lcd_set_cur_pos(0x40);
		lcd_write_str("                "); 
		lcd_set_cur_pos(0x40);
		lcd_write_str("done"); 
		delay(1000);
		break;

	case 2:
		if (! strcmp(argv[1], "disk")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_DISK);
			boot_method = CMOS_BOOT_METHOD_DISK;
			DPRINTF("Boot method is set to %s\n", "disk");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[1], "rom")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_ROM);
			boot_method = CMOS_BOOT_METHOD_ROM;
			DPRINTF("Boot method is set to %s\n", "rom");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[1], "net")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_NET);
			boot_method = CMOS_BOOT_METHOD_NET;
			DPRINTF("Boot method is set to %s\n", "net");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[1], "netkernel")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_NNET);
			boot_method = CMOS_BOOT_METHOD_NNET;
			DPRINTF("Boot method is set to %s\n", "net kernel");
			cmos_set_checksum();
		}
		else if (! strcmp(argv[1], "other")) {
			cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_OTHER);
			boot_method = CMOS_BOOT_METHOD_OTHER;
			DPRINTF("Boot method is set to %s\n", "other");
			cmos_set_checksum();
		}
		else return EBADARG;
		break;

	default:
		return EBADNUMARGS;
	}

	return 0;
}

int read_boot_method_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	printf("%s\n", boot_methods[(int)boot_method].desc);

	return 0;
}

int set_root_dev_func(int argc, char *argv[])
{
	struct boot_dev_t *dev;

	/* LCD is 1 param, console 2 params */
	switch (argc) {
	case 1:
		dev = find_dev_by_name(argv[0]);
		if (!dev)
			return EBADARG;
		lcd_set_cur_pos(0x40);
		lcd_write_str("                "); 
		lcd_set_cur_pos(0x40);
		lcd_write_str("done"); 
		delay(1000);
		break;

	case 2:
		if (!(dev = find_dev_by_name(argv[1]))) {
			printf("Unknown boot device\n");
			return 0;
		}
		break;

	default:
		return EBADNUMARGS;
	}

	cmos_write_byte(CMOS_ROOT_DEV_MAJ, dev->maj);
	cmos_write_byte(CMOS_ROOT_DEV_MIN, dev->min);
	cmos_set_checksum();
	set_kernel_image(NULL); /* re-read boot device */

	DPRINTF("Root device is set to 0x%02x%02x\n", dev->maj, dev->min);

	return 0;
}

int read_root_dev_func(int argc, char *argv[] __attribute__ ((unused)))
{
	struct boot_dev_t *dev;

	if (argc != 1)
		return EBADNUMARGS;

	dev = find_dev_by_num(cmos_read_root_dev());

	if (!dev)
		printf("Unknown root device\n");
	else
		printf("%s\n", dev->name);

	return 0;
}

int set_boot_dev_func(int argc, char *argv[])
{
	struct boot_dev_t *dev;

	if (argc != 2)
		return EBADNUMARGS;

	if (!(dev = find_dev_by_name(argv[1]))) {
		printf("Unknown boot device\n");
		return 0;
	}

	cmos_write_byte(CMOS_BOOT_DEV_MAJ, dev->maj);
	cmos_write_byte(CMOS_BOOT_DEV_MIN, dev->min);
	cmos_set_checksum();
	set_kernel_image(NULL); /* re-read boot device */

	DPRINTF("Boot device is set to 0x%02x%02x\n", dev->maj, dev->min);

	return 0;
}

int read_boot_dev_func(int argc, char *argv[] __attribute__ ((unused)))
{
	struct boot_dev_t *dev;

	if (argc != 1)
		return EBADNUMARGS;

	dev = find_dev_by_num(cmos_read_boot_dev());

	if (!dev)
		printf("Unknown boot device\n");
	else
		printf("%s\n", dev->name);

	return 0;
}

int set_mem_limit_func(int argc, char *argv[])
{
	unsigned long int limit;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &limit) != 0)
		return EBADNUM;

	set_mem_limit((int)limit);

	return 0;
}

int boot_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	boot_kernel_linux();

	return 0;
}

int hlt_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	__asm__ __volatile__ ("hlt");

	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
