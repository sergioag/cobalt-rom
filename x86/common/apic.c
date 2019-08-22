/*
 * $Id: apic.c,v 1.1.1.1 2003/06/10 22:41:39 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * support for the APIC on MP systems
 *
 *   Erik Gilling
 *   Duncan Laurie
 */

#include <io.h>
#include <printf.h>

#include "common/memory.h"
#include "common/alloc.h"
#include "common/apic.h"
#include "common/systype.h"
#include "common/delay.h"
#include "common/msr.h"
#include "common/rom.h"
#include "common/cmos.h"

/********************************************************************/
static volatile unsigned char *lapic = (unsigned char *) LAPIC_BASE;
static const char * ipi_modes[7] = {
	"Unknown", "Fixed", "LowPri", "SMI", "NMI", "INIT", "Start-Up"
};

#define lapic_get(off)\
        ((*((volatile unsigned int *) (lapic + (off)))))
#define lapic_set(off,val)\
        ((*((volatile unsigned int *) (lapic + (off)))) = (val))
/********************************************************************/

/*
 * initialize Local APIC
 *
 *   enable Virtual Wire compatibility mode,
 *   set Spurious Interrupt Vector,
 *   set Local INT0 as ExtINT,
 *   set Local INT1 as NMI
 */
void
lapic_init (void)
{
	unsigned int reg;

	if (systype_is_3k())
		return;

	lapic_enable();

	DPRINTF("Initializing Local APIC ID %d at 0x%08x (Virtual Wire mode)\n",
		lapic_get_id(), (dword) lapic);
	
	/* set LVT masks only first to deassert level-triggered sources */
	reg = lapic_get(LAPIC_LVT0);
	lapic_set(LAPIC_LVT0, reg | LAPIC_LVT_MASK);
	reg = lapic_get(LAPIC_LVT1);
	lapic_set(LAPIC_LVT1, reg | LAPIC_LVT_MASK);
	reg = lapic_get(LAPIC_TIMER);
	lapic_set(LAPIC_TIMER, reg | LAPIC_LVT_MASK);

	/* put apic in flat delivery mode, and accept all */
	lapic_set(LAPIC_DFR, LAPIC_DFR_FLAT);
	lapic_set(LAPIC_LDR, 0);
	lapic_set(LAPIC_TPR, 0);

	/* the APIC spurious interrupt must point to a vector whose lower
	 * nibble is 1111 (0xF).  upper nibble is ?F, where ? is 0-F.
	 * we use Int 0FFh, which must handle spurious interrupts and
	 * supply the necessary IRET.  this assumes the vector has already
	 * been initialized in memory.
	 *
	 * enable the APIC in SVR (bit 8)
	 * and set the spurious interrupt vector
	 */
	lapic_set(LAPIC_SVR, LAPIC_SVR_EN | LAPIC_SVR_VECTOR);

	/* program LVT0 as ExtInt, which delivers the signal to the INTR signal
	 * of all processors' cores listed in the destination as an interrupt that
	 * originated in an externally-connected interrupt controller.
	 */
	lapic_set(LAPIC_LVT0, LAPIC_LVT_EXTINT);

	/* program LVT1 as NMI */
	lapic_set(LAPIC_LVT1, LAPIC_LVT_NMI );

	/* mask and clear the timer */
	lapic_set(LAPIC_TIMER, LAPIC_LVT_MASK);

	/* mask and clear LVT2 / LVTERR */
	lapic_set(LAPIC_LVTERR, LAPIC_LVT_MASK);

	lapic_set(LAPIC_ESR, 0);
	udelay(100);
}

/*
 * send and end of interrupt signal to the local apic
 */
void
lapic_eoi(void)
{
	lapic_set(LAPIC_EOI, LAPIC_EOI_ACK);
}

/*
 * sends an interpocessor interrupt to a specific APIC ID
 *
 * dest: physical ID of destination APIC
 *
 * mode: specifies how the destination APICs should act upon receipt of interrupt
 *   0 - Fixed
 *   1 - Lowest Priority
 *   2 - SMI
 *   4 - NMI
 *   5 - INIT
 *   6 - Start Up
 *
 * vector: vector of interrupt being sent
 *
 * trigger: used for INIT Level de-assert delivery mode only
 *   0 - Edge
 *   1 - Level
 *
 * polarity: for INIT level de-assert delivery mode the level is 0, otherwise 1
 *   0 - de-assert
 *   1 - assert
 */
void
lapic_send_ipi (byte dest, byte mode, byte vector, byte trigger, byte polarity)
{
	dword low, hi;

	DPRINTF("Sending %s IPI (%s/%s) to APIC %d [vector 0x%02x]\n",
		ipi_modes[mode], (trigger) ? "Level" : "Edge" ,
		(polarity) ? "Assert" : "De-Assert", dest, vector);

	/* clear error register */
	lapic_set(LAPIC_ESR, 0);

	hi  = lapic_get(LAPIC_ICRH);
	hi &= 0xF0FFFFFF;
	hi |= ((dest & 0xF) << 24);

	low  = vector | ((mode & 0x7) << 8);
	low |= (polarity) ? (1<<14) : (trigger) ? (3<<14) : (1<<14);

	lapic_set(LAPIC_ICRH, hi);
	lapic_set(LAPIC_ICRL, low);
}

/*
 * send StartUp IPI with vector to destination APIC
 */
void
lapic_send_ipi_startup (byte dest, byte vector)
{
	lapic_send_ipi (dest, LAPIC_ICR_MODE_START, vector,
			LAPIC_ICR_TRIGGER_EDGE, LAPIC_ICR_LEVEL_ASSERT);
}

/*
 * broadcasts an interpocessor interrupt
 *
 * dest: where to send the interrupt
 *   1 - self
 *   2 - all including self
 *   3 - all excluding self
 *
 * mode: specifies how the destination APICs should act upon receipt of interrupt
 *   0 - Fixed
 *   1 - Lowest Priority
 *   2 - SMI
 *   4 - NMI
 *   5 - INIT
 *   6 - Start Up
 *
 * vector: vector of interrupt being sent
 *
 * trigger: used for INIT Level de-assert delivery mode only
 *   0 - Edge
 *   1 - Level
 *
 * polarity: for INIT level de-assert delivery mode the level is 0, otherwise 1
 *   0 - de-assert
 *   1 - assert
 */
void
lapic_broadcast_ipi (byte dest, byte mode, byte vector, byte trigger, byte polarity)
{
	dword low, hi;

	if (dest < 1 || dest > 3) return;

	DPRINTF("Broadcasting %s IPI (%s/%s) to %s [vector 0x%02x]\n",
		ipi_modes[mode], (trigger) ? "Level" : "Edge",
		(polarity) ? "Assert" : "De-Assert",
		(dest==1) ? "self" : (dest==2) ? "all CPUs" :
		(dest==3) ? "all but self" : "unknown", vector);

	/* clear error register */
	lapic_set(LAPIC_ESR, 0);

	hi  = lapic_get(LAPIC_ICRH);
	hi &= 0xF0FFFFFF;

	low  = vector | ((dest & 0x3) << 18) | ((mode & 0x7) << 8);
	low |= (polarity) ? (1<<14) : (trigger) ? (3<<14) : (1<<14);

	lapic_set(LAPIC_ICRH, hi);
	lapic_set(LAPIC_ICRL, low);
}

/*
 * wait for Local APIC delivery status idle
 */
byte
lapic_wait_for_idle(void)
{
	byte status, timeout=0;

	do {
		udelay(100);
		status = (byte)(lapic_get(LAPIC_ICRL) & LAPIC_ICR_BUSY);
	} while (status && (timeout++ < 1000));

	return status;
}

/*
 * get local apic register
 */
dword
lapic_get_reg (dword reg)
{
	return lapic_get(reg);
}

/*
 * set local apic register
 */
void
lapic_set_reg (dword reg, dword val)
{
	lapic_set(reg, val);
}

/*
 * return Local APIC ESR
 */
byte
lapic_status (void)
{
	byte stat = lapic_get(LAPIC_ESR) & 0xff;
	lapic_set(LAPIC_ESR, 0);
	return stat;
}

/*
 * enable Local APIC in MSR
 */
void
lapic_enable (void)
{
	msr_set_on(MSR_APIC, MSR_APIC_EN);
}

/*
 * enable Local APIC in MSR
 */
void
lapic_disable (void)
{
	msr_set_off(MSR_APIC, MSR_APIC_EN);
}

/*
 * set Local APIC ID
 */
void
lapic_set_id (byte id)
{
	lapic_set(LAPIC_ID, ((id & 0xf) << 24));
}

/*
 * return Local APIC ID
 */
byte
lapic_get_id (void)
{
	return ((lapic_get(LAPIC_ID) >> 24) & 0xf);
}

/*
 * return 1 if Local APIC is BSP
 */
byte
lapic_get_bsp (void)
{
	return ((msr_get(MSR_APIC) & MSR_APIC_BSP) >> 8);
}

/*
 * return the Local APIC version
 */
byte
lapic_get_version (void)
{
	return (lapic_get(LAPIC_VERSION) & 0xff);
}

/*
 * return the Local APIC base address
 */
dword
lapic_get_base (void)
{
	return (dword) lapic;
}

void
lapic_dump (void)
{
	DPRINTF("LAPIC: ID      %08x\n", lapic_get(LAPIC_ID));
	DPRINTF("LAPIC: VER     %08x\n", lapic_get(LAPIC_VERSION));
	DPRINTF("LAPIC: TPR     %08x\n", lapic_get(LAPIC_TPR));
	DPRINTF("LAPIC: LDR     %08x\n", lapic_get(LAPIC_LDR));
	DPRINTF("LAPIC: DFR     %08x\n", lapic_get(LAPIC_DFR));
	DPRINTF("LAPIC: SVR     %08x\n", lapic_get(LAPIC_SVR));
	DPRINTF("LAPIC: ESR     %08x\n", lapic_get(LAPIC_ESR));
	DPRINTF("LAPIC: ICRL    %08x\n", lapic_get(LAPIC_ICRL));
	DPRINTF("LAPIC: ICRH    %08x\n", lapic_get(LAPIC_ICRH));
	DPRINTF("LAPIC: TIMER   %08x\n", lapic_get(LAPIC_TIMER));
	DPRINTF("LAPIC: LVT0    %08x\n", lapic_get(LAPIC_LVT0));
	DPRINTF("LAPIC: LVT1    %08x\n", lapic_get(LAPIC_LVT1));
	DPRINTF("LAPIC: LVTERR  %08x\n", lapic_get(LAPIC_LVTERR));
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
