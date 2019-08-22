/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* National Semiconductor dp83815 (MacPhyter) eeprom ruitines
 * Moshen Chan <mchan@cobalt.com>
 * June 02, 2000
 */

#include <asm/io.h>
#include "common/macphyter_eeprom.h"
#include "common/delay.h"

//  EEPROM_Ctrl bit masks. 
#define EE_SHIFT_CLK_M    0x04    /* EEPROM shift clock. */
#define EE_CS_M           0x08    /* EEPROM chip select. */
#define EE_DATA_WRITE_M   0x01    /* EEPROM chip data in. */
#define EE_DATA_READ_M    0x02    /* EEPROM chip data out. */
#define EE_ENB_M          (0x4800 | EE_CS_M)
#define EE_WRITE_0_M      0x4808
#define EE_WRITE_1_M      0x4809
#define EE_OFFSET_M       0x08

// these are the opcodes for the EEPROM, shifted by an amount to
// leave room for the address and data fields.
#define EE_READ_CMD_M     (6 << 22)
#define EE_WRITE_CMD_M    (5 << 22)
#define EE_ENABLE_OPCODE_M (4 << 22)

#define EE_SIZE         12

#define eeprom_delay() udelay(1500)

#define SWAP_BITS(x)    ( (((x) & 0x01) << 7) \
                                | (((x) & 0x02) << 5) \
                                | (((x) & 0x04) << 3) \
                                | (((x) & 0x08) << 1) \
                                | (((x) & 0x10) >> 1) \
                                | (((x) & 0x20) >> 3) \
                                | (((x) & 0x40) >> 5) \
                                | (((x) & 0x80) >> 7))



/* eeprom data - fixed values come from National Semiconductor spec with
 * the following exception: Wake-On-Lan should only be enabled for
 * magic packets.
 */
unsigned short macphyter_default_data[] = {
    0xd008, 0x0400, 0x2cd0, 0xdf82,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, /*0xa098 */0x0098, 0x0055 
};


static int do_eeprom_cmd_m(unsigned short ioaddr, int cmd, int cmd_len);
static unsigned short read_ee_word_m(unsigned short ioaddr, int offset);
static void write_ee_word_m(unsigned short ioaddr, int offset, unsigned short word);
static int update_checksum(unsigned short ioaddr);

static int do_eeprom_cmd_m(unsigned short ioaddr, int cmd, int cmd_len) {
    unsigned retval = 0;
    short dataval;
    int ee_addr = ioaddr + EE_OFFSET_M;
    
    outw(EE_ENB_M | EE_SHIFT_CLK_M, ee_addr);

    /* Shift the command bits out. */
    do {
       /*  write bit 1 or 0 to register? */
	if (cmd & (1 << cmd_len))
	    dataval = EE_WRITE_1_M;
	else
	    dataval = EE_WRITE_0_M;
	
	outw(dataval, ee_addr);
	eeprom_delay();
	/* shift clock */
	outw(dataval | EE_SHIFT_CLK_M, ee_addr);
	eeprom_delay();
	/* get data from register, take a 1 or 0 from EE_DATA_READ bit */
	retval = (retval << 1) | ((inw(ee_addr) & EE_DATA_READ_M) ? 1 : 0);
    } while (--cmd_len >= 0);
    outw(EE_ENB_M, ee_addr);
    
    /* Terminate the EEPROM access. */
    outw(EE_ENB_M & ~EE_CS_M, ee_addr);
    return retval;
}

/* read 16 bit word at offset */
static unsigned short read_ee_word_m(unsigned short ioaddr, int offset) {
    unsigned short value;
    
    value = do_eeprom_cmd_m(ioaddr, EE_READ_CMD_M | (offset << 16), 25);
    return value;
}
    
/* write 16 bit word at offset */
static void write_ee_word_m(unsigned short ioaddr, int offset, unsigned short word) {
    /* enable eeprom writes */
    do_eeprom_cmd_m(ioaddr, (EE_ENABLE_OPCODE_M | ((short)48 << 16)), 27);
    
    do_eeprom_cmd_m(ioaddr, (EE_WRITE_CMD_M | (offset << 16)) | word, 27);
    /* disable eeprom writes */
    do_eeprom_cmd_m(ioaddr, (EE_ENABLE_OPCODE_M | ((short)0 << 16)), 27);

}

static int update_checksum(unsigned short ioaddr) {
    int i;
    unsigned char  sum = 0x55;
    unsigned char  temp;


    for (i=0; i<22; i++) {
	macphyter_read_byte(ioaddr, &temp, i);
	sum += temp;
    }

    sum = 0-sum;
    
    if (macphyter_write_byte(ioaddr, sum, 23) == 0)
        return 0;
    else 
	return -1;
}

int macphyter_read_byte(unsigned short ioaddr, unsigned char *data, int offset) {
    unsigned short value;

    if ((int) (offset /2) > (EE_SIZE - 1))
	return -1;

    value = read_ee_word_m(ioaddr, (int) (offset/2));
    if ((offset % 2) == 0)
	*data = (unsigned char) (value & 0xff);
    else
	*data = (unsigned char) (value >> 8);
    
    return 0;
}

int macphyter_write_byte(unsigned short ioaddr, unsigned char data, int offset) {
    unsigned short value;
    int wordoffset;

    wordoffset = (int) offset/2;

    if (wordoffset > (EE_SIZE - 1))
	return -1;

    value = read_ee_word_m(ioaddr, wordoffset);
    if ((offset % 2) == 0) {
	write_ee_word_m(ioaddr, wordoffset, (value & 0xff00) | data);
	/* verify */
	if (read_ee_word_m(ioaddr, wordoffset) != ((value & 0xff00) | data))
	    return -1;
    }
    else {
	write_ee_word_m(ioaddr, wordoffset, (value & 0xff) | (data << 8)); 
	if (read_ee_word_m(ioaddr, wordoffset) != ((value & 0xff) | (data << 8)))
	    return -1;
    }
    return 0;
}

int macphyter_read_word(unsigned short ioaddr, unsigned short *data, int offset) {
    
    if (offset > (EE_SIZE - 1))
	return -1;
    *data = read_ee_word_m(ioaddr, offset);
    
    return 0;
}

int macphyter_write_word(unsigned short ioaddr, unsigned short data, int offset) {
    if (offset > (EE_SIZE -1))
	return -1;
    write_ee_word_m(ioaddr, offset, data);
    /* verify */
    if (read_ee_word_m(ioaddr, offset) != data) 
	return -2;
    return 0;
}

int macphyter_write_eeprom(unsigned short ioaddr, unsigned short *eeprom_data) {
    int i;

    for (i=0; i<EE_SIZE; i++) {
	if (macphyter_write_word(ioaddr, eeprom_data[i], i) < 0)
	    return -1;
    }
    return 0;
}

int macphyter_read_mac(unsigned short ioaddr, unsigned char *mac_data) {
    int i;
    unsigned short word1, word2, word3;
    unsigned char byte;

    /* get words and shift it right 1 bit */
    macphyter_read_word(ioaddr, &word3, 9);
    macphyter_read_word(ioaddr, &word2, 8);
    macphyter_read_word(ioaddr, &word1, 7);
    macphyter_read_byte(ioaddr, &byte, 12);

    word3 >>= 1;
    word3 = ((word2 & 0x0001) << 15) | word3;
    
    word2 >>= 1;
    word2 = ((word1 & 0x0001) << 15) | word2;

    word1 >>=1;
    word1 = ((byte & 0x01) << 15) | word1;

    /* retrieve the bytes */
    mac_data[0] = (unsigned char)((word1 & 0xff00) >> 8);
    mac_data[1] = (unsigned char)(word1 & 0x00ff);
    mac_data[2] = (unsigned char)((word2 & 0xff00) >> 8);
    mac_data[3] = (unsigned char)(word2 & 0x00ff);
    mac_data[4] = (unsigned char)((word3 & 0xff00) >> 8);
    mac_data[5] = (unsigned char)(word3 & 0x00ff);
            
    /* swap bits */
    for (i = 0; i<6; i++) { 
	mac_data[i] = SWAP_BITS(mac_data[i]); 
    } 
    return 0;
}

int macphyter_write_mac(unsigned short ioaddr, unsigned char *mac) {
    int i;
    int hibit = 0;
    unsigned char value, value2;
    unsigned char mac_s[7];
    
    /* first write with default values */
    macphyter_write_eeprom(ioaddr, macphyter_default_data);

    mac_s[6] = 0;
    /* swap bits */
    for (i=0; i<6; i++) { 
        mac_s[i] = SWAP_BITS(mac[i]); 
    } 
    mac_s[6] = 0;

    /* shift it left 1 bit */
    hibit = (mac_s[0] & 0x80) >> 7;
    for (i=0; i<6; i++) {
        mac_s[i] <<= 1;
        mac_s[i] |= ((mac_s[i+1] & 0x80) >> 7);
    }
    mac_s[5] &= 0xfe;

    macphyter_read_byte(ioaddr, &value, 12);
    macphyter_read_byte(ioaddr, &value2, 18);

    /* write MAC */
    macphyter_write_byte(ioaddr, value | (hibit & 0x01), 12);
    macphyter_write_byte(ioaddr, mac_s[0], 15);
    macphyter_write_byte(ioaddr, mac_s[1], 14);
    macphyter_write_byte(ioaddr, mac_s[2], 17);
    macphyter_write_byte(ioaddr, mac_s[3], 16);
    macphyter_write_byte(ioaddr, mac_s[4], 19);
    macphyter_write_byte(ioaddr, (mac_s[5] & 0xfe) | value2, 18);

    /* verify - just check middle two words */
    if ( (read_ee_word_m(ioaddr, 7) != ((mac_s[0] << 8) | mac_s[1])) || \
         (read_ee_word_m(ioaddr, 8) != ((mac_s[2] << 8) | mac_s[3])))
        return -1;
    
    /* update checksum */ 
    if (update_checksum(ioaddr) != 0)  
        return -2; 
    
    return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
