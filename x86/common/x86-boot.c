/* $Id: x86-boot.c,v 1.3 2003/12/11 00:57:09 thockin Exp $
 * Copyright (c) 1999,2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 */

#include <io.h>
#include <string.h>
#include <printf.h>

#include "common/x86-boot.h"
#include "common/eth.h"
#include "common/rom.h"
#include "common/isapnp.h"
#include "common/boot.h"
#include "common/rammap.h"
#include "common/eeprom.h"
#include "common/cmos.h"
#include "common/cache.h"

void (*return_from_load_func)(void);
static int mem_limit = 0;

/*
 * this is to setup the magic block at boot-time
 * (see linux/Documentation/i386/zero-page.txt)
 */
#define PARAM(x)		((unsigned char *)(x))
#define EXT_MEM_K(x) 		(*(unsigned short*)(PARAM(x) + 0x002))
#define MAGIC_NUMBER(x) 	(*(unsigned short*)(PARAM(x) + 0x020))
#define CMDLINE_OFFSET(x) 	(*(unsigned short*)(PARAM(x) + 0x022))
#define ALT_MEM_K(x) 		(*(unsigned long *)(PARAM(x) + 0x1E0))
#define SETUP_SIZE(x)		(*(unsigned char *)(PARAM(x) + 0x1F1))
#define MOUNT_ROOT_RDONLY(x) 	(*(unsigned short*)(PARAM(x) + 0x1F2))
#define ORIG_ROOT_DEV(x) 	(*(unsigned short*)(PARAM(x) + 0x1FC))
#define AUX_DEVICE_INFO(x) 	(*(unsigned char *)(PARAM(x) + 0x1FF))
#define HEADER_SIGNATURE(x)	(*(unsigned long *)(PARAM(x) + 0x202))
#define HEADER_VERSION(x)	(*(unsigned short*)(PARAM(x) + 0x206))
#define LOADER_TYPE(x) 		(*(unsigned char *)(PARAM(x) + 0x210))
#define LOAD_FLAGS(x) 		(*(unsigned char *)(PARAM(x) + 0x211))
#define KERNEL_START(x) 	(*(unsigned long *)(PARAM(x) + 0x214))
#define INITRD_START(x) 	(*(unsigned long *)(PARAM(x) + 0x218))
#define INITRD_SIZE(x) 		(*(unsigned long *)(PARAM(x) + 0x21C))
#define HEAP_END_PTR(x)		(*(unsigned short*)(PARAM(x) + 0x224))
#define CMD_LINE_PTR(x)		(*(unsigned long *)(PARAM(x) + 0x228))
#define COMMAND_LINE(x) 	((char *)          (PARAM(x) + 0x800))

/* RAMCODE_START converted to MB */
#define MEMSIZE_DEFAULT 	(RAMCODE_START >> 20)
#define GDT_MEM			0x500
#define CL_OFF			0x800
#define PAGE_SIZE		0x1000
#define LINUX_MAGIC		0xA33F
#define CONSOLE_DEF		"ttyS0,115200"
#define CONSOLE_OFF		"/dev/null"
#define SHUTDOWN_CODE		0xf
#define WARM_RESET_VECTOR_LO	0x467
#define WARM_RESET_VECTOR_HI	0x469
#define LINUX_ENTRY_ADDR	0x100000
#define LINUX_ENTRY_STACK	0x90000

struct cobalt_boot_return_data return_data;

/* IA-32 Intel Architecture Manual
 * volume 3, section 3.4.3.1, page 3-14
 *
 * "If the segment descriptors in the GDT or an LDT are placed in ROM,
 * the processor can enter an indefinite loop if software or the 
 * processor attempts to update (write to) the ROM-based segment 
 * descriptors. To prevent this problem, set the accessed bits for 
 * all segment descriptors that are placed in a ROM."
 */

unsigned int gdt_data[] =
{ 
    0x00000000, 0x00000000, /* 0x00 - null descriptor */
    0x0000ffff, 0x008f9300, /* 0x08 - unreal */
    0x0000ffff, 0x00cf9b00, /* 0x10 - kernel code segment,
			     *        type: execute/read, accessed */
    0x0000ffff, 0x00cf9300  /* 0x18 - kernel data segment
			     *        type: read/write, accessed */
};

#define START_LINUX	__asm__ __volatile__("movl $0x90000, %%esi\n\t"	\
					     "xorl %%ebx, %%ebx\n\t"	\
					     "ljmp $0x10, %0\n\t"	\
					     : : "i" (LINUX_ENTRY_ADDR));

/* 
 * set up the magic boot block for booting Linux
 */
void
setup_boot_linux (int stage, const char *params, const char *kernel_image)
{
	unsigned char *boot_sect = (unsigned char *) LINUX_ENTRY_STACK;
	unsigned char *clp;
	char *p;
	uint32 reset = RAM_MONITOR_CODE;

	/* set shutdown code */
	p = (unsigned char *) SHUTDOWN_CODE;
	*p = 0xa;

	/* set warm reset vector */
	p = (unsigned char *) WARM_RESET_VECTOR_LO;
	*p = (reset & 0xffff);
	p = (unsigned char *) WARM_RESET_VECTOR_HI;
	*p = ((reset >> 16) & 0xffff);

	/* turn on cache, if needed */
	enable_caches();
	memset(boot_sect, 0, PAGE_SIZE);

	/* memory over 1Meg */
	EXT_MEM_K(boot_sect) = ((mem_size >> 10) - 1024);
	ALT_MEM_K(boot_sect) = ((mem_size >> 10) - 1024);

	/* size of setup in 512b sectors */
	SETUP_SIZE(boot_sect) = 0x4;

	/* mount root fs read-only */
	MOUNT_ROOT_RDONLY(boot_sect) = 0x1;

	/* loader type undefined */
	LOADER_TYPE(boot_sect) = 0xff;
	ORIG_ROOT_DEV(boot_sect) = root_device;

	/* start of kernel code */
	KERNEL_START(boot_sect) = LINUX_ENTRY_ADDR;
	MAGIC_NUMBER(boot_sect) = LINUX_MAGIC;
	CMDLINE_OFFSET(boot_sect) = CL_OFF;
	LOAD_FLAGS(boot_sect) = 0x01;
	clp = (unsigned char *)(boot_sect + CL_OFF);

	/*
	 * setup kernel command line
	 */
	if (stage == BOOT_STAGE_LOAD) {
		ORIG_ROOT_DEV(boot_sect) = load_device;
		p = load_params ? load_params : "";
		snprintf(clp, 2048, "console=%s%s mem=%dM "
			 "cobalt_boot_image=%s cobalt_boot_return=0x%0x "
			 "cobalt_boot_data=0x%0x cobalt_boot_load=0x%0x "
			 "cobalt_ramcode_map=0x%0x,0x%0x %s%s",
			 (console && debug) ? CONSOLE_DEF : CONSOLE_OFF,
			 (debug) ? " debug" : "", MEMSIZE_DEFAULT,
			 kernel_image, (unsigned int) return_from_kernel,
			 (unsigned int) &return_data, RAM_SERIAL_LOAD,
			 RAMCODE_START, RAMCODE_END - RAMCODE_START,
			 params, p);
	} else {
		p = boot_params ? boot_params : "";
		if (mem_limit) /* BOOT_STAGE_RUN */
			snprintf(clp, 2048, "console=%s%s mem=%dM %s%s",
				 (console) ? CONSOLE_DEF : CONSOLE_OFF,
				 (debug) ? " debug" : "", mem_limit,
				 params, p);
		else /* BOOT_STAGE_RUN */
			snprintf(clp, 2048, "console=%s%s %s%s",
			 	(console) ? CONSOLE_DEF : CONSOLE_OFF,
			 	(debug) ? " debug" : "", params, p);

	}

#if DEBUG_BOOT
	printf("\n");
	printf("              boot_sect: %08x\n", (unsigned int)boot_sect);
	printf("        EXT_MEM_K[0002]: %04x\n", (unsigned int)EXT_MEM_K(boot_sect));
	printf("     MAGIC_NUMBER[0020]: %04x\n", (unsigned int)MAGIC_NUMBER(boot_sect));
	printf("   CMDLINE_OFFSET[0022]: %04x\n", (unsigned int)CMDLINE_OFFSET(boot_sect));
	printf("        ALT_MEM_K[01E0]: %08x\n", (unsigned int)ALT_MEM_K(boot_sect));
	printf("       SETUP_SIZE[01F1]: %02x\n", (unsigned int)SETUP_SIZE(boot_sect));
	printf("MOUNT_ROOT_RDONLY[01F2]: %04x\n", (unsigned int)MOUNT_ROOT_RDONLY(boot_sect));
	printf("    ORIG_ROOT_DEV[01FC]: %04x\n", (unsigned int)ORIG_ROOT_DEV(boot_sect));
	printf("  AUX_DEVICE_INFO[01FF]: %02x\n", (unsigned int)AUX_DEVICE_INFO(boot_sect));
	printf(" HEADER_SIGNATURE[0202]: %08x\n", (unsigned int)HEADER_SIGNATURE(boot_sect));
	printf("   HEADER_VERSION[0206]: %04x\n", (unsigned int)HEADER_VERSION(boot_sect));
	printf("      LOADER_TYPE[0210]: %02x\n", (unsigned int)LOADER_TYPE(boot_sect));
	printf("       LOAD_FLAGS[0211]: %02x\n", (unsigned int)LOAD_FLAGS(boot_sect));
	printf("     KERNEL_START[0214]: %08x\n", (unsigned int)KERNEL_START(boot_sect));
	printf("     INITRD_START[0218]: %08x\n", (unsigned int)INITRD_START(boot_sect));
	printf("      INITRD_SIZE[021C]: %08x\n", (unsigned int)INITRD_SIZE(boot_sect));
	printf("     HEAP_END_PTR[0224]: %04x\n", (unsigned int)HEAP_END_PTR(boot_sect));
	printf("     CMD_LINE_PTR[0228]: %08x\n", (unsigned int)CMD_LINE_PTR(boot_sect));
#endif

	DPRINTF("command line: '%s'\n", clp);
	DPRINTF("booting kernel...\n");

	eeprom_set_bank_page(0, 0);
}

volatile void
read_kernel_from_disk(char *kernel_image, void (*return_func)(void))
{
	return_from_load_func = return_func;

	setup_boot_linux(BOOT_STAGE_LOAD, "ip=off ", kernel_image);
	START_LINUX;
}

volatile void
setup_net_boot (const int type, const char *kernel_image,
		void (*return_func)(void))
{
	char str[100];
	return_from_load_func = return_func;

	if (unique_root && bto_ip)
		snprintf(str, 100, "ip=on root=/dev/nfs nfsroot=%d.%d.%d.%d:/nfsroot-x86-%s,rsize=8192 ",
			 (bto_ip >> 24) & 0xff, (bto_ip >> 16) & 0xff,
			 (bto_ip >>  8) & 0xff, (bto_ip >>  0) & 0xff,
			 eth_get_first_mac());
	else if (unique_root)
		snprintf(str, 100, "ip=on root=/dev/nfs nfsroot=/nfsroot-x86-%s,rsize=8192 ",
			 eth_get_first_mac());
	else if (bto_ip) 
		snprintf(str, 100, "ip=on root=/dev/nfs nfsroot=%d.%d.%d.%d:/nfsroot-x86,rsize=8192 ",
			 (bto_ip >> 24) & 0xff, (bto_ip >> 16) & 0xff,
			 (bto_ip >>  8) & 0xff, (bto_ip >>  0) & 0xff);
	else 
		snprintf(str, 100, "ip=on root=/dev/nfs nfsroot=/nfsroot-x86,rsize=8192 ");

	setup_boot_linux(type, str, kernel_image);
	START_LINUX;
}

#define getcr0(cr0) asm("mov %%cr0, %0": "=r" (cr0))
#define setcr0(cr0) asm("mov %0, %%cr0": :"r" (cr0))

void
return_from_kernel (void)
{
	unsigned int cr0, esp, ss;
	struct {
		unsigned short size __attribute__ ((packed));
		unsigned long base __attribute__ ((packed));
	} gdt;

	DPRINTF("Back in ramcode: ");

	/* switch off paging */
	getcr0(cr0);
	cr0 = 0x11;
	setcr0(cr0);

        /* setup stack */
	esp = RAMCODE_END - 4;
	asm("mov %0, %%esp": :"r" (esp));
	asm("mov %0, %%ebp": :"r" (esp));

        /* set up gdt */
	memcpy((void *) 0x500, (void *) gdt_data, sizeof(gdt_data));

	gdt.size = sizeof(gdt_data) - 1;
	gdt.base = (unsigned long)0x500;
	asm("lgdt %0": :"m" (gdt));
    
	ss = 0x18;
	asm("mov %0, %%ss": :"r" (ss));
	asm("mov %0, %%ds": :"r" (ss));

        /* setup stack */
	esp = RAMCODE_END - 0x10;
	asm("mov %0, %%esp": :"r" (esp));
	asm("mov %0, %%ebp": :"r" (esp));

	DPRINTF("done\n");

	return_from_load_func();
}

volatile void
boot_kernel_linux (void)
{
	setup_boot_linux(BOOT_STAGE_RUN, "ip=off ", NULL);
	START_LINUX;
}

void
set_mem_limit (int limit)
{
	mem_limit = limit;
}

#define BSD_ASKNAME      0x01    /* ask for file name to reboot from */
#define BSD_SINGLE       0x02    /* reboot to single user only */
#define BSD_NOSYNC       0x04    /* dont sync before reboot */
#define BSD_HALT         0x08    /* don't reboot, just halt */
#define BSD_INITNAME     0x10    /* name given for /etc/init (unused) */
#define BSD_DEFROOT      0x20    /* use compiled-in rootdev */
#define BSD_KDB          0x40    /* give control to kernel debugger */
#define BSD_RDONLY       0x80    /* mount root fs read-only */
#define BSD_DUMP         0x100   /* dump kernel memory before reboot */
#define BSD_MINIROOT     0x200   /* mini-root present in memory */
#define BSD_CONFIG       0x400   /* invoke user configuration routing */
#define BSD_VERBOSE      0x800   /* print all potentially useful info */
#define BSD_SERIAL       0x1000  /* use serial port as console */
#define BSD_CDROM        0x2000  /* use cdrom as root */

volatile void
boot_kernel_netbsd (void)
{
	printf("\nBooting NetBSD:\n");
	printf("     Entry: 0x%0x\n", (unsigned int) kernel_start_addr);
	printf("     Flags: 0x%0x\n", BSD_SINGLE | BSD_VERBOSE | BSD_SERIAL);
	printf("   Low Mem: %d Kb\n", (640 - 1));
	printf("   Ext Mem: %d Kb\n", (mem_size >> 10) - 1);
	printf("\njumping to kernel...\n");

	PCI_POST(0xff);

	(*kernel_start_addr) (BSD_SINGLE | BSD_VERBOSE | BSD_SERIAL,
			      0, 0, 0, (mem_size >> 10) - 1, 640 - 1);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
