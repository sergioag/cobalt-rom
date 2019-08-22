/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: msr.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 */

#ifndef COMMON_MSR_H
#define COMMON_MSR_H

#define wrmsr(msr,lo,hi)	__asm__ __volatile__("wrmsr" \
				     : \
				     :"c" (msr), "a" (lo), "d"(hi))

unsigned long long msr_get(unsigned int index);
void msr_set(unsigned int index, unsigned long long value);
void msr_set_on(unsigned int index, unsigned long long bits);
void msr_set_off(unsigned int index, unsigned long long bits);

#define MSR_APIC  		0x1b	/* APIC MSR: base, enable, bsp 	*/
#define  MSR_APIC_EN		0x800	/* enable/disable bit in MSR 	*/
#define  MSR_APIC_BSP		0x100	/* indicate bootstrap processor */

#define MSR_EBL_CR_POWERON	0x2a
#define MSR_MICROCODE_SIG	0x8b
#define MSR_MICROCODE_TRIG	0x79
#define MSR_BBL_CR_OVRD		0x17

#define MSR_BBL_CR_D0		0x88
#define MSR_BBL_CR_D1		0x89
#define MSR_BBL_CR_D2		0x8a
#define MSR_BBL_CR_D3		0x8b

#define MSR_BBL_CR_ADDR		0x116
#define MSR_BBL_CR_DECC		0x118
#define MSR_BBL_CR_CTL		0x119
#define MSR_BBL_CR_TRIG		0x11a
#define MSR_BBL_CR_BUSY		0x11b
#define MSR_BBL_CR_CTL3		0x11e

#define MSR_MTRR_CAP		0xfe
#define MSR_MTRRvar		0x200
#define MSR_MTRRdefType		0x2ff
#define  MSR_MTRRfixEnable	0x400		/* bit 10 */
#define  MSR_MTRRvarEnable	0x800		/* bit 11 */
#define MSR_MTRRphysBase(num)	(MSR_MTRRvar + (num<<1))
#define MSR_MTRRphysMask(num)	(MSR_MTRRvar + 1 + (num<<1))

#define MSR_MTRRfix64K_00000	0x250 /*   0-512K  : 0606060606060606 */
#define MSR_MTRRfix16K_80000	0x258 /* 512-640K  : 0606060606060606 */
#define MSR_MTRRfix16K_A0000	0x259 /* 640-768K  : 0000000000000000 */
#define MSR_MTRRfix4K_C0000	0x268 /* 768-800K  : 0000000000000000 */
#define MSR_MTRRfix4K_C8000	0x269 /* 800-832K  : 0000000000000000 */
#define MSR_MTRRfix4K_D0000	0x26a /* 832-864K  : 0000000000000000 */
#define MSR_MTRRfix4K_D8000	0x26b /* 864-896K  : 0000000000000000 */
#define MSR_MTRRfix4K_E0000	0x26c /* 896-928K  : 0000000000000000 */
#define MSR_MTRRfix4K_E8000	0x26d /* 928-960K  : 0000000000000000 */
#define MSR_MTRRfix4K_F0000	0x26e /* 960-992K  : 0505050505050505 */
#define MSR_MTRRfix4K_F8000	0x26f /* 992-1024K : 0505050505050505 */

#define MSR_BBL_CR_MC4_CTL	0x40c
#define MSR_BBL_CR_STATUS	0x40d

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
