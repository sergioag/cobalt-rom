/* $Id: apic.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */

#ifndef COMMMON_APIC_H
#define COMMMON_APIC_H

#include <extra_types.h>

#define LAPIC_BASE		0xFEE00000	/* Physical APIC Address Base	*/
#define LAPIC_ID		0x20    	/* ID Register			*/
#define LAPIC_VERSION		0x30		/* Version Register		*/
#define LAPIC_TPR		0x80		/* Task Priority Register	*/
#define LAPIC_LDR		0xD0		/* Logical Destination Register	*/
#define LAPIC_DFR		0xE0		/* Destination Format Register	*/
#define  LAPIC_DFR_FLAT		(~0UL)
#define LAPIC_EOI		0xB0		/* End of Interrupt		*/
#define  LAPIC_EOI_ACK		0x0		/*  write this to ack interrupt	*/
#define LAPIC_SVR		0xF0		/* Spurious Interrupt Vector	*/
#define  LAPIC_SVR_EN		0x100		/*  APIC enable bit		*/
#define  LAPIC_SVR_FP_DIS	0x200		/*  Focus Processor disable bit	*/
#define  LAPIC_SVR_VECTOR	0xFF		/*  Spurious APIC Vector	*/
#define LAPIC_ESR		0x280		/* Error Status Register	*/
#define  LAPIC_ESR_SEND_CS	0x1		/*  ERR: send checksum		*/
#define  LAPIC_ESR_RECV_CS	0x2		/*  ERR: receive checksum	*/
#define  LAPIC_ESR_SEND_ACC	0x4		/*  ERR: send accept		*/
#define  LAPIC_ESR_RECV_ACC	0x8		/*  ERR: receive accept		*/
#define  LAPIC_ESR_SEND_ILL	0x20		/*  ERR: send illegal vector	*/
#define  LAPIC_ESR_RECV_ILL	0x40		/*  ERR: receive illegal vector	*/
#define  LAPIC_ESR_ADDR_ILL	0x80		/*  ERR: illegal register addr	*/
#define LAPIC_ICRL		0x300		/* Interrupt Command Low bits	*/
#define LAPIC_ICRH		0x310		/* Interrupt Command High bits	*/
#define  LAPIC_ICR_BUSY		0x1000		/* set when interrupt busy	*/
#define  LAPIC_ICR_DEST_SELF	0x1		/*  Dest: current APIC		*/
#define  LAPIC_ICR_DEST_ALL	0x2		/*  Dest: all including self	*/
#define  LAPIC_ICR_DEST_OTHER	0x3		/*  Dest: all excluding self	*/
#define  LAPIC_ICR_MODE_FIXED	0x0		/*  Delivery Mode: Fixed	*/
#define  LAPIC_ICR_MODE_LOW	0x1		/*  Delivery Mode: Low Priority	*/
#define  LAPIC_ICR_MODE_SMI	0x2		/*  Delivery Mode: SMI		*/
#define  LAPIC_ICR_MODE_NMI	0x4		/*  Delivery Mode: NMI		*/
#define  LAPIC_ICR_MODE_INIT	0x5		/*  Delivery Mode: INIT		*/
#define  LAPIC_ICR_MODE_START	0x6		/*  Delivery Mode: Start-Up	*/
#define  LAPIC_ICR_LEVEL_DEASSERT 0x0		/*  Level: De-assert		*/
#define  LAPIC_ICR_LEVEL_ASSERT   0x1		/*  Level: Assert		*/
#define  LAPIC_ICR_TRIGGER_EDGE	  0x0		/*  Trigger Mode: Edge		*/
#define  LAPIC_ICR_TRIGGER_LEVEL  0x1		/*  Trigger Mode: Level		*/
#define LAPIC_TIMER		0x320		/* Local Vector Table (Timer)	*/
#define LAPIC_LVT0     		0x350		/* Local Vector Table (LINT0)	*/
#define LAPIC_LVT1     		0x360		/* Local Vector Table (LINT1)	*/
#define LAPIC_LVTERR   		0x370		/* Local Vector Table (Error)	*/
#define  LAPIC_LVT_NMI		0x400
#define  LAPIC_LVT_EXTINT	0x700
#define  LAPIC_LVT_LEVEL	(1<<15)		/* Local APIC LVT Trigger Mode	*/
#define  LAPIC_LVT_EDGE		~(1<<15)	/* Local APIC LVT Trigger Mode	*/
#define  LAPIC_LVT_MASK		(1<<16)		/* Local APIC LVT Mask bit	*/
#define  LAPIC_LVT_UNMASK	~(1<<16)	/* Local APIC LVT Un-Mask bit	*/

void lapic_init(void);
dword lapic_get_reg(dword);
void lapic_set_reg(dword, dword);
void lapic_enable(void);
void lapic_disable(void);
byte lapic_status(void);
byte lapic_get_bsp(void);
void lapic_set_id(byte);
byte lapic_get_id(void);
byte lapic_get_version(void);
dword lapic_get_base(void);
void lapic_eoi(void);
byte lapic_wait_for_idle(void);
void lapic_send_ipi(byte, byte, byte, byte, byte);
void lapic_send_ipi_startup(byte, byte);
void lapic_broadcast_ipi(byte, byte, byte, byte, byte);
void lapic_dump(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
