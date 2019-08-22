/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: dallas.c,v 1.1.1.1 2003/06/10 22:41:41 iceblink Exp $
 *
 * changed to get rid of pci initialization dependency -- asun
 */

#include <io.h>
#include <printf.h>
#include "extra_types.h"
#include "common/systype.h"
#include "common/pci.h"
#include "common/delay.h"
#include "common/isapnp.h"
#include "common/id_eeprom.h"

static unsigned char ssn[8];
static unsigned char ssn_string[17];

enum {
  GPIO_TYPE_UNKNOWN = 0,
  GPIO_TYPE_PCI,
  GPIO_TYPE_IO
};

static struct ssn_gpio { 
    int systype;
    byte devfn;
    byte type, mask;
    word direction_addr, input_addr, output_addr;
} gpios[] = {
  { SYSTYPE_3000, PCI_DEVFN(0x3, 0x0), GPIO_TYPE_PCI, 0x08, 0x7d, 0x7f, 0x7e},
  { SYSTYPE_5000, PCI_DEVFN(0xf, 0x0), GPIO_TYPE_IO, 0x01, 0x01, 0x0, 0x0},
  { SYSTYPE_UNKNOWN, 0, GPIO_TYPE_UNKNOWN, 0, 0, 0, 0}
};

static struct ssn_gpio *gpio = NULL;

static int io_init(void) {
    byte data;
    word port;
    
    switch (gpio->type) {
    case GPIO_TYPE_PCI:
      /* Set input mode on GPIO */
      data = pci_read_config_byte(0x0, gpio->devfn, gpio->direction_addr);
      if (data & gpio->mask)
	pci_write_config_byte(0x0, gpio->devfn, gpio->direction_addr, 
				  data & ~gpio->mask);

      /* Set the output value to be 0 */
      data = pci_read_config_byte(0x0, gpio->devfn, gpio->output_addr); 

      if (data & gpio->mask)
	pci_write_config_byte(0x0, gpio->devfn, gpio->output_addr, 
			      data & ~gpio->mask);

      break;
    case GPIO_TYPE_IO:
      /* find out gpio base address */
      outb(PNP_NS_LOGICAL_DEV, PNP_NS_INDEX_PORT);
      outb(PNP_NS_DEV_RAQXTR_GPIO, PNP_NS_DATA_PORT);
      outb(PNP_NS_ADDR_HIGH, PNP_NS_INDEX_PORT);
      port = inb(PNP_NS_DATA_PORT) << 8;
      outb(PNP_NS_ADDR_LOW, PNP_NS_INDEX_PORT);
      port |= inb(PNP_NS_DATA_PORT);
      if (port) {
  	  byte val;

	  gpio->direction_addr += port;
	  gpio->input_addr += port;
	  gpio->output_addr += port;

	  /* set output value to 0 */
	  val = inb(gpio->direction_addr);
	  outb(val | gpio->mask, gpio->direction_addr);
	  data = inb(gpio->output_addr);
	  if (data & gpio->mask)
	    outb(data & ~gpio->mask, gpio->output_addr);
	  
	  /* now set to input mode */
	  outb(val & ~gpio->mask, gpio->direction_addr);
      }
      break;
    default:
      return -1;
    }

    /* Let things calm down */
    udelay(500);
    return 0;
}

static void io_write(int delay) {
    unsigned char data;

    switch (gpio->type) {
    case GPIO_TYPE_PCI:
      /* Set output mode on GPIO3 */
      data = pci_read_config_byte(0x0, gpio->devfn, gpio->direction_addr);
      pci_write_config_byte(0x0, gpio->devfn, gpio->direction_addr, 
				data | gpio->mask);
      udelay(delay);

      /* Set input mode */
      pci_write_config_byte(0x0, gpio->devfn, gpio->direction_addr, 
				data & ~gpio->mask);
      break;
    case GPIO_TYPE_IO:
      data = inb(gpio->direction_addr);
      outb(data | gpio->mask, gpio->direction_addr);
      udelay(delay);
      outb(data & ~gpio->mask, gpio->direction_addr);
      break;
    default:
      return;
    }
}

static int io_read(void) {
    unsigned char data = 0;
    
    /* Get the input value */
    switch (gpio->type) {
    case GPIO_TYPE_PCI:
        data = pci_read_config_byte(0x0, gpio->devfn, gpio->input_addr);
	break;
    case GPIO_TYPE_IO:
        data = inb(gpio->input_addr);
	break;
    default:
	return 0;
        break;
    }

    if (data & gpio->mask)
        return 1;

    return 0;
}

static void io_write_byte(unsigned char c) {
    int i;
    
    for (i = 0; i < 8; i++, c >>= 1) {
	if (c & 1) {
	    /* Transmit a 1 */
            io_write(5);
            udelay(80);
        } else {
            /* Transmit a 0 */
            io_write(80);
            udelay(10);
        }
    }
}


static unsigned char io_read_byte(void)
{
    int i;
    unsigned char c = 0;
    
    for (i = 0; i < 8; i++) {
	io_write(1);    /* Start the read */
        udelay(2);
        if (io_read()) {
            c |= 1 << i;
        }
        udelay(60);
    }

    return c;
}

unsigned char* get_ssn(void)
{
    int i;
    
    i = systype();
    if ((i == SYSTYPE_5K) && (boardtype() == BOARDTYPE_ALPINE)) {
	id_eeprom_read_serial_num( ssn );
    }
    else
    {
	gpio = gpios;
	while (gpio && (gpio->systype != SYSTYPE_UNKNOWN)) {
	    if (i == gpio->systype)
		break;
	    ++gpio;
	}
	
	if (gpio->systype == SYSTYPE_UNKNOWN) 
	    return NULL;
	
	io_init();
	
	    /* Master Reset Pulse */
	for (i = 0; i < 600; i += 30) {
	    if (io_read())
		break;
	}
	
	if (i >= 600) 
	    return NULL;
	
	io_write(600);
	
	for (i = 0; i < 300; i += 15) {
	    udelay(15);
	    if (io_read() == 0) {
            /* We got a presence pulse */
		udelay(600);        /* Wait for things to quieten down */
		break;
	    }
	}
	
	if (i >= 300) 
	    return NULL;
	
	io_write_byte(0x33);
	
	for (i = 0; i < 8; i++) {
	    ssn[i] = io_read_byte();
	}
    }
    
    return (unsigned char *)ssn;
}
    
unsigned char* get_ssn_hex(void) {
    int i;

    get_ssn();

    /* convert to HEX */
    for (i = 7; i >= 0; i--) {
	sprintf(ssn_string + (7 - i) * 2, "%02x\n", ssn[i]);
    }
    ssn_string[16] = (unsigned char) '\0';
    return (unsigned char *)ssn_string;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
