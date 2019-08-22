/* $Id: crfs_func.h,v 1.1.1.1 2003/06/10 22:41:36 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs_func.h
 *
 * functions to perform basic fs operations
 */

#ifndef COBALT_ROMFS_FUNC_H
#define COBALT_ROMFS_FUNC_H

#include "crfs.h"

#define CO_FILE_MAGIC	0x00c0f100
typedef struct {
	unsigned long F_magic;
	inode_id_t F_inode;	/* inode number */
	block_id_t F_curblock;	/* current fblock block number */
	unsigned int F_fpos;	/* file position */
	int F_nospc;		/* are we out of disk space? */
	crfs_fblock *F_fblock;	/* currently active fblock */
} CO_FILE;

#define CO_DIR_MAGIC	0x00c0d100
typedef struct {
	unsigned long D_magic;
	inode_id_t D_inode;	/* inode number */
	unsigned int D_pos;	/* offset */
	crfs_dirent D_dirent;	/* last read dirent */
} CO_DIR;

struct crfs_stat {
	inode_id_t st_ino;	/* inode */
	fmask_t st_fmask;	/* file mask */
	uint32 st_size;		/* total size, in bytes */
};	

/*
 * crfs_init
 *
 * initalized the internal data structures of the filesystem core
 * code.  This is in some way analogous to mounting the filesystem
 */  
int crfs_init(void);

/*
 * crfs_mkfs
 *   size - size of the filesytem to make
 *
 * initalizes a new filesystem.
 */
int crfs_mkfs(size_t size);

/*
 * crfs_create
 *   fname - filename to create
 *
 * creates a 0 length file
 */
int crfs_create(char *fname);

/*
 * crfs_remove
 *   fname - filename to remove
 *
 * removes a file
 */
int crfs_remove(char *fname);

/*
 * crfs_open
 *   fname - filename to open
 *
 * opens the file and returns the file structure or NULL on an
 * error condition.
 */
CO_FILE *crfs_open(char *fname);

/*
 * crfs_stat
 *
 * gets info about a file
 */
int crfs_stat(char *fname, struct crfs_stat *buf);

/*
 * crfs_flush
 *
 * makes sure the FS state is written to the backing store - GLOBAL
 */
int crfs_flush(void);

/*
 * crfs_close
 *   fp - file pointer to close
 *
 * Flushes the fs, closes the file, and frees the file pointer
 */
int crfs_close(CO_FILE *fp);

/*
 * crfs_read
 *   fp - file pointer to read from
 *   buff - buffer to read into
 *   size - number of bytes to be read
 *
 * reads size number of bytes into buff from the current location
 * of fp.
 */
int crfs_read(CO_FILE *fp, void *buff, size_t size);

/*
 * crfs_write
 *   fp - file pointer to write to
 *   buff - buffer to write from
 *   size - number of bytes to write
 *
 * writes size number of bytes into fp from buff
 */
int crfs_write(CO_FILE *fp, void *buff, size_t size);

#define CRFS_SEEK_SET	0
#define CRFS_SEEK_CUR	1
#define CRFS_SEEK_END	2
/*
 * crfs_seek
 *   fp - file pointer to seek in
 *   delta - amount to seek
 *   whence - where to start form
 *
 * seeks in fp by delta amount.
 */
int crfs_seek(CO_FILE *fp, int delta, int whence);

/*
 * crfs_mkdir
 *   path - name of directory to create
 *
 * creates a directory
 */
int crfs_mkdir(char *path);

/*
 * crfs_rmdir
 *   path - name of directory to remove
 *
 * NOTE: this is recursive - like rm -rf
 *
 * removes a directory
 */
int crfs_rmdir(char *path);

/*
 * crfs_opendir
 *   name - name of directory to open
 *
 * opens a directory and returns a handle to it.
 */
CO_DIR *crfs_opendir(char *name);

/*
 * crfs_readdir
 *   dir - directory pointer to use
 *
 * gets the next valid dirent in the directory.  
 * Returns NULL if at the end of a directory
 */
crfs_dirent *crfs_readdir(CO_DIR *dir);

/*
 * crfs_closedir
 *   dir - directory to close
 *
 * closes the directory and frees the handle
 */
int crfs_closedir(CO_DIR *dir);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
