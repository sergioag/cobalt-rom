/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: isapnp.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $ */

#ifndef COMMON_ISAPNP_H
#define COMMON_ISAPNP_H

#undef _BASE_0x370_
#ifdef _BASE_0x370_
#  define PNP_CONFIG_PORT 0x370
#  define PNP_INDEX_PORT 0x370
#  define PNP_DATA_PORT 0x371
#else
#  define PNP_CONFIG_PORT 0x3f0
#  define PNP_INDEX_PORT 0x3f0
#  define PNP_DATA_PORT 0x3f1
#endif

#define  _NS_BADDR0_
#ifdef _NS_BADDR0_
#  define PNP_NS_INDEX_PORT 0x002e
#  define PNP_NS_DATA_PORT 0x002f
#else
#  define PNP_NS_INDEX_PORT 0x015c
#  define PNP_NS_DATA_PORT 0x015D
#endif

#define PNP_NS_LOGICAL_DEV      0x07
#define PNP_NS_ADDR_HIGH        0x60
#define PNP_NS_ADDR_LOW         0x61
#define PNP_NS_INTR_PIN         0x70
#define PNP_NS_INTR_TYPE        0x71

/* RaQ-XTR SuperIO Logical Devices */
#define PNP_NS_DEV_RAQXTR_RTC   0x02
#define PNP_NS_DEV_RAQXTR_GPIO  0x07
#define PNP_NS_DEV_RAQXTR_PM    0x08
/* RaQ-Alpine SuperIO Logical Devices */
#define PNP_NS_DEV_ALPINE_RTC   0x10
#define PNP_NS_DEV_ALPINE_GPIO  0x07
#define PNP_NS_DEV_ALPINE_SWC   0x04

/* RTC registers */
#define PNP_NS_RTC_CRA		0x0a
#define PNP_NS_RTC_BANK_0	0x20
#define PNP_NS_RTC_BANK_2       0x40
#define PNP_NS_APCR1		0x40
#define PNP_NS_APCR6		0x4c

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
