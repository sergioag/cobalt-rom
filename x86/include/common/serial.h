/* $Id: serial.h,v 1.1.1.1 2003/06/10 22:42:04 iceblink Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 * 
 * general definitions used for serial code 
 */

#ifndef COMMON_SERIAL_H
#define COMMON_SERIAL_H
 
/* Serial port definitions */
#define SERIAL_BASE_UART1     0x3f8
#define SERIAL_BASE_UART2     0x2f8

/* These are the supported serial types */
#define PORT_UNKNOWN    0
#define PORT_8250       1
#define PORT_16450      2
#define PORT_16550      3
#define PORT_16550A     4
#define PORT_CIRRUS     5
#define PORT_16650      6
#define PORT_16650V2    7
#define PORT_16750      8
#define PORT_STARTECH   9
#define PORT_MAX        9

/* low-level IO macros */

/* IO to/from specific UART */
#define uart_read_port(u,p)     ((unsigned char)(inb_p(u + p)))
#define uart_write_port(u,p,v)  (outb_p(v, (u + p)))

/* IO to/from UART 1 */
#define serial_read_port(p)     ((unsigned char)(inb_p(SERIAL_BASE_UART1 + (p))))
#define serial_write_port(p,v)  (outb_p((v), (SERIAL_BASE_UART1 + (p))))

/* Function prototypes */
void serial_init(int con_on);
void serial_init_pnp(void);
void serial_init_16550(int u);
void serial_inb(unsigned char *inbyte);
void serial_outb(char outbyte);
unsigned char serial_inb_timeout(unsigned char *inbyte, unsigned long timeout);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
