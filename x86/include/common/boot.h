/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: boot.h,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */
#ifndef COMMON_BOOT_H
#define COMMON_BOOT_H

#include "cmos.h"

#define ROM_KERNEL		"/vmlinux.bz2"
#define BOOT_KERNEL_LINUX	"/boot/vmlinux.bz2,/vmlinux.bz2,/boot/vmlinux.gz,/vmlinux.gz"
#define BOOT_KERNEL_LINUX_SMP	"/boot/vmlinux-smp.bz2,/vmlinux-smp.bz2,/boot/vmlinux.bz2,/vmlinux.bz2,/boot/vmlinux-smp.gz,/vmlinux-smp.gz,/boot/vmlinux.gz,/vmlinux.gz"
#define BOOT_KERNEL_NETBSD	"/boot/netbsd.bz2"

#define BOOT_STAGE_LOAD          1
#define BOOT_STAGE_RUN           2
#define BOOT_STAGE_DEFAULT       BOOT_STAGE_RUN

#define BOOT_OPT_BFD      (CMOS_BOOT_KERN_DEV|CMOS_BOOT_FS_DEV)
#define BOOT_OPT_BFR      (CMOS_BOOT_KERN_ROM|CMOS_BOOT_FS_DEV)
#define BOOT_OPT_BFN      (CMOS_BOOT_KERN_ROM|CMOS_BOOT_FS_NET)
#define BOOT_OPT_BFNN     (CMOS_BOOT_KERN_NET|CMOS_BOOT_FS_NET)
#define BOOT_OPT_MASK     (CMOS_BOOT_KERN_MASK|CMOS_BOOT_FS_MASK)

#define BOOT_OPT_OTHER    (CMOS_BOOT_KERN_INVALID|CMOS_BOOT_FS_INVALID) /* use cmos/other settings */

struct cobalt_boot_return_data {
	int error;
	int flen;
};

struct boot_method_t {
	char *desc;
	int (*boot_func)(int);
	int arg;
};

extern char *boot_params, *load_params;
extern unsigned int serial_load_size;
extern struct boot_method_t boot_methods[];
extern unsigned int root_device, load_device;

typedef void (kernel_start_func) (int, int, int, int, int, int);
extern kernel_start_func *kernel_start_addr;

int do_bfX(int);
int do_bfd_netbsd(int);

int  boot_default(void);
void net_boot(const int);
void set_kernel_image(char *);
void set_boot_opt(const int /*type mask*/, int /*method*/);
void set_mem_limit(int limit);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
