/* $Id: crfs.h,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs.h
 */

#ifndef COBALT_ROMFS_H
#define COBALT_ROMFS_H

#ifdef COBALT_ROMFS_INTERNAL
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
#endif

#ifdef INROM
#include <extra_types.h>
#endif

/* 16 bit inode and block identifiers */
typedef uint16 inode_id_t;
typedef uint16 block_id_t;

/* 4k blocks - a lot depends on this! */
#define CRFS_BLOCK_SIZE		0x1000

typedef enum {
	CRFS_FMASK_FILE = 	0x1,
	CRFS_FMASK_EXE = 	0x2,
	CRFS_FMASK_DIR = 	0x4,
	CRFS_FMASK_GZ = 	0x8,
} fmask_t;

/*
 * internal data structures of CRFS
 *
 * NOTE: The absolute size of these structs is ALL IMPORTANT!!
 */

/* 
 * a super block
 *
 * this must be exactly 1 block
 */
#define CRFS_NSBLOCKS		2
#define CRFS_SB_VTABLE_LEN	16
struct _crfs_super_block {
	char sb_magic[4]; 	/* "CRfs" */
	uint32 sb_fsver;
	uint32 sb_checksum;	
	uint32 sb_blockver;	/* incremented on change */
	uint32 sb_fssize;	/* size of the fs */
	inode_id_t sb_root;	/* inode # of the root dir */
	block_id_t sb_vtable[CRFS_SB_VTABLE_LEN]; /* blocks holding itables */
}; 
typedef union {
	struct _crfs_super_block usb_data;
	char pad[CRFS_BLOCK_SIZE];
} crfs_super_block;
#define sb_magic	usb_data.sb_magic
#define sb_fsver	usb_data.sb_fsver
#define sb_checksum	usb_data.sb_checksum
#define sb_blockver	usb_data.sb_blockver
#define sb_fssize	usb_data.sb_fssize
#define sb_root		usb_data.sb_root
#define sb_vtable	usb_data.sb_vtable

/*
 * an inode
 *
 * this must be exactly 16 bytes
 */
struct _crfs_inode {
	inode_id_t in_inode;	/* inode number */
	block_id_t in_first;	/* first block of this inode */	
	block_id_t in_last;	/* last block of this inode */
	uint32 in_fsize;	/* size of file */
	fmask_t in_fmask;	/* file features */
};
typedef union {
	struct _crfs_inode uin_data;
	char pad[16];
} crfs_inode;
#define in_inode	uin_data.in_inode
#define in_first	uin_data.in_first
#define in_last		uin_data.in_last
#define in_fsize	uin_data.in_fsize
#define in_fmask	uin_data.in_fmask

/*
 * an inode table
 *
 * if crfs_inode stays 16, this will be sized right
 */
#define CRFS_ITABLE_NINODES	256
typedef struct {
	crfs_inode it_inodes[CRFS_ITABLE_NINODES];
} crfs_itable;

/*
 * a file block (part of a chain)
 *
 * this must be exactly 1 block
 */
struct _crfs_fblock {
	block_id_t fb_prev;	/* previous block */
	block_id_t fb_next;	/* next block */
	block_id_t fb_offset;	/* offset within the file (in blocks) */
	char reserved[10];	/* for expansion */
	uint8 fb_data[0];	/* data makes up the rest */
};
typedef union {
	struct _crfs_fblock ufb_data;
	char pad[CRFS_BLOCK_SIZE];
} crfs_fblock;
#define fb_prev		ufb_data.fb_prev
#define fb_next		ufb_data.fb_next
#define fb_offset	ufb_data.fb_offset
#define fb_data		ufb_data.fb_data
#define CRFS_FBLOCK_DATA	(CRFS_BLOCK_SIZE - sizeof(struct _crfs_fblock))

/*
 * a directory entry
 *
 * this must be exactly 64 bytes
 */
#define CRFS_FILENAME_MAX	59
struct _crfs_dirent {
	inode_id_t de_inode;	/* inode number of dirent */
	char de_name[CRFS_FILENAME_MAX + 1];	/* name of dirent */
};
typedef union {
	struct _crfs_dirent ude_data;
	char pad[64];
} crfs_dirent;
#define de_inode	ude_data.de_inode
#define de_name		ude_data.de_name

/*
 * a directory
 *
 * if crfs_dirent stays 64, this will be sized right - 1 block
 */
#define CRFS_DIR_NENTS	64
typedef struct {
	crfs_dirent d_entries[CRFS_DIR_NENTS];
} crfs_directory;

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
