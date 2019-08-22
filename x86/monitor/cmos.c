/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: cmos.c,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */

#include <printf.h>
#include <string.h>

#include "common/rom.h"
#include "common/cmos.h"
#include "common/rtc.h"

#include "cmos.h"

int read_cmos_func(int argc, char *argv[])
{
	unsigned char value;
	unsigned long index;

	if (argc != 2) return EBADNUMARGS;
	if (stoli(argv[1], &index) != 0) return EBADNUM;
	if (index > CMOS_INFO_MAX) return EBADARG;

	value = cmos_read_byte((int)index);

	printf("\n0x%02x: 0x%02x\n", (unsigned int)index, value);
    
	return 0;
}

int dump_cmos_func( int argc, char *argv[] __attribute__((unused)) )
{
    int i;

    if( argc != 1 )
	return EBADNUMARGS;

    printf( "    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f" );
    for( i=0 ; i<=CMOS_INFO_MAX ; i++ )
    {
	if( ! (i%0x10) )
	    printf( "\n%02x:", i );

	printf( " %02x", cmos_read_byte( i ) );
    }

    printf( "\n" );
    
    return 0;
}

int check_cmos_cks_func( int argc, char *argv[] __attribute__((unused)) )
{
    if( argc != 1 )
	return EBADNUMARGS;

    printf( "CMOS checksum %svalid\n", (cmos_check_checksum())?"":"not " );

    return 0;
}

int update_cmos_cks_func( int argc, char *argv[] __attribute__((unused)) )
{
    if( argc != 1 )
	return EBADNUMARGS;

    cmos_set_checksum();

    return 0;
}

int write_cmos_func(int argc, char *argv[])
{
	unsigned long index, value;
    
	if (argc != 3) return EBADNUMARGS;
	if (stoli(argv[1], &index) != 0) return EBADNUM;
	if (stoli(argv[2], &value) != 0) return EBADNUM;
	if (index > CMOS_INFO_MAX) return EBADARG;

	cmos_write_byte((int)index, (unsigned char)value);
	cmos_set_checksum();

	return 0;
}

int cmos_reset_func(int argc, char *argv[] __attribute__((unused)) )
{
	if (argc != 1)
		return EBADNUMARGS;

	cmos_first_boot_defaults_only( 0 );
	cmos_set_defaults();
	rtc_set_defaults();

	return 0;
}

int cmos_flags_func(int argc, char *argv[])
{
	struct cmos_flag *f;

	if (argc < 2) {
		cmos_flags_print();
		return 0;
	}
	f = cmos_flags_namefind(argv[1]);
	if (!f) {
		cmos_flags_print();
		return EBADARG;
	}

	switch (argc) {
	case 3:
		if (!strncmp(argv[2], "on", 2))
			f->state = 1;
		else if (!strncmp(argv[2], "off", 3))
			f->state = 0;
		else
			return EBADARG;

		cmos_write_flag(f->flag, f->state);
		break;

	case 2:
		printf("%s is %s (system state is %s)\n", f->name,
		       cmos_read_flag(f->flag) ? "on" : "off",
		       (f->state) ? "on" : "off");
		break;

	default:
		return EBADNUMARGS;
	}

	cmos_flags_sync(1);

	return 0;
}

int cmos_ver_func(int argc, char *argv[] __attribute__((unused)) )
{
	if (argc != 1)
		return EBADNUMARGS;

	printf("CMOS version is %d\n", cmos_version);

	return 0;
}

int bto_ip_func( int argc, char *argv[] )
{
    unsigned long val;
    unsigned int ip;

    switch( argc )
    {
	case 1:
	    printf( "%d.%d.%d.%d", 
		    (bto_ip >> 24) & 0xff,
		    (bto_ip >> 16) & 0xff,
		    (bto_ip >>  8) & 0xff,
		    (bto_ip >>  0) & 0xff );
	    break;

	case 5:
	    if (stoli(argv[1], &val) != 0) return EBADNUM;
	    ip = (val&0xff) << 24;

	    if (stoli(argv[2], &val) != 0) return EBADNUM;
	    ip |= (val&0xff) << 16;

	    if (stoli(argv[3], &val) != 0) return EBADNUM;
	    ip |= (val&0xff) << 8;

	    if (stoli(argv[4], &val) != 0) return EBADNUM;
	    ip |= val & 0xff;

	    cmos_set_bto_ip( ip );

	    break;

	default:
	    return EBADNUMARGS;
    }

    return 0;
    
}

int bto_code_func(int argc, char *argv[])
{
	unsigned long val;

	switch (argc) {
	case 2:
		if (stoli(argv[1], &val) != 0) return EBADNUM;
		cmos_write_byte(CMOS_BTO_CODE_0, (val >> 24) & 0xff);
		cmos_write_byte(CMOS_BTO_CODE_1, (val >> 16) & 0xff);
		cmos_write_byte(CMOS_BTO_CODE_2, (val >> 8) & 0xff);
		cmos_write_byte(CMOS_BTO_CODE_3, val & 0xff);
		break;
	case 1:
		printf("0x%x\n", 
			cmos_read_byte(CMOS_BTO_CODE_0) << 24 |
			cmos_read_byte(CMOS_BTO_CODE_1) << 16 |
			cmos_read_byte(CMOS_BTO_CODE_2) << 8 |
			cmos_read_byte(CMOS_BTO_CODE_3));
		break;
	default:
		return EBADNUMARGS;
	}

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
