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
#include "common/isapnp.h"

#ifndef INROM
# include <sys/io.h>
#else 
# include <io.h>
# include "common/rom.h"
# include "common/systype.h"
#endif /* INROM */

#define APCR6_WAKE_VAL  0x80
#define APCR6_WAKE_MASK 0xc0

int cmos_get_powerfail( void )
{
	int bank, apcr6;

	if( ! systype_is_5k() )
	    return 0;

	/* switch to bank 2 */
	outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	bank = inb(CMOS_DATA_PORT);
	outb((bank & 0x8f) | PNP_NS_RTC_BANK_2, CMOS_DATA_PORT);

	/* get moap bit */
	outb(PNP_NS_APCR6, CMOS_ADDR_PORT);
	apcr6 = inb(CMOS_DATA_PORT);

	/* switch banks back */
	outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	outb(bank, CMOS_DATA_PORT);

	return  (apcr6 & APCR6_WAKE_MASK) != APCR6_WAKE_VAL;
}

void cmos_set_powerfail( int state )
{
	int bank, apcr6;

	if( ! systype_is_5k() )
	    return;

	/* switch to bank 2 */
	outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	bank = inb(CMOS_DATA_PORT);
	outb((bank & 0x8f) | PNP_NS_RTC_BANK_2, CMOS_DATA_PORT);

	/* get moap bit */
	outb(PNP_NS_APCR6, CMOS_ADDR_PORT);
	apcr6 = inb(CMOS_DATA_PORT);

	apcr6 &= ~APCR6_WAKE_MASK;
	
	if( ! state )
	    apcr6 |= APCR6_WAKE_VAL;

	outb( apcr6, CMOS_DATA_PORT );

	/* switch banks back */
	outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	outb(bank, CMOS_DATA_PORT);

}

#define NS_GP1_EN0        0x04
#define NS_GP1_EN0_PME2_E 0x02
int cmos_get_powerapply( void )
{
    if( ! systype_is_5k() )
	return 0;
    return inb( NATSEMI_GPE1_PORT | NS_GP1_EN0 ) & NS_GP1_EN0_PME2_E;
}

void cmos_set_powerapply( int state )
{
    unsigned char reg;

    if( ! systype_is_5k() )
	return;

    reg = inb( NATSEMI_GPE1_PORT | NS_GP1_EN0 );
    reg &= ~NS_GP1_EN0_PME2_E;

    if( state )
	reg |= NS_GP1_EN0_PME2_E;

    outb( reg, NATSEMI_GPE1_PORT | NS_GP1_EN0 );
}

unsigned char 
cmos_read_byte(int index)
{
	if (index > CMOS_INFO_MAX) {
		return 0;
	}

	    /* force bank 0 */
	if( boardtype() == BOARDTYPE_MONTEREY )
	{
	    unsigned char bank;
	    
	    outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	    bank = inb(CMOS_DATA_PORT);
	    outb((bank & 0x8f) | PNP_NS_RTC_BANK_0, CMOS_DATA_PORT);
	}

	outb(index, CMOS_ADDR_PORT);
	return inb(CMOS_DATA_PORT);
}

int 
cmos_write_byte(int index, unsigned char val)
{
	if (index > CMOS_INFO_MAX) {
		return -1;
	}

	    /* force bank 0 */
	if( boardtype() == BOARDTYPE_MONTEREY )
	{
	    unsigned char bank;
	    
	    outb(PNP_NS_RTC_CRA, CMOS_ADDR_PORT);
	    bank = inb(CMOS_DATA_PORT);
	    outb((bank & 0x8f) | PNP_NS_RTC_BANK_0, CMOS_DATA_PORT);
	}
	
	outb(index, CMOS_ADDR_PORT);
	outb(val, CMOS_DATA_PORT);

	return 0;
}

int 
cmos_read_all_flags(void)
{
	int cmos_flags = 0; /* 2 bytes of flags for now */
	char *p = (char *)&cmos_flags;

	/* being little endian, we address the LSB first */
	*p++ = cmos_read_byte(CMOS_FLAG_BYTE_1);
	*p++ = cmos_read_byte(CMOS_FLAG_BYTE_0);

	return cmos_flags;
}

void 
cmos_write_all_flags(int cmos_flags)
{
	char *p = (char *)&cmos_flags;

	/* being little endian, we address the LSB first */
	cmos_write_byte(CMOS_FLAG_BYTE_1, *p++);
	cmos_write_byte(CMOS_FLAG_BYTE_0, *p++);

	cmos_set_checksum();
}

int 
cmos_read_flag(int flag)
{
	int cmos_flags;

	if (flag < CMOS_FLAG_MIN || flag > CMOS_FLAG_MAX) {
		return -1;
	}

	cmos_flags = cmos_read_all_flags();

	if (cmos_flags & flag) {
		return 1;
	} else {
		return 0;
	}
}

int 
cmos_write_flag(int flag, int val)
{
	int cmos_flags;

	if (flag < CMOS_FLAG_MIN || flag > CMOS_FLAG_MAX) {
		return -1;
	}

	cmos_flags = cmos_read_all_flags();

	if (val) {
		cmos_flags |= flag;
	} else {
		cmos_flags &= (~flag);
	}

	cmos_write_all_flags(cmos_flags);

	return 0;
}

int cmos_check_checksum( void )
{
    int i;
    unsigned short sum = 0;
    
    for( i = CMOS_CKS_START; i <= CMOS_CKS_END; ++i )
    {
	if( (i == CMOS_CHECKSUM) || 
	    (i == (CMOS_CHECKSUM+1)) )
	    continue;

        sum += cmos_read_byte( i );
    }
    return( (sum & 0xffff) ==
            ((cmos_read_byte(CMOS_CHECKSUM) << 8) |
             cmos_read_byte(CMOS_CHECKSUM+1)) );
}

void cmos_set_checksum( void )
{
    int i;
    unsigned short sum = 0;
    
    for( i = CMOS_CKS_START; i <= CMOS_CKS_END; ++i )
    {
	if( (i == CMOS_CHECKSUM) || 
	    (i == (CMOS_CHECKSUM+1)) )
	    continue;

        sum += cmos_read_byte( i );
    }
    
    cmos_write_byte( CMOS_CHECKSUM, sum >> 8 );
    cmos_write_byte( CMOS_CHECKSUM+1, sum & 0xff );
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
