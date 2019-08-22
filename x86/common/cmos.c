/*
 *
 * Filename: cmos.c
 *
 * Description: Functions dealing with reading/writing CMOS
 *
 * Author(s): Tim Hockin
 *
 * Copyright 1999 Cobalt Networks, Inc.
 *
 */

#include "common/cmos.h"

#ifndef INROM

#include <stdio.h>
#include <string.h>

#else /* #ifdef INROM */

#include <string.h>
#include <stdlib.h>

#include "io.h"
#include "common/rom.h"
#include "common/systype.h"

static void cmos_write_serial_num(char *);
static void cmos_write_rom_rev(void);
static void init_bto_ip(void);

/* cmos stored info/flags */
char serial_num[16];
unsigned long uptime;
char console;
char debug;
char boot_method;
char auto_prompt;
char first_boot;
char clean_boot;
char hw_noprobe;
char show_logo;
char unique_root;
char sysfault;
unsigned int bto_ip;
unsigned char cmos_version;
unsigned char boot_type;

static struct cmos_flag cmos_flags[] = {
	{ "debug",	CMOS_DEBUG_FLAG,	0 },
	{ "console",	CMOS_CONSOLE_FLAG,	0 },
	{ "prompt",	CMOS_AUTO_PROMPT_FLAG,	0 },
	{ "clean_boot",	CMOS_CLEAN_BOOT_FLAG,	0 },
	{ "sysfault",	CMOS_SYSFAULT_FLAG,	0 },
	{ "nologo",	CMOS_NOLOGO_FLAG,	0 },
	{ "unique_root",CMOS_UNIQUE_FLAG,	0 },
	{ 0, 0, 0 }
};

int 
cmos_init(void)
{
	if( (!cmos_check_checksum()) )
	{
	    first_boot = 1;
	    do_first_boot();
	    cmos_set_checksum();
	} else {
	    first_boot = 0;
	}

	/* make sure nothing is in CMOS here - linux will probe for IDE info */
	cmos_write_byte(CMOS_BIOS_DRIVE_INFO, 0x0);
	
	cmos_write_rom_rev();

	/* if the version valid flag is not set, set it and zero the version */
	if (!cmos_read_flag(CMOS_VERSION_FLAG)) {
		cmos_write_flag(CMOS_VERSION_FLAG, 1);
		cmos_write_byte(CMOS_VERSION, 0);
	}

	console = cmos_read_flag(CMOS_CONSOLE_FLAG);
	debug = cmos_read_flag(CMOS_DEBUG_FLAG) && console;
	clean_boot = cmos_read_flag(CMOS_CLEAN_BOOT_FLAG);
	show_logo = !cmos_read_flag(CMOS_NOLOGO_FLAG);
	auto_prompt = cmos_read_flag(CMOS_AUTO_PROMPT_FLAG) && console;
	unique_root = cmos_read_flag(CMOS_UNIQUE_FLAG);
	sysfault = cmos_read_flag(CMOS_SYSFAULT_FLAG);

	boot_method = cmos_read_byte(CMOS_BOOT_METHOD);
	cmos_version = cmos_read_byte(CMOS_VERSION);
	boot_type = cmos_read_byte(CMOS_BOOT_TYPE);

	/* 
	 * The only place we worry about version is here - all the rest of 
	 * the ROM code can assume the very latest version, unless you run 
	 * before this.
	 */
	if (cmos_version < CMOS_VERSION_CURRENT) {
	    if (cmos_version < CMOS_VER_BTOCODE) {
		/* zero out the BTO code */
		cmos_write_byte(CMOS_BTO_CODE_0, 0x0);
		cmos_write_byte(CMOS_BTO_CODE_1, 0x0);
		cmos_write_byte(CMOS_BTO_CODE_2, 0x0);
		cmos_write_byte(CMOS_BTO_CODE_3, 0x0);
	    }
	    
	    cmos_write_byte(CMOS_VERSION, CMOS_VERSION_CURRENT);
	    cmos_version = CMOS_VERSION_CURRENT;
	    cmos_set_checksum();
	}

	init_bto_ip();

	cmos_read_serial_num(serial_num);

	cmos_flags_sync(0);

	return 0;
}

void 
cmos_set_defaults(void)
{
	unsigned char maj, min;

	/* set all flags to 0 */
	cmos_write_byte(CMOS_FLAG_BYTE_0, 0x0);
	cmos_write_byte(CMOS_FLAG_BYTE_1, 0x0);

	/* set initial flag values */
	cmos_write_flag(CMOS_CONSOLE_FLAG, 1);
	cmos_write_flag(CMOS_DEBUG_FLAG, 1); /* FIXME: set to 0 for GM */
	cmos_write_flag(CMOS_CLEAN_BOOT_FLAG, 1);
	/* cmos_write_flag(CMOS_AUTO_PROMPT_FLAG, 0); */
	/* cmos_write_flag(CMOS_SYSFAULT_FLAG, 0); */
	/* cmos_write_flag(CMOS_OOPSPANIC_FLAG, 0); */
	/* cmos_write_flag(CMOS_NOLOGO_FLAG, 0); */
	/* cmos_write_flag(CMOS_HW_NOPROBE_FLAG, 0); */
	/* cmos_write_flag(CMOS_DELAY_CACHE_FLAG, 0); */
	cmos_write_flag(CMOS_VERSION_FLAG, 1);

	/* default to bfd - index 0 */
	cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_DISK);

	/* default bfX option is to boot from disk. */
	cmos_write_byte(CMOS_BOOT_TYPE, CMOS_BOOT_TYPE_DEF);

	/* FIXME: handle this better for future..
	 * set default boot device:
	 *
	 * SYSTYPE_3K = hda1
	 * SYSTYPE_5K = md1
	 */
	maj = (systype_is_3k()) ? 0x03 : 0x09;
	min = 0x01;

	cmos_write_byte(CMOS_BOOT_DEV_MAJ, maj);
	cmos_write_byte(CMOS_BOOT_DEV_MIN, min);
	cmos_write_byte(CMOS_ROOT_DEV_MAJ, maj);
	cmos_write_byte(CMOS_ROOT_DEV_MIN, min);

	/* default to 'off' powermode */
	cmos_set_powerapply(0);
	cmos_set_powerfail(0);

	/* zero BTO IP */
	cmos_write_byte(CMOS_BTO_IP_0, 0);
	cmos_write_byte(CMOS_BTO_IP_1, 0);
	cmos_write_byte(CMOS_BTO_IP_2, 0);
	cmos_write_byte(CMOS_BTO_IP_3, 0);
	cmos_write_byte(CMOS_BTO_IP_CSUM, 0);

	cmos_set_checksum();
}

void 
cmos_first_boot_defaults_only( int reset_sn )
{
	/* clear these - they are not used in ROM, but are in userland */
	cmos_write_byte(CMOS_UPTIME_0, 0x0);
	cmos_write_byte(CMOS_UPTIME_1, 0x0);
	cmos_write_byte(CMOS_UPTIME_2, 0x0);
	cmos_write_byte(CMOS_UPTIME_3, 0x0);

	cmos_write_byte(CMOS_BOOTCOUNT_0, 0x0);
	cmos_write_byte(CMOS_BOOTCOUNT_1, 0x0);
	cmos_write_byte(CMOS_BOOTCOUNT_2, 0x0);
	cmos_write_byte(CMOS_BOOTCOUNT_3, 0x0);

	cmos_write_byte(CMOS_BTO_CODE_0, 0x0);
	cmos_write_byte(CMOS_BTO_CODE_1, 0x0);
	cmos_write_byte(CMOS_BTO_CODE_2, 0x0);
	cmos_write_byte(CMOS_BTO_CODE_3, 0x0);

        if( reset_sn )
            cmos_write_serial_num("Uninitialized");

	/* default to bfnn */
	cmos_write_byte(CMOS_BOOT_METHOD, CMOS_BOOT_METHOD_NNET);

            /* alpine need's it's wake event select registers cleared */

        if( boardtype() == BOARDTYPE_ALPINE )
        {
            outb( 0x00, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x01, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x02, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x03, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x04, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x05, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x06, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x07, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x08, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x09, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0a, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0b, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0c, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0d, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0e, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x0f, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x10, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x11, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x12, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x13, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x14, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x15, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x16, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x17, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x18, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x19, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1a, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1b, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1c, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1d, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1e, 0x500 ); outb( 0x00, 0x501 );
            outb( 0x1f, 0x500 ); outb( 0x00, 0x501 );
        }
            

	cmos_set_checksum();
}

static void 
cmos_write_rom_rev(void)
{
	cmos_write_byte(CMOS_ROM_REV_MAJ, V_MAJ); 
	cmos_write_byte(CMOS_ROM_REV_MIN, V_MIN); 
	cmos_write_byte(CMOS_ROM_REV_REV, V_REV);

	cmos_set_checksum();
}


static void 
cmos_write_serial_num(char *sn)
{
	char *str = (char *)strdup(sn);
	sn[13] = '\0';

	cmos_write_byte(CMOS_SYS_SERNUM_0, str[0]);
	cmos_write_byte(CMOS_SYS_SERNUM_1, str[1]);
	cmos_write_byte(CMOS_SYS_SERNUM_2, str[2]);
	cmos_write_byte(CMOS_SYS_SERNUM_3, str[3]);
	cmos_write_byte(CMOS_SYS_SERNUM_4, str[4]);
	cmos_write_byte(CMOS_SYS_SERNUM_5, str[5]);
	cmos_write_byte(CMOS_SYS_SERNUM_6, str[6]);
	cmos_write_byte(CMOS_SYS_SERNUM_7, str[7]);
	cmos_write_byte(CMOS_SYS_SERNUM_8, str[8]);
	cmos_write_byte(CMOS_SYS_SERNUM_9, str[9]);
	cmos_write_byte(CMOS_SYS_SERNUM_10, str[10]);
	cmos_write_byte(CMOS_SYS_SERNUM_11, str[11]);
	cmos_write_byte(CMOS_SYS_SERNUM_12, str[12]);

	cmos_write_byte(CMOS_SYS_SERNUM_CSUM, cmos_gen_csum(str));
	cmos_set_checksum();
	free(str);
}

#endif /* INROM */


/*
 * from here down code is shared with cmos tool
 */

void 
cmos_read_serial_num(char *sernum)
{
	unsigned char cmos_csum, read_csum;

	/* 13 bytes of serial num */
	sernum[0] = cmos_read_byte(CMOS_SYS_SERNUM_0);
	sernum[1] = cmos_read_byte(CMOS_SYS_SERNUM_1);
	sernum[2] = cmos_read_byte(CMOS_SYS_SERNUM_2);
	sernum[3] = cmos_read_byte(CMOS_SYS_SERNUM_3);
	sernum[4] = cmos_read_byte(CMOS_SYS_SERNUM_4);
	sernum[5] = cmos_read_byte(CMOS_SYS_SERNUM_5);
	sernum[6] = cmos_read_byte(CMOS_SYS_SERNUM_6);
	sernum[7] = cmos_read_byte(CMOS_SYS_SERNUM_7);
	sernum[8] = cmos_read_byte(CMOS_SYS_SERNUM_8);
	sernum[9] = cmos_read_byte(CMOS_SYS_SERNUM_9);
	sernum[10] = cmos_read_byte(CMOS_SYS_SERNUM_10);
	sernum[11] = cmos_read_byte(CMOS_SYS_SERNUM_11);
	sernum[12] = cmos_read_byte(CMOS_SYS_SERNUM_12);
	sernum[13] = '\0';

	cmos_csum = cmos_read_byte(CMOS_SYS_SERNUM_CSUM);
	read_csum = cmos_gen_csum(sernum);

	if (read_csum != cmos_csum) 
	{
	    if( systype_is_3k() ) /* migrate old checksum loacation */
	    {
		cmos_csum = cmos_read_byte( CMOS_SYS_SERNUM_OLD_CSUM );
		if( read_csum == cmos_csum )
		{
		    cmos_write_byte( CMOS_SYS_SERNUM_CSUM, cmos_csum );
		    cmos_set_checksum();
		    return;
		}
	    }
#ifdef INROM
	    strcpy(sernum, "invalid csum!");
#else
	    strcat(sernum, " (invalid csum)");
#endif
	}
}

/* not very sophisticated - but enough to catch a casual user changing stuff */
unsigned char
cmos_gen_csum(char *str)
{
        int i;
        int sum=0;
        char *key = "cNoEbTaWlOtR!";

        for (i = 0; i < CMOS_SYS_SERNUM_LEN; i++) {
                sum += str[i] ^ key[i];
        }
        /* magic - low byte of "CobaltNetwork" summed as above */
        sum = ((sum & 0x7f) ^ (0xd6)) & 0xff;

        return (unsigned char)sum;
}

unsigned short
cmos_read_boot_dev(void)
{
	unsigned short ret;
	unsigned char maj;
	unsigned char min;

	maj = cmos_read_byte(CMOS_BOOT_DEV_MAJ);
	min = cmos_read_byte(CMOS_BOOT_DEV_MIN);
	ret = (((unsigned short)maj)<<8) | min;

	return ret;
}

unsigned short
cmos_read_root_dev(void)
{
	unsigned short ret;
	unsigned char maj;
	unsigned char min;

	maj = cmos_read_byte(CMOS_ROOT_DEV_MAJ);
	min = cmos_read_byte(CMOS_ROOT_DEV_MIN);
	ret = (((unsigned short)maj)<<8) | min;

	return ret;
}

static void
init_bto_ip(void)
{
	unsigned char calcsum;
	unsigned char csum;
	unsigned char btoar[4];

	btoar[0] = cmos_read_byte(CMOS_BTO_IP_0);
	btoar[1] = cmos_read_byte(CMOS_BTO_IP_1);
	btoar[2] = cmos_read_byte(CMOS_BTO_IP_2);
	btoar[3] = cmos_read_byte(CMOS_BTO_IP_3);
	csum = cmos_read_byte(CMOS_BTO_IP_CSUM);

	calcsum = btoar[0] + btoar[1] + btoar[2] + btoar[3];
	if (calcsum != csum) {
		/* hrrrm, reset it */
		cmos_write_byte(CMOS_BTO_IP_0, 0);
		cmos_write_byte(CMOS_BTO_IP_1, 0);
		cmos_write_byte(CMOS_BTO_IP_2, 0);
		cmos_write_byte(CMOS_BTO_IP_3, 0);
		cmos_write_byte(CMOS_BTO_IP_CSUM, 0);
		cmos_set_checksum();
		bto_ip = 0;
	} else {
		bto_ip = (btoar[0]<<24) | (btoar[1]<<16) 
			| (btoar[2]<<8) | (btoar[3]<<0);
	}
}

void cmos_set_bto_ip( unsigned int ip )
{
    unsigned char csum;

    csum = ((ip>>24)&0xff) + ((ip>>16)&0xff) + ((ip>>8)&0xff) + (ip&0xff);

    cmos_write_byte(CMOS_BTO_IP_0, (ip>>24) & 0xff );
    cmos_write_byte(CMOS_BTO_IP_1, (ip>>16) & 0xff );
    cmos_write_byte(CMOS_BTO_IP_2, (ip>>8) & 0xff );
    cmos_write_byte(CMOS_BTO_IP_3, ip & 0xff );
    cmos_write_byte(CMOS_BTO_IP_CSUM, csum);
    cmos_set_checksum();
    bto_ip = ip;

}

struct cmos_flag *
cmos_flags_find(int flag)
{
	struct cmos_flag *f = cmos_flags;
	while (f->name) {
		if (f->flag == flag)
			return f;
		f++;
	}
	return NULL;
}

struct cmos_flag *
cmos_flags_namefind(char * name)
{
	struct cmos_flag *f = cmos_flags;
	while (f->name) {
		if (!strncmp(name, f->name, strlen(name)))
			return f;
		f++;
	}
	return NULL;
}


void cmos_flags_print(void)
{
	struct cmos_flag *f = cmos_flags;

	printf("CMOS flags:\n");
	while (f->name) {
		printf("%s %s\n", f->name, (f->state) ? "on" : "off");
		f++;
	}
}

void
cmos_flags_sync(int dir)
{
	struct cmos_flag *f = cmos_flags;

	while (f->name) {
		switch (f->flag) {
		case CMOS_DEBUG_FLAG:
			if (dir)
				debug = f->state;
			else
				f->state = debug;
			break;
		case CMOS_CONSOLE_FLAG:
			if (dir)
				console = f->state;
			else
				f->state = console;
			break;
		case CMOS_AUTO_PROMPT_FLAG:
			if (dir)
				auto_prompt = f->state;
			else
				f->state = auto_prompt;
			break;
		case CMOS_CLEAN_BOOT_FLAG:
			if (dir)
				clean_boot = f->state;
			else
				f->state = clean_boot;
			break;
		case CMOS_SYSFAULT_FLAG:
			if (dir)
				sysfault = f->state;
			else
				f->state = sysfault;
			break;
		case CMOS_NOLOGO_FLAG:
			if (dir)
				show_logo = f->state;
			else
				f->state = show_logo;
			break;
		case CMOS_UNIQUE_FLAG:
			if (dir)
				unique_root = f->state;
			else
				f->state = unique_root;
			break;
		default:
		}
		f++;
	}
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
