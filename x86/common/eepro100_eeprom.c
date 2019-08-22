/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* Intel EEpro100 EEPROM ruitines
 * Moshen Chan <mchan@cobalt.com>
 * June 02, 2000
 */

#include <asm/io.h>
#include "common/eepro100_eeprom.h"
#include "common/delay.h"


/*  EEPROM_Ctrl bit masks. */
#define EE_SHIFT_CLK    0x01    /* EEPROM shift clock. */
#define EE_CS           0x02    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x04    /* EEPROM chip data in. */
#define EE_DATA_READ    0x08    /* EEPROM chip data out. */
#define EE_ENB          (0x4800 | EE_CS)
#define EE_WRITE_0      0x4802
#define EE_WRITE_1      0x4806
#define EE_OFFSET       0x0E

/* these are the opcodes for the EEPROM, shifted by an amount to */
/* leave room for the address and data fields. */
#define EE_READ_CMD     (6 << 22)
#define EE_WRITE_CMD    (5 << 22)
#define EE_ENABLE_OPCODE (4 << 22)
#define EE_SIZE         64

#define eeprom_delay() udelay(500)

/* eeprom data - fixed values come from Intel spec */
static unsigned short eeprom_default_data[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101, 0x4701, 0x0000,
    0x3525, 0x0903, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};


static int do_eeprom_cmd(unsigned short ioaddr, int cmd, int cmd_len);
static unsigned short read_ee_word(unsigned short ioaddr, int offset);
static void write_ee_word(unsigned short ioaddr, int offset, unsigned short word);
static int update_checksum(unsigned short ioaddr);

static int do_eeprom_cmd(unsigned short ioaddr, int cmd, int cmd_len) {
    unsigned retval = 0;
    short dataval;
    int ee_addr = ioaddr + EE_OFFSET;
    
    outw(EE_ENB | EE_SHIFT_CLK, ee_addr);

    /* Shift the command bits out. */
    do {
       /*  write bit 1 or 0 to register? */
        if (cmd & (1 << cmd_len))
            dataval = EE_WRITE_1;
        else
            dataval = EE_WRITE_0;
        
        outw(dataval, ee_addr);
        eeprom_delay();
        /* shift clock */
        outw(dataval | EE_SHIFT_CLK, ee_addr);
        eeprom_delay();
        /* get data from register, take a 1 or 0 from EE_DATA_READ bit */
        retval = (retval << 1) | ((inw(ee_addr) & EE_DATA_READ) ? 1 : 0);
    } while (--cmd_len >= 0);
    outw(EE_ENB, ee_addr);
    
    /* Terminate the EEPROM access. */
    outw(EE_ENB & ~EE_CS, ee_addr);
    return retval;
}

static unsigned short read_ee_word(unsigned short ioaddr, int offset) {
    unsigned short value;
    
    value = do_eeprom_cmd(ioaddr, EE_READ_CMD | (offset << 16), 25);
    return value;
}

static void write_ee_word(unsigned short ioaddr, int offset, unsigned short word) {
    /* enable eeprom writes */
    do_eeprom_cmd(ioaddr, (EE_ENABLE_OPCODE | ((short)48 << 16)), 27);
    
    do_eeprom_cmd(ioaddr, (EE_WRITE_CMD | (offset << 16)) | word, 27);
    /* disable eeprom writes */
    do_eeprom_cmd(ioaddr, (EE_ENABLE_OPCODE | ((short)0 << 16)), 27);

}

static int update_checksum(unsigned short ioaddr) {
    int i;
    unsigned short sum = 0;
    unsigned short checksum = 0;

    for (i=0; i<63; i++) {
        sum += read_ee_word(ioaddr, i);
    }

    checksum = 0xBABA - sum;
    checksum &= 0xFFFF;

    write_ee_word(ioaddr, 63, checksum);

    /* verify checksum */
    sum = 0;
    for (i=0; i<EE_SIZE; i++) {
        sum += read_ee_word(ioaddr, i);
    }
    if (sum == 0xBABA)
        return 0;
    else
        return -1;
}



int eepro100_read_byte(unsigned short ioaddr, unsigned char *data, int offset) {
    unsigned short value;

    if ((int) (offset /2) > (EE_SIZE - 1))
        return -1;

    value = read_ee_word(ioaddr, (int) (offset/2));
    if ((offset % 2) == 0)
        *data = (unsigned char) (value & 0xff);
    else
        *data = (unsigned char) (value >> 8);
    
    return 0;
}

int eepro100_write_byte(unsigned short ioaddr, unsigned char data, int offset) {
    unsigned short value;
    int wordoffset;

    wordoffset = (int) offset/2;

    if (wordoffset > (EE_SIZE - 1))
        return -1;

    value = read_ee_word(ioaddr, wordoffset);
    if ((offset % 2) == 0) {
        write_ee_word(ioaddr, wordoffset, (value & 0xff00) | data);
        /* verify */
        if (read_ee_word(ioaddr, wordoffset) != ((value & 0xff00) | data))
            return -1;
    }
    else {
        write_ee_word(ioaddr, wordoffset, (value & 0xff) | (data << 8)); 
        if (read_ee_word(ioaddr, wordoffset) != ((value & 0xff) | (data << 8)))
            return -1;
    }
    return 0;
}


int eepro100_read_word(unsigned short ioaddr, unsigned short *data, int offset) {

    if (offset > (EE_SIZE - 1))
        return -1;
    *data = read_ee_word(ioaddr, offset);

    return 0;
}

int eepro100_write_word(unsigned short ioaddr, unsigned short data, int offset) {
    if (offset > (EE_SIZE -1))
        return -1;
    write_ee_word(ioaddr, offset, data);
    /* verify */
    if (read_ee_word(ioaddr, offset) != data)
        return -2;
    return 0;
}


int eepro100_write_eeprom(unsigned short ioaddr, unsigned short *eeprom_data) {
    int i;

    for (i=0; i<EE_SIZE; i++) {
        if (eepro100_write_word(ioaddr, eeprom_data[i], i) < 0)
            return -1;
    }
    return 0;
}



int eepro100_read_mac(unsigned short ioaddr, unsigned char *mac_data) {
    int i;
    unsigned char byte;
    
    for (i = 0; i<6; i++) {
        if (eepro100_read_byte(ioaddr, &byte, i) < 0)
            return -1;
        mac_data[i] = byte;
    }
    return 0;
}



int eepro100_write_mac(unsigned short ioaddr, unsigned char *mac) {
    /* first write with default values */
    eepro100_write_eeprom(ioaddr, eeprom_default_data);

    /* write MAC */
    write_ee_word(ioaddr, 0, (mac[1] << 8) | mac[0]);
    write_ee_word(ioaddr, 1, (mac[3] << 8) | mac[2]);
    write_ee_word(ioaddr, 2, (mac[5] << 8) | mac[4]);

    /* verify */
    if ( (read_ee_word(ioaddr, 0) != ((mac[1] << 8) | mac[0])) || \
         (read_ee_word(ioaddr, 1) != ((mac[3] << 8) | mac[2])) || \
         (read_ee_word(ioaddr, 2) != ((mac[5] << 8) | mac[4])))
        return -1;
    
    /* update checksum */
    if (update_checksum(ioaddr) != 0) {
        return -2;
    }
    return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
