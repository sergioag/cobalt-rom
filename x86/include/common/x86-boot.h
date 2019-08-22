/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: x86-boot.h,v 1.1.1.1 2003/06/10 22:42:04 iceblink Exp $ */

#ifndef COMMON_X86_BOOT_H
#define COMMON_X86_BOOT_H

void setup_boot(int, char *);
void return_from_kernel(void);
volatile void boot_kernel_linux(void);
volatile void boot_kernel_netbsd (void);
volatile void read_kernel_from_disk(char *, void (*)(void));
volatile void setup_net_boot(const int, const char *, void (*)(void));
void copy_kernel(void);
void dl_kernel(void);
void bunzip2_kernel(void);
void reset_lcd_for_boot(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
