/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: boot.h,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */

#ifndef MONITOR_BOOT_H
#define MONITOR_BOOT_H

int load_kernel_func(int argc, char *argv[]);
int dl_kernel_func(int argc, char *argv[]);
int boot_kernel_func(int argc, char *argv[]);
int net_kernel_func(int argc, char *argv[]);
int checksum_kernel_func(int argc, char *argv[]);
int bunzip2_kernel_func(int argc, char *argv[]);
int bfr_func(int argc, char *argv[] __attribute__ ((unused)));
int set_params_func(int argc, char *argv[]);
int set_load_params_func(int argc, char *argv[]);
int boot_netbsd_func(int argc, char *argv[] __attribute__ ((unused)));
int bfd_func(int argc, char *argv[] __attribute__ ((unused)));
int bfn_func(int argc, char *argv[] __attribute__ ((unused)));
int bfnn_func(int argc, char *argv[] __attribute__ ((unused)));
int bfX_func(int argc, char *argv[]);
int reboot_func(int argc, char *argv[] __attribute__ ((unused)));
int halt_func(int argc, char *argv[] __attribute__ ((unused)));
int boot_default_func(int, char *[] __attribute__ ((unused)));
int set_boot_method_func(int, char *[]);
int read_boot_method_func(int, char *[] __attribute__ ((unused)));
int set_boot_dev_func(int, char *[]);
int read_boot_dev_func(int, char *[] __attribute__ ((unused)));
int set_root_dev_func(int, char *[]);
int read_root_dev_func(int, char *[] __attribute__ ((unused)));
int set_mem_limit_func(int, char *[]);
int boot_func(int argc, char *argv[]);
int init_delay_func(int argc, char *argv[] __attribute__ ((unused)));
int hlt_func(int argc, char *argv[]);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
