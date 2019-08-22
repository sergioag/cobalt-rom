/*
 * $Id: boot_main.c,v 1.3 2004/01/23 01:40:34 thockin Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 *
 * contains kernel image loaders & boot methods
 * LCD boot message routines
 * 
 * Author(s): Timothy Stonis
 *            Erik Gilling
 *            Tim Hockin
 *            Duncan Laurie
 *            Adrian Sun
 */

#include "linux/elf.h"
#include "common/elf.h"

#include <string.h>
#include <printf.h>

#include "common/rom.h"
#include "common/rammap.h"
#include "common/boot.h"
#include "common/serial.h"
#include "common/spin.h"
#include "common/fs.h"
#include "common/x86-boot.h"
#include "common/lcd.h"
#include "common/cmos.h"
#include "common/cache.h"
#include "common/delay.h"
#include "common/bzlib.h"
#include "common/misczip.h"

static void relocate_and_zero(Elf32_Phdr *);
static void extract_header_info(unsigned char *);
static void bfd_linux(void);
static void bfn_linux(void);
static void bfd_netbsd(void);

extern struct cobalt_boot_return_data return_data;
extern unsigned int serial_load_size;
unsigned int root_device, load_device;
char *boot_params, *load_params;
kernel_start_func *kernel_start_addr;

/* local variables that get set via functions */
static char *kernel_image = BOOT_KERNEL_LINUX;
static int boot_opt = CMOS_BOOT_TYPE_DEF;
static void (*boot_fcn)(void) = bfd_linux;

/*
 *   table of available boot methods and descriptions. these are
 *   really just short cuts.
 */
struct boot_method_t boot_methods[] = {
	{ "From disk", do_bfX, BOOT_OPT_BFD},	/* bfd must be index 0 */
	{ "From ROM",  do_bfX, BOOT_OPT_BFR },
	{ "From network",  do_bfX, BOOT_OPT_BFN },
	{ "From network (kernel)",  do_bfX, BOOT_OPT_BFNN },
	{ "From settings",  do_bfX, BOOT_OPT_OTHER }    /* use cmos/other settings */
};

/*
 *    override the default kernel image defined above
 */
void set_kernel_image(char *image)
{
	if (image)
		kernel_image = image;
	root_device = cmos_read_root_dev();
	load_device = cmos_read_boot_dev();
}


void set_boot_opt(const int mask, int method)
{
	/* mask off unwanted types */
	method &= mask;
	boot_opt &= ~mask;
	boot_opt |= method;
}

void net_boot(const int type)
{
	setup_net_boot(type, kernel_image, boot_fcn);
}

/*
 *   relocate the ELF file (kernel image),
 *   zero the BSS
 */
static void relocate_and_zero(Elf32_Phdr *phdr)
{
	unsigned long ctr;
	unsigned char *src, *dest;

#if DEBUG_BOOT
	printf("\n");
	printf("       offset: 0x%0x\n", (unsigned int)phdr->p_offset);
	printf("        vaddr: 0x%0x\n", (unsigned int)phdr->p_vaddr);
	printf("        paddr: 0x%0x\n", (unsigned int)phdr->p_paddr);
	printf("     filesize: 0x%0x\n", (unsigned int)phdr->p_filesz);
	printf("      memsize: 0x%0x\n", (unsigned int)phdr->p_memsz);
	printf("        flags: 0x%0x\n", (unsigned int)phdr->p_flags);
#endif

	if (phdr->p_filesz == 0) {
		printf("ERROR: cannot relocate with filesize %d\n",
			(unsigned int) phdr->p_filesz);
		return;
	}

	/* Setup the source and destination */
	dest = (unsigned char *) phdr->p_vaddr;
	src  = (unsigned char *) (RAM_DECOMP_AREA + phdr->p_offset); 

	/* kernel loads at 0x00100000, but relocs below 16MB */
	(unsigned long)dest &= ~0xff000000; /* mask off > 16 MB */

#if DEBUG_BOOT
	printf("Relocating %d bytes from 0x%0x to 0x%0x\n",
	       (unsigned int) phdr->p_filesz,
	       (unsigned int) src,
	       (unsigned int) dest);
#endif
	/* copy the segment to new address */
	for (ctr=0; ctr < phdr->p_filesz; ctr++)
		*(dest++) = *(src++);

	/* in case memory size if larger than data size */
	if (phdr->p_memsz > phdr->p_filesz) {
#if DEBUG_BOOT
		printf("Zeroing %d bytes from 0x%0x to 0x%0x\n",
		       (unsigned int) phdr->p_memsz-phdr->p_filesz,
		       (unsigned int) dest,
		       (unsigned int) dest+(phdr->p_memsz-phdr->p_filesz));
#endif
		for (ctr=phdr->p_filesz; ctr<phdr->p_memsz; ctr++)
			*(dest++) = 0;
	}
}

/*
 *   parse kernel image and extract header info
 */
static void extract_header_info(unsigned char *addr)
{
	Elf32_Ehdr	*ehead;
	Elf32_Phdr	*phead;
	Elf32_Shdr	*shead;
	unsigned char	ctr;
    
	ehead = (Elf32_Ehdr *)addr;
	shead = (Elf32_Shdr *)((unsigned long) addr + 
			       (unsigned long) (ehead->e_shoff));

#if DEBUG_BOOT
	printf("\nExtracting ELF header: %s\n",
	       (elf_valid(ehead) ? "OK" : "INVALID"));
	printf("        ehead: 0x%0x\n", (unsigned int)ehead);
	printf("        shead: 0x%0x\n", (unsigned int)shead);
	printf("      e_ident: %d\n",    (unsigned int)ehead->e_ident);
	printf("       e_type: %d\n",    (unsigned int)ehead->e_type);
	printf("    e_machine: %d\n",    (unsigned int)ehead->e_machine);
	printf("    e_version: 0x%08x\n",(unsigned int)ehead->e_version);
	printf("      e_entry: 0x%08x\n",(unsigned int)ehead->e_entry);
	printf("      e_phoff: 0x%08x\n",(unsigned int)ehead->e_phoff);
	printf("      e_shoff: 0x%08x\n",(unsigned int)ehead->e_shoff);
	printf("      e_flags: 0x%08x\n",(unsigned int)ehead->e_flags);
	printf("     e_ehsize: %d\n",    (unsigned int)ehead->e_ehsize);
	printf("  e_phentsize: %d\n",    (unsigned int)ehead->e_phentsize);
	printf("      e_phnum: %d\n",    (unsigned int)ehead->e_phnum);
	printf("  e_shentsize: %d\n",    (unsigned int)ehead->e_shentsize);
	printf("      e_shnum: %d\n",    (unsigned int)ehead->e_shnum);
	printf("   e_shstrndx: %d\n",    (unsigned int)ehead->e_shstrndx);
#endif

	/* for each program segment, relocate it - ELF spec might help */
	for (ctr = 0; ctr < ehead->e_phnum; ctr++) {
		phead = (Elf32_Phdr *)((unsigned long) addr + 
			       (unsigned long) ehead->e_phoff +
			       (unsigned long) (ehead->e_phentsize * ctr));
		relocate_and_zero(phead);
	}
    
	/* linux loads at 0x00100000, but relocs below 16MB */
	kernel_start_addr = (kernel_start_func*) (ehead->e_entry & 0xffffff);
}

/*
 *   load a kernel image from the serial port
 *   to the memory location RAM_SERIAL_LOAD
 */
void dl_kernel(void)
{
	unsigned char inbyte;
	unsigned int ctr = 0;
	unsigned char *load_addr = (unsigned char *) RAM_SERIAL_LOAD;

	printf("Loading Kernel...\n");
	serial_inb(&inbyte);

	do {
		*(load_addr + ctr) = inbyte;
		if ((++ctr % 0x400) == 0)
			printf("\r%dk", ctr>>10);
	} while (!serial_inb_timeout(&inbyte, 8*BOGO_SEC));

	serial_load_size = ctr;
	printf("\r%dk done\n", ctr>>10);
}

/*
 *   uncompress kernel image from
 *   RAM_SERIAL_LOAD to RAM_DECOMP_AREA
 */
void bunzip2_kernel(void)
{
	int err;
	unsigned int destLen= RAM_DECOMP_AREA_MAX-RAM_DECOMP_AREA;

	enable_caches();
	reset_lcd_for_boot();

	printf("Decompressing -");

	spin_reset();
	
	if( (err = BZ2_bzBuffToBuffDecompress( (char *)RAM_DECOMP_AREA, &destLen,
					       (char *)RAM_SERIAL_LOAD, serial_load_size, 
					       1, 0 )) != BZ_OK )
	{
	    printf( "Error bunzip2ing kernel. Trying gunzip.\n" );
	    if( uncompress_kernel((char *)RAM_SERIAL_LOAD) < 0 )
	    {
		printf( "Error gunziping kernel. Giving up.\n" );
		return;
	    }
	}
	
	printf("\b- done\n");

	extract_header_info((unsigned char *) RAM_DECOMP_AREA);
}

/*
 *   default caller for boot_default
 */
int boot_default_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
    
	return boot_default();
}

/*
 *   boot system with whatever method is default
 */
int boot_default(void)
{
	int r, i = (int) boot_method;

	printf("Booting default method - %s\n\n", 
	       boot_methods[i].desc);

	r = boot_methods[i].boot_func(boot_methods[i].arg);

	/* we shouldn't actually get here */
	return r;
}

static int boot_prep(void)
{
	enable_caches();
	reset_lcd_for_boot();

	if (load_from_romfs(ROM_KERNEL) < 0)
		return -1;

	printf("First stage kernel (Linux): ");
	bunzip2_kernel();
	return 0;
}

int do_bfd_netbsd (int unused __attribute__ ((unused)))
{
	if (boot_prep() < 0)
		return -1;

	read_kernel_from_disk(kernel_image, bfd_netbsd);

	/* should not return */
	return -4;
}

/*
 *   load a kernel and then boot with filesystem options 
 */ 
int do_bfX(int options)
{
	if (boot_prep() < 0)
		return -1;

	if (options != BOOT_OPT_OTHER)
		set_boot_opt(BOOT_OPT_MASK, options);

	/* alter fs function */
	boot_fcn = ((boot_opt & CMOS_BOOT_FS_MASK) == CMOS_BOOT_FS_NET) ? 
		bfn_linux : bfd_linux;

	options = boot_opt & CMOS_BOOT_KERN_MASK;
	if (options == CMOS_BOOT_KERN_ROM) /* rom kernel */
		if (boot_fcn == bfn_linux)
			net_boot(BOOT_STAGE_RUN);
		else 
			boot_kernel_linux();
	else if (options == CMOS_BOOT_KERN_NET) /* net kernel */
		net_boot(BOOT_STAGE_LOAD);
	else /* CMOS_BOOT_KERN_DEV or unknown == device kernel */
	       read_kernel_from_disk(kernel_image, boot_fcn);
	return -2;
}

static void
bfd_netbsd(void)
{
	printf("Second stage kernel (NetBSD): ");
	serial_load_size = return_data.flen;
	bunzip2_kernel();
	boot_kernel_netbsd();
}


/* common stuff for linux booting */
static void bfX_linux_prep(void)
{
    if( return_data.error < 0 )
    {
	printf( "Second stage load failed.  Booting ROM kernel...\n" );
	
	lcd_set_cur_pos(0x04);
	lcd_write_str("Failed! Boot");
	lcd_set_cur_pos(0x44);
	lcd_write_str("ing from ROM");
	delay(5000);
	if (load_from_romfs(ROM_KERNEL) < 0)
	{
	    printf( "Can't Load ROM kernel!!!!\n" );
	    lcd_set_cur_pos(0x04);
	    lcd_write_str("ROM Boot    ");
	    lcd_set_cur_pos(0x44);
	    lcd_write_str("Failed!     ");	    
	    while(1);
	}
    }
    else
    {
	printf("Second stage kernel: ");
	lcd_set_cur_pos( 0x44 );
	    /* put half a progess bar on */
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_char(0xff);
	lcd_write_str("      ");
	serial_load_size = return_data.flen;
    }

    bunzip2_kernel(); 
}

/*
 *  load kernel from disk, uncompress and boot it
 *  give fancy LCD status as well
 */
static void
bfd_linux(void)
{
    bfX_linux_prep();
    boot_kernel_linux();
}

/* net fs */
static void bfn_linux(void)
{
    bfX_linux_prep();
    net_boot(BOOT_STAGE_RUN);
}

extern char *title1, *title2;

/*
 *  clear LCD and display flashy startup graphics
 */
void reset_lcd_for_boot(void)
{
    lcd_set_font( 3 );

    lcd_set_cur_pos(0x00);
    lcd_write_char(0x00);
    lcd_write_char(0x01);
    lcd_write_char(0x02);
    lcd_write_char(0x03);
    lcd_write_str("                ");
    lcd_set_cur_pos(0x04);
    lcd_write_str( title1 );
   
    lcd_set_cur_pos(0x40);
    lcd_write_char(0x04);
    lcd_write_char(0x05);
    lcd_write_char(0x06);
    lcd_write_char(0x07);
    lcd_write_str("                ");
    lcd_set_cur_pos(0x44);
    lcd_write_str( title2 );
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
