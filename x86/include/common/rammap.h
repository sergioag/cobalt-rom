/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: rammap.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $ */

/*
 * rammap.h
 */
#ifndef COMMON_RAMMAP_H
#define COMMON_RAMMAP_H

#include "common/romconf.h"

/* linux loads at 1 MB */
#define RAM_LINUX_LOAD		(0x100000)

/* the address at which the ramcode is loaded - this is always >= 22MB */
#define RAMCODE_START		(RAM_RAMCODE_ADDR)
/* the MAXIMUM address that any piece of the ramcode can use */
#define RAMCODE_END		(0x02000000)

/* the base ramcode address - small aread needed */
#define RAM_RAM_CODE		(RAM_RAMCODE_ADDR)
/* the base monitor address */
#define RAM_MONITOR_CODE	(RAM_MONITOR_ADDR)
#define RAM_MONITOR_MAX		(RAMCODE_START + 0x100000)

/* serial load area - 4MB allocated */
#define RAM_SERIAL_LOAD		(RAM_MONITOR_MAX)
#define RAM_SERIAL_LOAD_MAX	(RAM_SERIAL_LOAD + 0x400000)

/* the heap - the rest of memory, less 1 MB for stack on low-mem systems */
#define RAM_HEAP_SPACE		(RAM_SERIAL_LOAD_MAX)
#define RAM_HEAP_SPACE_MAX	(RAMCODE_END - 0x100000)

/*
 * in putting this below RAMCODE, we are making assumptions:
 * 1) Nothing in DECOMP will be needed across kernel runs
 * 2) No kernel will ever relocate itself > 16 MB (see boot_main.c)
 */

/* decompression area - 6MB allocated @ 16 MB */
#define RAM_DECOMP_AREA     	(0x01000000)
#define RAM_DECOMP_AREA_MAX	(RAM_DECOMP_AREA + 0x00600000)

/*
 * PCI address space region limits
 * 
 */
#define RAM_PCI_REGION_IO	(RAM_IO_REGION)
#define RAM_PCI_REGION_MMIO	(RAM_MEM_REGION)

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
