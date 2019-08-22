/*
 *
 * Filename: cmos.h
 *
 * Description: CMOS variables and goodies like that
 *
 * Author(s): Tim Hockin
 *
 * Copyright 1999, Cobalt Networks, Inc.
 *
 */
#ifndef COMMON_CMOS_H
#define COMMON_CMOS_H

#define CMOS_ADDR_PORT		0x70
#define CMOS_DATA_PORT		0x71

#define NATSEMI_PM1_EVT_PORT	0x510
#define NATSEMI_PM1_CNT_PORT	0x520
#define NATSEMI_GPE1_PORT	0x530

#define CMOS_INFO_MAX		0x7f	/* top address allowed */

#define CMOS_CKS_START		0x0e
#define CMOS_CKS_END		CMOS_INFO_MAX

/* flag bytes - 16 flags for now, leave room for more */
#define CMOS_FLAG_BYTE_0	0x10
#define CMOS_FLAG_BYTE_1	0x11

/* flags in flag bytes - up to 16 */
#define CMOS_FLAG_MIN		0x0001
#define CMOS_CONSOLE_FLAG	0x0001	/* console on/off */
#define CMOS_DEBUG_FLAG		0x0002	/* ROM debug messages */
#define CMOS_AUTO_PROMPT_FLAG	0x0004	/* boot to ROM prompt? */
#define CMOS_CLEAN_BOOT_FLAG	0x0008	/* set by a clean shutdown */
#define CMOS_HW_NOPROBE_FLAG	0x0010	/* go easy on the probing */
#define CMOS_SYSFAULT_FLAG	0x0020	/* system fault detected */
#define CMOS_OOPSPANIC_FLAG	0x0040	/* panic on oops */
#define CMOS_DELAY_CACHE_FLAG	0x0080	/* delay cache initialization */
#define CMOS_NOLOGO_FLAG	0x0100	/* don't show boot logo */
#define CMOS_VERSION_FLAG	0x0200	/* the version field is valid */
#define CMOS_PASSWD_FLAG	0x0400	/* passwd enabled? */
#define CMOS_UNIQUE_FLAG	0x0800	/* unique nfsroot? */
#define CMOS_FLAG_MAX		0x0800

/* leave byte 0x12 blank */
#define CMOS_BIOS_DRIVE_INFO	0x12	/* drive info would go here */

#define CMOS_VERSION		0x13	/* CMOS format version # */
#define CMOS_VERSION_CURRENT	1	/* update this to the current ver. */
#define CMOS_VER_BTOCODE	1	/* min. version needed for btocode */

/* cmos password */
#define CMOS_PASSWD_LEN		8	/* len of cmos passwd */
#define CMOS_PASSWD_0		0x14	/* assuming hex passwd */
#define CMOS_PASSWD_1		0x15
#define CMOS_PASSWD_2		0x16
#define CMOS_PASSWD_3		0x17
#define CMOS_PASSWD_4		0x18
#define CMOS_PASSWD_5		0x19
#define CMOS_PASSWD_6		0x1a
#define CMOS_PASSWD_7		0x1b

#define CMOS_UNUSED_1c		0x1c	/* unused */
#define CMOS_UNUSED_1d		0x1d	/* unused */
#define CMOS_UNUSED_1e		0x1e	/* unused */
#define CMOS_UNUSED_1f		0x1f	/* unused */

#define CMOS_BOOT_METHOD	0x20	/* index of default boot method */
#define CMOS_BOOT_METHOD_DISK	0	/* disk kernel, disk fs */
#define CMOS_BOOT_METHOD_ROM	1	/* rom kernel, disk fs */
#define CMOS_BOOT_METHOD_NET	2	/* rom kernel, net fs */
#define CMOS_BOOT_METHOD_NNET	3	/* net kernel, net fs */
#define CMOS_BOOT_METHOD_OTHER	4	/* use boot_type cmos flag */

#define CMOS_BOOT_DEV_MAJ	0x21	/* major # of boot device */
#define CMOS_BOOT_DEV_MIN	0x22	/* minor # of boot device */
#define CMOS_ROOT_DEV_MAJ	0x23	/* major # of root device */
#define CMOS_ROOT_DEV_MIN	0x24	/* minor # of root device */
#define CMOS_UNUSED_25		0x25
#define CMOS_UNUSED_26		0x26
#define CMOS_UNUSED_27		0x27
#define CMOS_UNUSED_28		0x28

#define CMOS_UNUSED_2a		0x2a	/* unused */
#define CMOS_UNUSED_2b		0x2b	/* unused */
#define CMOS_UNUSED_2c		0x2c	/* unused */
#define CMOS_UNUSED_2d		0x2d	/* unused */

#define CMOS_CHECKSUM		0x2e	/* checksum bytes */
#define CMOS_CHECKSUM_1		0x2f

#define CMOS_UPTIME_0		0x30	/* running uptime counter */
#define CMOS_UPTIME_1		0x31	/* in units of 1 minute - 32 bits */
#define CMOS_UPTIME_2		0x32
#define CMOS_UPTIME_3		0x33

#define CMOS_UNUSED_34		0x34	/* unused */
#define CMOS_UNUSED_35		0x35	/* unused */
#define CMOS_UNUSED_36		0x36	/* unused */
#define CMOS_UNUSED_37		0x37	/* unused */

#define CMOS_BOOTCOUNT_0	0x38	/* count of successful boots */
#define CMOS_BOOTCOUNT_1	0x39
#define CMOS_BOOTCOUNT_2	0x3a
#define CMOS_BOOTCOUNT_3	0x3b

#define CMOS_UNUSED_3c		0x3c	/* unused */
#define CMOS_UNUSED_3d		0x3d	/* unused */
#define CMOS_UNUSED_3e		0x3e	/* unused */
#define CMOS_UNUSED_3f		0x3r	/* unused */

#define CMOS_SYS_SERNUM_LEN	13	/* 13 bytes */
#define CMOS_SYS_SERNUM_0	0x40	/* system serial number */
#define CMOS_SYS_SERNUM_1	0x41	/* same as on the back of */
#define CMOS_SYS_SERNUM_2	0x42	/* the system */
#define CMOS_SYS_SERNUM_3	0x43
#define CMOS_SYS_SERNUM_4	0x44
#define CMOS_SYS_SERNUM_5	0x45
#define CMOS_SYS_SERNUM_6	0x46
#define CMOS_SYS_SERNUM_7	0x47
#define CMOS_SYS_SERNUM_8	0x48
#define CMOS_SYS_SERNUM_9	0x49
#define CMOS_SYS_SERNUM_10	0x4a
#define CMOS_SYS_SERNUM_11	0x4b
#define CMOS_SYS_SERNUM_12	0x4c
#define CMOS_SYS_SERNUM_CSUM	0x4f
#define CMOS_SYS_SERNUM_OLD_CSUM 0xfe	/* checksum for serial num - 1 byte */

#define CMOS_ROM_REV_MAJ	0x50	/* 0-255 */
#define CMOS_ROM_REV_MIN	0x51	/* 0-255 */
#define CMOS_ROM_REV_REV	0x52	/* 0-255 */

#define CMOS_BTO_CODE_0		0x53
#define CMOS_BTO_CODE_1		0x54
#define CMOS_BTO_CODE_2		0x55
#define CMOS_BTO_CODE_3		0x56

#define CMOS_BTO_IP_CSUM	0x57
#define CMOS_BTO_IP_0		0x58 
#define CMOS_BTO_IP_1		0x59 
#define CMOS_BTO_IP_2		0x5a 
#define CMOS_BTO_IP_3		0x5b

#define CMOS_BOOT_TYPE		0x5c	/* byte for load/run methods */
#define CMOS_BOOT_KERN_DEV	0x01	/* use kernel on major/minor number */
#define CMOS_BOOT_KERN_ROM	0x02	/* use rom kernel */
#define CMOS_BOOT_KERN_NET	0x03	/* use net kernel */
#define CMOS_BOOT_KERN_MASK	0x0F	/* mask for above */
#define CMOS_BOOT_KERN_INVALID	0x0F	/* an invalid value */

#define CMOS_BOOT_FS_DEV	0x10	/* use major/minor number dev */
#define CMOS_BOOT_FS_UNUSED	0x20	/* unused */
#define CMOS_BOOT_FS_NET	0x30	/* use net fs */
#define CMOS_BOOT_FS_MASK	0xF0	/* mask for above */
#define CMOS_BOOT_FS_INVALID	0xF0	/* an invalid value */

#define CMOS_BOOT_TYPE_DEF	(CMOS_BOOT_KERN_DEV|CMOS_BOOT_FS_DEV)

#define CMOS_UNUSED_5d		0x3d	/* unused */
#define CMOS_UNUSED_5e		0x3e	/* unused */
#define CMOS_UNUSED_5f		0x3f	/* unused */

#define CMOS_UNUSED_60		0x60	/* unused */
#define CMOS_UNUSED_61		0x61	/* unused */
#define CMOS_UNUSED_62		0x62	/* unused */
#define CMOS_UNUSED_63		0x63	/* unused */
#define CMOS_UNUSED_64		0x64	/* unused */
#define CMOS_UNUSED_65		0x65	/* unused */
#define CMOS_UNUSED_66		0x66	/* unused */
#define CMOS_UNUSED_67		0x67	/* unused */
#define CMOS_UNUSED_68		0x68	/* unused */
#define CMOS_UNUSED_69		0x69	/* unused */
#define CMOS_UNUSED_6a		0x6a	/* unused */
#define CMOS_UNUSED_6b		0x6b	/* unused */
#define CMOS_UNUSED_6c		0x6c	/* unused */
#define CMOS_UNUSED_6d		0x6d	/* unused */
#define CMOS_UNUSED_6e		0x6e	/* unused */
#define CMOS_UNUSED_6f		0x6f	/* unused */

#define CMOS_FAULT_TIMESTAMP	0x70	/* FRU fault timestamp */
#define CMOS_FAULT_TIMESTAMP_0	0x70
#define CMOS_FAULT_TIMESTAMP_1	0x71
#define CMOS_FAULT_TIMESTAMP_2	0x72
#define CMOS_FAULT_TIMESTAMP_3	0x73
#define CMOS_FAULT_ERROR_CODE	0x74	/* FRU fault error code */
#define CMOS_FAULT_CODE		0x75	/* FRU fault code */
#define CMOS_FAULT_CODE_0	0x75
#define CMOS_FAULT_CODE_1	0x76
#define CMOS_STATUS_TIMESTAMP	0x77	/* FRU status timestamp */
#define CMOS_STATUS_TIMESTAMP_0 0x78
#define CMOS_STATUS_TIMESTAMP_1 0x79
#define CMOS_STATUS_TIMESTAMP_2 0x7A
#define CMOS_STATUS_TIMESTAMP_3 0x7B
#define CMOS_STATUS_CURRENT	0x7C	/* FRU current status */

#define CMOS_UNUSED_7d		0x7d	/* unused */
#define CMOS_UNUSED_7e		0x7e	/* unused */
#define CMOS_UNUSED_7f		0x7f	/* unused */

struct boot_dev_t {
	char *name;
	unsigned char maj;
	unsigned char min;
};

extern struct boot_dev_t boot_devs[];

unsigned char cmos_read_byte(int);
int cmos_write_byte(int, unsigned char);
int cmos_read_flag(int);
int cmos_write_flag(int, int);
int cmos_read_all_flags(void);
void cmos_write_all_flags(int);
unsigned char cmos_gen_csum(char *);
void cmos_read_serial_num(char *);
int cmos_get_moap(void);
void cmos_set_moap(const int);
int cmos_get_powerfail(void);
void cmos_set_powerfail(int state);
int cmos_get_powerapply(void);
void cmos_set_powerapply(int state);
unsigned short cmos_read_boot_dev(void);
unsigned short cmos_read_root_dev(void);
struct boot_dev_t *find_dev_by_name(char *);
struct boot_dev_t *find_dev_by_num(unsigned short);
int cmos_check_checksum(void);
void cmos_set_checksum(void);
void cmos_set_bto_ip(unsigned int ip);

#ifdef INROM	/* some is shared with CMOS tool - some is not */
int cmos_init(void);
void cmos_set_defaults(void);
void cmos_first_boot_defaults_only(int reset_sn);


/* the number of minutes the system has been running in usermode */
extern unsigned long uptime;

/* the system serial number - 13 bytes used now*/
extern char serial_num[16];

/* is the console enabled or disabled */
extern char console;

/* should rom show debug messages to console ? */
extern char debug;

/* index of what the system boots from - disk, net, rom, etc */
extern char boot_method;

/* should we automatically boot, or stop at ROM mode */
extern char auto_prompt;

/* is this the very first boot, or the battery is dead */
extern char first_boot;

/* did we shutdown cleanly */
extern char clean_boot;

/* should we skip hardware probing as much as possible */
extern char hw_noprobe;

/* should we show the cobalt lgog/name? */
extern char show_logo;

/* System Fault flag */
extern char sysfault;

/* use unique nfsroot? */
extern char unique_root;

/* CMOS structure version */
extern unsigned char cmos_version;

/* IP for bto server */
extern unsigned int bto_ip;

/* options for boot type */
extern unsigned char boot_type;

struct cmos_flag {
	char *name;
	int flag;
	char state;
};

extern struct cmos_flag cmos_flags[];

struct cmos_flag * cmos_flags_find(int);
struct cmos_flag * cmos_flags_namefind(char *);
void cmos_flags_sync(int);
void cmos_flags_print(void);

#endif /* INROM */

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
