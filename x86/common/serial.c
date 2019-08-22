/* $Id: serial.c,v 1.1.1.1 2003/06/10 22:41:54 iceblink Exp $
 * Copyright 1997-2000 Cobalt Networks, Inc.
 * 
 * Functions for supporting and testing serial I/O
 * 
 * Author(s): Timothy Stonis
 *            Andrew Bose
 *            Erik Gilling
 *            Duncan Laurie
 */
#include <io.h>
#include <string.h>

#include "common/rom.h"
#include "common/serial.h"
#include "common/serial_reg.h"
#include "common/isapnp.h"
#include "common/systype.h"

static int serconsole;

void 
serial_init(int con_on)
{
	serconsole = con_on;
	serial_init_pnp();
	serial_init_16550(SERIAL_BASE_UART1);
}


/* serial_init_16550()
 *
 *  Description: Initialize all the 16550 registers for 115200 baud, 
 *  asynchronous, 8 bit, 1 stop bit, no parity communication (Channel A)
 *  Do the first one now, leave the rest for OS
 */
void 
serial_init_16550(int u)
{
	/* Set the divisor latch bit */
	uart_write_port(u, UART_LCR, UART_LCR_WLEN8 | UART_LCR_DLAB);

	/* 
	 * this is for selecting the baud based on the 
	 * prescaler select divisor 13  
	 */
	uart_write_port(u, UART_DLM, 0); /* Set divisor latch high byte */
	uart_write_port(u, UART_DLL, 1); /* Set divisor latch low byte  */

	/* Set the line control (clearing DLB) */
	uart_write_port(u, UART_LCR, UART_LCR_WLEN8);

	/* disable all interrupts */
	uart_write_port(u, UART_IER, 0);

	/* turn on RTS and DTR */
	uart_write_port(u, UART_MCR, UART_MCR_DTR | UART_MCR_RTS);

	/* turn off ISEN (interrupt signal enable) */
	uart_write_port(u, UART_MCR,
			uart_read_port(u, UART_MCR) & ~UART_MCR_ISEN);

	/* turn FIFOs on */
	uart_write_port(u, UART_FCR, 1);
}

void 
serial_init_pnp(void)
{
	switch (systype())
	{
	case SYSTYPE_3K:

		/* Enter Config Mode */
		outb(0x51, PNP_CONFIG_PORT);
		outb(0x23, PNP_CONFIG_PORT);
	
		/* enable Uart1 */
		outb(0x07, PNP_CONFIG_PORT); /* Point to Device Select Reg */
		outb(0x04, PNP_DATA_PORT);   /* logic device 4 (UART1)*/
	
		outb(0x30, PNP_CONFIG_PORT); /* Register 0x30 */
		outb(0x01, PNP_DATA_PORT);   /* enable UART1 */
	
		/* enable Uart2 */
		outb(0x07, PNP_CONFIG_PORT); /* Point to Device Select Reg */
		outb(0x05, PNP_DATA_PORT);   /* logic device 4 (UART2)*/
	
		outb(0x30, PNP_CONFIG_PORT); /* Register 0x30 */
		outb(0x01, PNP_DATA_PORT);   /* enable UART2 */
	
		/* Exit config mode */
		outb(0xBB, PNP_CONFIG_PORT);

		break;

	case SYSTYPE_5K:
		/* this is done early in rom init by romcode/reset5k.S
		 * to avoid garbage characters on the console */
		break;
	default:
	}
}

/*
 *   Write a byte to the SCC (Channel A) and blink LED
 */
void 
serial_outb(char outbyte)
{
	if (!serconsole) return;
	while ((serial_read_port(UART_LSR) & UART_LSR_TXRDY) == 0);
	serial_write_port(UART_TX, outbyte);
}

/*
 *   Read a byte from the SCC (Channel A)
 */
void 
serial_inb(unsigned char *inbyte)
{
	if (!serconsole) return;
	while ((serial_read_port(UART_LSR) & UART_LSR_DR) == 0);
	*inbyte = serial_read_port(UART_RX);
}

/*
 *   Read a byte from the SCC (Channel A) with timeout
 */
unsigned char 
serial_inb_timeout(unsigned char *inbyte, unsigned long timeout)
{
        unsigned char lsr, ier;

	if (!serconsole) return 0xff;

	/*   read IER and save state,
	 *   then disable interrupts to be sure some other driver
	 *   doesn't handle our data
	 */
	ier = serial_read_port(UART_IER);
	serial_write_port(UART_IER, 0x00);

	/* repeat until data available */
	do {
		/* watch for timeout */
                if (--timeout == 0)
			return 0xff;

		/* query line status register */
		lsr = serial_read_port(UART_LSR);

		/* trap overflow and framing errors */
		if (lsr & (UART_LSR_OE | UART_LSR_FE))
			continue;

	} while (!(lsr & UART_LSR_DR));

	/* read and save byte */
	*inbyte = serial_read_port(UART_RX);

	/* restore saved interrupts */
	serial_write_port(UART_IER, ier);

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
