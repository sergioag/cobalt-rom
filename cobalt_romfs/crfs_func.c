/* $Id: crfs_func.c,v 1.1.1.1 2003/06/10 22:41:36 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs_func.c
 *
 * functions to perform basic fs operations
 */

/*
 * Notes:
 *
 * When writing, we use a phased commit.  Whenever a fresh (not write-cached)
 * block is to be written, we allocate a new block, duplicate the original,
 * and put all writes into this new block.  This means that the old data and
 * block layout is left unmolested.  When the superblock is written to the
 * backing-store (be that flash or a diskfile), all the other writes are done.
 * The new superblock is then valid for the new data.  If the write fails at
 * any time before the superblock is written, the old superblock will be more
 * current, and the old data will be used (no corruption).  If the superblock
 * itself fails to write, we should notice it when we checksum it next, and
 * use the other super block.  At least, that is the theory.
 */ 

/* 
 * some conventions:
 *
 * A block being 0 means that it is not valid as this is
 * one of the superblocks.
 *
 * Inode 0 is reserved for error conditions.  And therefore
 * should never be allocated.
 *
 * A dirent with an inode of 0 is invalid/free because of the
 * above reason.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifdef INROM
# include <printf.h>
# include "common/rom.h"
#else
# include <stdio.h>
#endif

#include "crfs_func.h"
#include "crfs_block.h"

#define CRFS_DEBUG	0
#if CRFS_DEBUG
# ifdef INROM
#  define FSDBG(f, a...)		printf("CRFS: " f, ##a);
# else
#  define FSDBG(f, a...)		fprintf(stderr, "CRFS: " f, ##a);
# endif
#else
# define FSDBG(f, a...)
#endif

/* global block cache */
struct crfs_datum {
	struct crfs_datum *dt_next;
	block_id_t dt_blockaddr;
	int dt_dirty;
	void *dt_data;
};
static struct crfs_datum *crfs_block_cache;

/* global maps of available/used items */
struct freemap {
	int fm_cur; /* byte before which we KNOW there are none free */
	int fm_mapsize;
	unsigned char *fm_map;
	unsigned char *fm_freed;
};
static struct freemap bl_fm, in_fm;
static struct freemap *block_freemap = &bl_fm;
static struct freemap *inode_freemap = &in_fm;

static int freemap_init(struct freemap *map, int mapsize);
static int freemap_next(struct freemap *map);
static int freemap_flush(struct freemap *map);
static int freemap_mark(struct freemap *map, int bit);
static int freemap_unmark(struct freemap *map, int bit);
#define freemap_isset(map, b)	((map)->fm_map[(b)>>3] & (0x1<<((b)&0x7)))

/* the current super block */
static crfs_super_block *super_block;
static int crfs_which_super = -1;
#define SB_BLOCKNUM(sb)		((sb) * 16)

/* current working dir */
static uint16 crfs_pwd;

static crfs_inode crfs_lookup_inode(inode_id_t inum);
static int crfs_write_inode(crfs_inode *inode);
static int crfs_lookup_dirent(crfs_directory *dir, char *name, int nlen, 
	crfs_dirent *de);
static int crfs_namei(char **fname, crfs_inode *dirnode);
static int append_fblock(CO_FILE *f);
static int next_fblock(CO_FILE *fp);
static void mark_inode(inode_id_t inode);
static void unmark_inode(inode_id_t inode);
static int rm_file(char *fname, int do_dirs);
static void *block_dup(void *dest, void *src);
static int is_dir(crfs_inode *dirnode, char *fname);
static crfs_inode inode_lookup_from_namei(crfs_inode *dirnode, char *fname);

static void *crfs_read_block(block_id_t idx, void *buf);
static int crfs_write_block(block_id_t idx, void *data);
static struct crfs_datum *cache_add(block_id_t idx, void *data, int dirty);
static int cache_flush(void);
static int block_is_dirty(block_id_t idx);

/* other macros */
#define ITABLE_BLOCKNUM(inum)	(super_block->sb_vtable[(inum) >> 8])
#define INODE_IDX(inum)		((inum) & 0xff)


/*
 * freemap manipulation routines
 */
static int
freemap_init(struct freemap *map, int mapsize)
{
	map->fm_map = malloc(mapsize);
	if (!map->fm_map) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}
	map->fm_freed = malloc(mapsize);
	if (!map->fm_freed) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	map->fm_cur = 0;
	map->fm_mapsize = mapsize;
	memset(map->fm_map, 0, mapsize);
	memset(map->fm_freed, 0, mapsize);
	
	return 0;
}

static int
freemap_flush(struct freemap *map)
{
	map->fm_cur = 0;
	memset(map->fm_freed, 0, map->fm_mapsize);

	return 0;
}

static int 
freemap_next(struct freemap *map)
{
	int i;
	int j;
	
	/* search for a free bit */
	for (i = map->fm_cur; i < map->fm_mapsize; i++) {
		unsigned char val = map->fm_map[i] | map->fm_freed[i];

		if (val != 0xff) {
			/* look at each bit in the byte */
			for (j = 0; j < CHAR_BIT; j++) {
				if ((val & (1<<j)) == 0) {
					map->fm_map[i] |= (1<<j);
					map->fm_cur = i;
					return (i*CHAR_BIT)+j;
				}
			}
		} else {
			map->fm_cur++;
		}
	}
	FSDBG("error: freemap is full - try flushing cache\n");
	return -1;
}

static int
freemap_mark(struct freemap *map, int bit)
{
	map->fm_map[bit/CHAR_BIT] |= 0x1 << (bit % CHAR_BIT);
	return 0;
}

static int
freemap_unmark(struct freemap *map, int bit)
{
	int by = bit/CHAR_BIT;
	int bi = 1 << (bit % CHAR_BIT);
	
	map->fm_freed[by] |= (map->fm_map[by] & bi);
	map->fm_map[by] &= ~bi;

	return 0;
}

static void *
block_dup(void *dest, void *src)
{
	if (!dest) {
		dest = malloc(CRFS_BLOCK_SIZE);
		if (!dest) {
			printf("malloc() failed at %s:%d\n", 
				__FILE__, __LINE__);
			return NULL;
		}
	}

	memcpy(dest, src, CRFS_BLOCK_SIZE);

	return dest;
}

static crfs_inode 
crfs_lookup_inode(inode_id_t inum)
{
	block_id_t block_index;
	unsigned char buf[CRFS_BLOCK_SIZE];
	crfs_itable *it;
	
	block_index = ITABLE_BLOCKNUM(inum);
	it = crfs_read_block(block_index, buf);

	FSDBG("lookup_inode: block_index: %d\n", block_index);
	FSDBG("lookup_inode: inum %d, in_inode %d, first %d, size %d\n", 
		inum,
		it->it_inodes[INODE_IDX(inum)].in_inode,
		it->it_inodes[INODE_IDX(inum)].in_first,
		it->it_inodes[INODE_IDX(inum)].in_fsize);

	return  it->it_inodes[INODE_IDX(inum)];
}

/*
 * write out an inode and associated itable
 *
 * returns:
 *	0 on success
 *	-ENOMEM if a malloc failed
 *	-ENOSPC if no blocks are free
 */
static int 
crfs_write_inode(crfs_inode *inode)
{
	block_id_t block_index;
	crfs_itable *it;
	unsigned char buf[CRFS_BLOCK_SIZE];
	
	/* figure out on which block is the itable for this inode */
	block_index = ITABLE_BLOCKNUM(inode->in_inode);
	if (!block_index) {
		/* hrrm, doesn't exist yet */
		int tmp;

		tmp = freemap_next(block_freemap);
		if (tmp < 0) {
			return -ENOSPC;
		}

		/* use this new block for an itable, store it in vtable */
		block_index = tmp;
		ITABLE_BLOCKNUM(inode->in_inode) = block_index;
		it = malloc(CRFS_BLOCK_SIZE);
		if (!it) {
			printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
			return -ENOMEM;
		}
		memset(it, 0, sizeof(*it));
	} else {
		/* read the current value, no matter what */
		it = crfs_read_block(block_index, buf);

		/* phased-commit here, see comment near top of file */
		if (!block_is_dirty(block_index)) {
			int tmp;

			/* find a new one */
			tmp = freemap_next(block_freemap);
			if (tmp < 0) {
				return -ENOSPC;
			}

			/* use the old to start the new one */
			freemap_unmark(block_freemap, block_index);
			block_index = tmp;
			ITABLE_BLOCKNUM(inode->in_inode) = block_index;
		}
	}
	
	/* store the new inode in the itable */
	it->it_inodes[INODE_IDX(inode->in_inode)] = *inode;

	crfs_write_block(block_index, it);
	
	return 0;
}

static int 
crfs_lookup_dirent(crfs_directory *dir, char *name, int nlen, crfs_dirent *de)
{
	int i;
	char tmp[CRFS_FILENAME_MAX + 1];
	
	/* truncate if we have to */
	if (nlen > CRFS_FILENAME_MAX) {
		return -1;
	}

	/* make a local copy for an exact match */
	strncpy(tmp, name, nlen);
	tmp[nlen] = '\0';

	for (i = 0; i < CRFS_DIR_NENTS; i++) { 
		crfs_dirent *tmpde = &dir->d_entries[i];
		if (!strcmp(tmp, tmpde->de_name) && tmpde->de_inode) {
			break;
		}
	}

	if (i < CRFS_DIR_NENTS) {
		*de = dir->d_entries[i];
		return 0;
	} else {
		de->de_inode = 0;
		return -1;
	}
}

/* 
 * crfs_namei
 *
 * get the dir inode and leaf dirent of a path
 *
 * returns:
 *	0 on success
 *	-ENOENT if a directory piece does not exist
 *	-ENOTDIR if a directory piece is not a dir
 */
static int 
crfs_namei(char **fname, crfs_inode *dirnode)
{
	int done = 0;
	char *str_start;
	char *str_end;
	block_id_t block_index;
	crfs_inode inode;
	crfs_dirent de;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int r;
	
	str_start = str_end = *fname;

	/* get the inode for the first dir */
	if (*str_start == '/') {
		inode = crfs_lookup_inode(super_block->sb_root);
		block_index = inode.in_first;
		str_end = ++str_start;
	} else {
		inode = crfs_lookup_inode(crfs_pwd);	
		block_index = inode.in_first;
	}

	FSDBG("namei: start inode %d, block %d\n", 
		inode.in_inode, inode.in_first);

	while (!done) {
		/* look for a new delimiter (dir or end of str) */
		while ((*str_end != '/') && (*str_end != '\0')) {
			str_end++;
		}
		
		switch (*str_end) {
		  case '/': {
			/* a dir element */
			crfs_directory *d;
			
			/* multiple slashes are OK */
			if (str_start == str_end) {
				str_start = ++str_end;
				continue;
			}
			/* read curdir */
			d = crfs_read_block(block_index, buf);
			r = crfs_lookup_dirent(d, str_start, 
				str_end - str_start, &de);
			if (r < 0) {
				dirnode->in_inode = 0;
				return -ENOENT;
			}

			/* if it is the end of path */
			if (*(str_end + 1) == '\0') {
				*str_end = '\0';
				done = 1;
			} else {
				/* has a '/' - must be a dir */
				inode = crfs_lookup_inode(de.de_inode);
				if (!inode.in_inode 
				 || !(inode.in_fmask & CRFS_FMASK_DIR)) {
					dirnode->in_inode = 0;
					return -ENOTDIR;
				}
				/* set this as curdir */
				block_index = inode.in_first;
				str_start = ++str_end;
			}
			break;
		  }
		  case '\0': {
			/* end of path */
			done = 1;
			break;
		  }
		}	
	}

	/* save the results */
	*dirnode = inode;
	*fname = str_start;

	return 0;
}

static int
next_fblock(CO_FILE *fp)
{
	if (fp->F_fblock->fb_next) {
		fp->F_curblock = fp->F_fblock->fb_next;
		crfs_read_block(fp->F_curblock, fp->F_fblock);
		return 0;
	}

	return -1;
}
	
/*
 * allocate and load a new fblock onto the CO_FILE
 *
 * returns:
 *	0 on success
 *	-ENOSPC if no blocks are free
 */
static int 
append_fblock(CO_FILE *fp)
{
	int block_index;
	crfs_inode inode;
	
	/* get the next free block */
	block_index = freemap_next(block_freemap);
	if (block_index < 0) {
		fp->F_nospc = 1;
		return -ENOSPC;
	}
	fp->F_nospc = 0;
	
	/* get the inode */
	inode = crfs_lookup_inode(fp->F_inode);

	if (fp->F_curblock) {
		/* write the old fblock */
		fp->F_fblock->fb_next = block_index;
		crfs_write_block(fp->F_curblock, fp->F_fblock);
		fp->F_fblock->fb_offset++;
	} else {
		/* new fblock */
		fp->F_fblock->fb_offset = 0;
		inode.in_first = block_index;
	}

	/* update it for new data */
	fp->F_fblock->fb_prev = fp->F_curblock;
	fp->F_fblock->fb_next = 0;
	memset(fp->F_fblock->fb_data, 0, CRFS_FBLOCK_DATA);
	fp->F_curblock = block_index;

	/* write it */
	crfs_write_block(block_index, fp->F_fblock);

	/* update the inode */
	inode.in_last = block_index;
	crfs_write_inode(&inode);

	return 0;
}


static uint32 
crfs_checksum(crfs_super_block *sblock)
{
	unsigned int i;
	uint32 sum = 0;
	
	for (i = 0; i < CRFS_BLOCK_SIZE/sizeof(uint32); i++) {
		if (i != 2) {
			sum += ((uint32 *)sblock)[i];
		}
	}
	sum = ~(sum-1);
	return sum;
}

/*
 * denote an inode (and all blocks/dirents) as in-use
 */
static void 
mark_inode(inode_id_t inode)
{
	block_id_t block_index;
	crfs_itable *it;
	crfs_inode *in;
	int i;
	unsigned char buf[CRFS_BLOCK_SIZE];
	
	/* this inode is now in use */
	freemap_mark(inode_freemap, inode);

	/* read the itable for this inode */
	block_index = ITABLE_BLOCKNUM(inode);
	it = crfs_read_block(block_index, buf);
	in = &(it->it_inodes[INODE_IDX(inode)]);
	
	/* see if it is a file or directory */
	if (in->in_fmask & CRFS_FMASK_DIR) {
		/* it's a dir */
		crfs_directory *d;

		/* mark it's first (only) block as busy */
		freemap_mark(block_freemap, in->in_first);

		/* read the block */
		block_index = in->in_first;
		d = crfs_read_block(block_index, buf);

		/* walk the list of dirents */
		for (i = 0; i < CRFS_DIR_NENTS; i++) {
			crfs_dirent *de = &(d->d_entries[i]);

			/* mark inodes for every dirent */
			if (de->de_inode) {
			    if (!freemap_isset(inode_freemap, de->de_inode)) {
			        mark_inode(de->de_inode);
			    }
			}
		}
	} else {
		/* it's a file of some sort */
		crfs_fblock *f;

		/* walk the chain of file blocks */
		block_index = in->in_first;
		while (block_index) {
			/* mark blocks as used */
			freemap_mark(block_freemap, block_index);
			f = crfs_read_block(block_index, buf);
			block_index = f->fb_next;
		}
	}
}

static void 
unmark_inode(inode_id_t inode)
{
	block_id_t block_index;
	crfs_itable *it;
	crfs_inode *in;
	int i;
	unsigned char buf[CRFS_BLOCK_SIZE];
	
	/* this inode is now free */
	freemap_unmark(inode_freemap, inode);

	/* read the itable for this inode */
	block_index = ITABLE_BLOCKNUM(inode);
	it = crfs_read_block(block_index, buf);
	in = &(it->it_inodes[INODE_IDX(inode)]);
	
	/* see if it is a file or directory */
	if (in->in_fmask & CRFS_FMASK_DIR) {
		/* it's a dir */
		crfs_directory *d;

		/* mark it's first (only) block as busy */
		freemap_unmark(block_freemap, in->in_first);

		/* read the block */
		block_index = in->in_first;
		d = crfs_read_block(block_index, buf);

		/* walk the list of dirents - skip "." and ".." */
		for (i = 2; i < CRFS_DIR_NENTS; i++) {
			crfs_dirent *de = &(d->d_entries[i]);

			/* unmark inodes for every dirent */
			if (de->de_inode) {
				unmark_inode(de->de_inode);
			}
		}
	} else {
		/* it's a file of some sort */
		crfs_fblock *f;

		/* walk the chain of file blocks */
		block_index = in->in_first;
		while (block_index) {
			/* mark blocks as free */
			freemap_unmark(block_freemap, block_index);
			f = crfs_read_block(block_index, buf);
			block_index = f->fb_next;
		}
	}
}

static int
rm_file(char *fname, int do_dirs)
{
	crfs_inode dirnode;
	crfs_dirent de;
	block_id_t dirblock;
	crfs_directory *d;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int r;
	int i;
	
        /* get the requested directory */
	r = crfs_namei(&fname, &dirnode);
	if (r < 0) {
		return r;
	}

	/* check filename length */
	if (strlen(fname) > CRFS_FILENAME_MAX) {
		return -ENAMETOOLONG;
	}

	r = is_dir(&dirnode, fname);
	if (r && !do_dirs) {
		return -EISDIR;
	}

	if (!strcmp(fname, ".") || !strcmp(fname, "..")) {
		return -EINVAL;
	}

	/* read the directory */
	dirblock = dirnode.in_first;
	d = crfs_read_block(dirblock, buf); 	

	/* look it up and remove it */
	r = crfs_lookup_dirent(d, fname, strlen(fname), &de);
	if (r < 0) {
		return -ENOENT;
	}

	for (i = 0; i < CRFS_DIR_NENTS; i++) {
		if (!strcmp(d->d_entries[i].de_name, fname)) {
			/* de-populate the dirent */
			d->d_entries[i].de_inode = 0;
			d->d_entries[i].de_name[0] = '\0';

			/* save this dir */
			crfs_write_block(dirblock, d);
			break;
		}
	}

	/* unmark the inode and all fblocks */
	unmark_inode(de.de_inode);

	return 0;
}

static int
is_dir(crfs_inode *dirnode, char *fname)
{
	crfs_inode inode;

	inode = inode_lookup_from_namei(dirnode, fname);
	return (inode.in_fmask & CRFS_FMASK_DIR);
}

static crfs_inode
inode_lookup_from_namei(crfs_inode *dirnode, char *fname)
{
	unsigned char buf[CRFS_BLOCK_SIZE];
	crfs_directory *d;
	crfs_dirent de;
	int r;

	/* hack for "/" */
	if (!*fname) {
		return *dirnode;
	}

	/* read the directory */
	d = crfs_read_block(dirnode->in_first, buf);

	/* try to find the entry */
	r = crfs_lookup_dirent(d, fname, strlen(fname), &de);
	if (r < 0) {
		crfs_inode in;
		in.in_inode = 0;
		in.in_fmask = 0;
		return in;
	}

	return crfs_lookup_inode(de.de_inode);
}

/* 
 * Cache layer - NOTE: we cache CRFS blocks, not underlying blocks
 */

/* 
 * this will call read_block() from crfs_block_stdio.c or eeprom.c, 
 * depending on whether we build it in the ROM or as userland
 */
static void * 
crfs_read_block(block_id_t idx, void *buf)
{
	struct crfs_datum *head = crfs_block_cache;
	
	/* see if it is in the cache */
	while (head) {
		if (head->dt_blockaddr == idx) {
			return block_dup(buf, head->dt_data);
		}	
		head = head->dt_next;
	}

	if (!buf) {
		buf = malloc(CRFS_BLOCK_SIZE);
		if (!buf) {
			printf("malloc() failed at %s:%d\n", 
				__FILE__, __LINE__);
			return NULL;
		}
	}

	/* not cached - read it and add it */
	read_block(idx, buf);
	cache_add(idx, buf, 0);

	return buf;
}

static int 
crfs_write_block(block_id_t idx, void *data)
{
	struct crfs_datum *head = crfs_block_cache;
		
	/* see if it is in the list */
	while (head) {
		if (head->dt_blockaddr == idx) {
			/* found it */
			FSDBG("crfs_write_block: %d found cached, updated\n", idx);
			head->dt_data = block_dup(head->dt_data, data);
			head->dt_dirty = 1;
			return 0;
		}
		head = head->dt_next;
	}

	head = cache_add(idx, data, 1);
	head->dt_dirty = 1;

	return 0;
}

static struct crfs_datum *
cache_add(block_id_t idx, void *data, int dirty)
{
	struct crfs_datum *new;

	new = malloc(sizeof(*new));
	while (!new) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		new = malloc(sizeof(*new));
	}
	new->dt_data = block_dup(NULL, data);
	new->dt_blockaddr = idx;
	new->dt_next = crfs_block_cache;
	new->dt_dirty = dirty;
	crfs_block_cache = new;
	if (dirty) {
		FSDBG("cache_add: %d cached (%p, next=%p)\n", idx, new, 
			new->dt_next);
	}

	return new;
}

static int 
cache_flush(void)
{
	struct crfs_datum *head = crfs_block_cache;
	
	if (head) {
		while (head) {
			if (head->dt_dirty) {
				write_block(head->dt_blockaddr, head->dt_data);
				FSDBG("cache_flush: %d written\n", 
					head->dt_blockaddr);
			}
			crfs_block_cache = head->dt_next;
			free(head->dt_data);
			free(head);
			head = crfs_block_cache;
		}
		/* 
		 * toggle our idea of which superblock is active 
		 * this should provide us a bit of redundancy
		 */
		crfs_which_super = (crfs_which_super + 1) % CRFS_NSBLOCKS;
		
		/* increment the current superblock version num */
		super_block->sb_blockver++;
		super_block->sb_checksum = crfs_checksum(super_block);
		write_block(SB_BLOCKNUM(crfs_which_super), super_block);
	}	

	/* clear freemaps (sometimes we don't init them - like co_mkfs) */
	if (block_freemap) {
		freemap_flush(block_freemap);
	}
	if (inode_freemap) {
		freemap_flush(block_freemap);
	}

	FSDBG("cache_flush: done\n");

	return 0;
}

static int 
block_is_dirty(block_id_t idx)
{
	struct crfs_datum *head = crfs_block_cache;
	
	while (head) {
		if(head->dt_blockaddr == idx) {
			return head->dt_dirty;
		}
		head=head->dt_next;
	}
	
	return 0;
}

/*
 * exported functions
 */

/* 
 * crfs_init
 *
 * read the FS, store metadata in memory 
 *
 * returns:
 *	0 on success
 *	-1 on error
 */
int 
crfs_init(void)
{
	int i;
	crfs_super_block *sblocks[CRFS_NSBLOCKS];
	int nsblocks = 0;
	unsigned char buf[CRFS_BLOCK_SIZE];
	uint32 sbver = 0;
	
	crfs_block_cache = NULL;
	
	/* read the superblocks */
	for (i = 0; i < CRFS_NSBLOCKS; i++) {
		crfs_super_block *sb;

		sb = crfs_read_block(SB_BLOCKNUM(i), buf);
		if (sb->sb_checksum == crfs_checksum(sb) 
		 && !strcmp(sb->sb_magic, "CRfs")) {
		 	/* keep the valid ones */
			sblocks[nsblocks++] = block_dup(NULL, sb);
		}
	}
	FSDBG("crfs_init: found %d valid superblocks\n", nsblocks);
	
	/* look for the best one */
	for (i = 0; i < nsblocks; i++) {
		if (sblocks[i]->sb_blockver > sbver) {
			crfs_which_super = i;
			sbver = sblocks[i]->sb_blockver;
		}
	}
	if (crfs_which_super >= 0) {
		super_block = block_dup(NULL, sblocks[crfs_which_super]);
	}
	
	/* cleanup */
	for (i = 0; i < nsblocks; i++) {
		free(sblocks[i]);
	}
	if (crfs_which_super < 0) {
		printf("ERROR: no valid CRFS superblocks were found\n");
		return -1;
	}

	/* make the freemaps */
	i = freemap_init(block_freemap,
		(super_block->sb_fssize/CRFS_BLOCK_SIZE)/CHAR_BIT);
	i |= freemap_init(inode_freemap, (USHRT_MAX + 1)/CHAR_BIT);
	if (i) {
		return -1;
	}

	/* mark inode 0 - it is reserved */
	freemap_mark(inode_freemap, 0); 

	/* mark the super-blocks as used */
	for (i = 0; i < CRFS_NSBLOCKS; i++) {
		freemap_mark(block_freemap, SB_BLOCKNUM(i)); 
	}

	/* mark as in-use any blocks holding inode tables */
	for (i = 0; i < CRFS_SB_VTABLE_LEN; i++) {
		if (super_block->sb_vtable[i]) {
			freemap_mark(block_freemap, super_block->sb_vtable[i]);
		}
	}

	/* mark the root dir inode as in-use */
	mark_inode(super_block->sb_root);
	
	/* gotta start somewhere */
	crfs_pwd = super_block->sb_root;
	
	return 0;	
}

/*
 * crfs_create
 *
 * create a new file with zero size
 *
 * returns:
 * 	0 on success
 *	-1 on error, with errno set
 * errno values:
 *	EISDIR if the requested file is a dir
 *	EEXIST if the requested file exists
 *	ENAMETOOLONG if the request file's name is > CRFS_FILENAME_MAX
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR an element of the path used as a dierctory is not
 *	ENOMEM if a memory allocation could not be satisfied
 *	ENOSPC if there is no space available
 */
int 
crfs_create(char *fname)
{
	crfs_inode dirnode;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int i;
	int r;
	block_id_t dirblock;
	crfs_directory *d;
	crfs_dirent de;
	
	/* get the requested directory */
	r = crfs_namei(&fname, &dirnode);
	if (r < 0) {
		errno = -r;
		return -1;
	}

	/* check filename length */
	if (strlen(fname) > CRFS_FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* check if it is a dir */
	if (is_dir(&dirnode, fname)) {
		errno = EISDIR;
		return -1;
	}

	/* read the dir block */
	dirblock = dirnode.in_first;		
	d = crfs_read_block(dirblock, buf);
	r = crfs_lookup_dirent(d, fname, strlen(fname), &de);
	if (r == 0) {
		errno = EEXIST;
		return -1;
	}
	
	/* for each dirent - find an unused one */
	for (i = 0; i < CRFS_DIR_NENTS; i++) {
		if (d->d_entries[i].de_inode == 0) {
			int inum;
			crfs_inode inode;

			/* make a new inode */
			inum = freemap_next(inode_freemap);
			inode.in_inode = inum;
			inode.in_first = 0;
			inode.in_last = 0;
			inode.in_fsize = 0;
			inode.in_fmask = CRFS_FMASK_FILE;

			/* write the new inode */
			r = crfs_write_inode(&inode);
			if (r < 0) {
				errno = -r;
				return -1;
			}

			/* phased-commit, see comment @top of file */
			if (!block_is_dirty(dirblock)) {
				uint16 dirblock2;

				dirblock2 = freemap_next(block_freemap);
				freemap_unmark(block_freemap, dirblock);
				
				dirblock = dirblock2;
				
				dirnode.in_first = dirblock;
				dirnode.in_last = dirblock;
				r = crfs_write_inode(&dirnode);
				if (r < 0) {
					errno = -r;
					return -1;
				}
			}
			
			/* populate the dirent for this inode */
			d->d_entries[i].de_inode = inum;
			strncpy(d->d_entries[i].de_name, fname,
				CRFS_FILENAME_MAX);
			d->d_entries[i].de_name[CRFS_FILENAME_MAX]='\0';

			/* save this dir */
			crfs_write_block(dirblock, d);
			
			return 0;
		}
	}

	errno = ENOSPC;
	return -1;
}

/* 
 * crfs_open
 *
 * open a file
 *
 * returns:
 * 	a pointer to a CO_FILE on success
 *	NULL on error, with errno set
 * errno values:
 *	EISDIR if the requested file is a dir
 *	ENAMETOOLONG if the request file's name is > CRFS_FILENAME_MAX
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR an element of the path used as a dierctory is not
 *	ENOMEM if a memory allocation could not be satisfied
 */ 	
CO_FILE *
crfs_open(char *fname)
{
	crfs_inode dirnode;
	crfs_inode inode;
	CO_FILE *file;
	crfs_dirent de;
	block_id_t block_index;
	crfs_directory *d;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int r;
	
        /* get the requested directory */
	r = crfs_namei(&fname, &dirnode);
	if (r < 0) {
		errno = -r;
		return NULL;
	}

	/* check filename length */
	if (strlen(fname) > CRFS_FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	/* check if it is a dir */
	if (is_dir(&dirnode, fname)) {
		errno = EISDIR;
		return NULL;
	}

	/* read the directory */
	block_index = dirnode.in_first;
	d = crfs_read_block(block_index, buf); 	

	/* try to find the entry */
	r = crfs_lookup_dirent(d, fname, strlen(fname), &de);
	if (r < 0) {
		errno = ENOENT;
		return NULL;
	}

	/* read the dirent-pointed inode */
	inode = crfs_lookup_inode(de.de_inode);
	
	/* make a CO_FILE */
	file = malloc(sizeof(*file));
	if (!file) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		errno = ENOMEM;
		return NULL;
	}

	file->F_magic = CO_FILE_MAGIC;
	file->F_inode = inode.in_inode;
	file->F_curblock = inode.in_first;
	file->F_fpos = 0;
	file->F_nospc = 0;
	file->F_fblock = malloc(sizeof(*file->F_fblock));
	if (!file->F_fblock) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		errno = ENOMEM;
		free(file);
		return NULL;
	}

	/* if there is a first block, load it */
	if (file->F_curblock) {
		crfs_read_block(file->F_curblock, file->F_fblock);
	} else {
		memset(file->F_fblock, 0, sizeof(*file->F_fblock));
	}

	return file;	
}

/*
 * crfs_flush
 *
 * flush the CRFS block cache to the backing store
 */
int 
crfs_flush(void)
{
	/* global cache flush */
	cache_flush();

	return 0;
}

/*
 * crfs_close
 *
 * close an open CO_FILE
 *
 * returns:
 *	0 on success
 *	-1 on error, with errno set
 * errno values:
 *	EBADF if the passed in pointer was not a valid CO_FILE pointer
 */
int 
crfs_close(CO_FILE *fp)
{
	if (fp->F_magic != CO_FILE_MAGIC) {
		errno = EBADF;
		return -1;
	}

	crfs_flush();
	fp->F_magic = 0;
	free(fp->F_fblock);
	free(fp);

	return 0;
}

/*
 * crfs_read
 * 
 * read some data from a CO_FILE
 *
 * returns:
 * 	0 on EOF
 * 	number of bytes read on success
 *	-1 on error, with errno set
 * errno values:
 *	EBADF if the passed in pointer was not a valid CO_FILE pointer
 */
int 
crfs_read(CO_FILE *fp, void *buff, size_t size)
{
	int nread = 0;
	crfs_inode inode;
	unsigned int fboff;
	unsigned int fbleft;
	char *cbuf = (char *)buff;
	
	FSDBG("Entering crfs_read()\n");

	/* check magic */
	if (fp->F_magic != CO_FILE_MAGIC) {
		errno = EBADF;
		return -1;
	}

	/* get the inode for meta-data */
	inode = crfs_lookup_inode(fp->F_inode);

	/* check for eof */
	if (fp->F_fpos >= inode.in_fsize) {
		return 0;
	}
	
	/* check to see if read is over the EOF, if so limit the read */
	if ((fp->F_fpos + size) > inode.in_fsize) {
		size = inode.in_fsize - fp->F_fpos;
	}
	
	/* if we have a curblock, it is loaded */
	if (!fp->F_curblock) {
		/* no blocks! */
		return 0;
	}

	/* figure out our fblock offset */
	fboff = fp->F_fpos % CRFS_FBLOCK_DATA;
	fbleft = CRFS_FBLOCK_DATA - fboff;

	/* do we want more than is this current fblock has left? */
	if (size >= fbleft) {
		/* add all that is left from this block */
		memcpy(cbuf, fp->F_fblock->fb_data + fboff, fbleft);
		cbuf += fbleft;
		size -= fbleft;
		nread += fbleft;
		fp->F_fpos += fbleft;
		fboff = 0;
				
		/* try to load the next block, see if we hit end of chain */
		if (next_fblock(fp) < 0 && size) {
			/* the chain ends early wrt fsize - VERY BAD */
			return 0; 
		}
		
		/* 
		 * reads should be block aligned at this point so we read 
		 * as many complete blocks as possible 
		 */
		while (size >= CRFS_FBLOCK_DATA) {
			memcpy(cbuf, fp->F_fblock->fb_data, CRFS_FBLOCK_DATA);
			cbuf += CRFS_FBLOCK_DATA;
			size -= CRFS_FBLOCK_DATA;
			nread += CRFS_FBLOCK_DATA;
			fp->F_fpos += CRFS_FBLOCK_DATA;
			
			/* try to load the next block, see if we hit end of chain */
			if (next_fblock(fp) < 0 && size) {
				/* the chain ends early - VERY BAD */
				return 0; 
			}
		}
	}
 
 	/* we may have some left to read */
	if (size) {
		memcpy(cbuf, fp->F_fblock->fb_data + fboff, size);
		nread += size;
		fp->F_fpos += size;
	}		

	FSDBG("Leaving crfs_read()\n");
	return nread;
}

/*
 * crfs_write
 * 
 * write some data to a CO_FILE
 *
 * returns:
 * 	0 on EOF
 * 	number of bytes written on success
 * 	-1 on failure, with ernro set
 * errno values:
 * 	ENOSPC if there is no space available
 * 	EBADF if the passed in pointer is not a valid CO_FILE pointer
 */
 //FIXME: this doesn't do phased commit
int 
crfs_write(CO_FILE *fp, void *buff, size_t size)
{
	int nwrote = 0;
	crfs_inode inode;
	unsigned int fboff;
	unsigned int fbleft;
	char *cbuf = (char *)buff;
	int r;
	
	FSDBG("Entering crfs_write()\n");

	/* check magic */
	if (fp->F_magic != CO_FILE_MAGIC) {
		errno = EBADF;
		return -1;
	}

	/* if we don't have a curblock or couldn't get one before, get one */
	if (!fp->F_curblock || fp->F_nospc) {
		r = append_fblock(fp);
		if (r < 0) {
			errno = -r;
			return -1;
		}
	}

	/* figure out our fblock offset */
	fboff = fp->F_fpos % CRFS_FBLOCK_DATA;
	fbleft = CRFS_FBLOCK_DATA - fboff;
	
	/* do we want more than this current fblock has left? */
	if (size >= fbleft) {
		/* write as much as we can */
		memcpy(fp->F_fblock->fb_data + fboff, cbuf, fbleft);
		cbuf += fbleft;
		size -= fbleft;
		nwrote += fbleft;
		fp->F_fpos += fbleft;
		fboff = 0;
		
		/* we know we are done with this fblock - write it out */
		crfs_write_block(fp->F_curblock, fp->F_fblock);
		
		/* if we want to write more yet, append a new fblock */
		if (size && next_fblock(fp) < 0) {
			r = append_fblock(fp);
			if (r < 0) {
				errno = -r;
				nwrote = -1;
				size = 0;
			}
		}
		
		/*
		 * writes should be block aligned now - write as many complete
		 * blocks as we can
		 */
		while (size >= CRFS_FBLOCK_DATA) {
			memcpy(fp->F_fblock->fb_data, cbuf, CRFS_FBLOCK_DATA);
			cbuf += CRFS_FBLOCK_DATA;
			size -= CRFS_FBLOCK_DATA;
			nwrote += CRFS_FBLOCK_DATA;
			fp->F_fpos += CRFS_FBLOCK_DATA;

			/* this fblock is full - write it out */
			crfs_write_block(fp->F_curblock, fp->F_fblock);
			
			/* if we want to write more yet, append a new fblock */
			if (size && next_fblock(fp) < 0) {
				r = append_fblock(fp);
				if (r < 0) {
					errno = -r;
					nwrote = -1;
					size = 0;
				}
			}
		}
	}
	
	/* we may have some left to write */
	if (size) {
		memcpy(fp->F_fblock->fb_data + fboff, cbuf, size);
		nwrote += size;
		fp->F_fpos += size;
	}
	crfs_write_block(fp->F_curblock, fp->F_fblock);
	
	/* get the inode for meta-data */
	inode = crfs_lookup_inode(fp->F_inode);

	/* update the file length */
	if (inode.in_fsize < fp->F_fpos) {
		inode.in_fsize = fp->F_fpos;
		crfs_write_inode(&inode);
	}
	
	FSDBG("Leaving crfs_write()\n");
	return nwrote;
}

/* unimplemented */
int 
crfs_seek(CO_FILE *fp, int delta, int whence)
{
	/* check magic */
	if (fp->F_magic != CO_FILE_MAGIC) {
		errno = EBADF;
		return -1;
	}

	if (whence != CRFS_SEEK_SET 
	 && whence != CRFS_SEEK_CUR
	 && whence != CRFS_SEEK_END) {
	 	errno = EINVAL;
		return -1;
	}

	delta = 0; errno = ENOSYS; return -1;
	return 0;
}

/*
 * crfs_mkdir
 *
 * create a new directory
 *
 * returns:
 * 	0 on success
 *	-1 on error, with errno set
 * errno values:
 *	EEXIST if the requested file exists
 *	ENAMETOOLONG if the request file's name is > CRFS_FILENAME_MAX
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR an element of the path used as a dierctory is not
 *	ENOMEM if a memory allocation could not be satisfied
 *	ENOSPC if there is no space available
 */
int
crfs_mkdir(char *path)
{
	crfs_inode dirnode;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int i;
	int r;
	block_id_t dirblock;
	crfs_directory *d;
	crfs_dirent de;
	
	/* get the requested directory */
	r = crfs_namei(&path, &dirnode);
	if (r < 0) {
		errno = -r;
		return -1;
	}

	/* check filename length */
	if (strlen(path) > CRFS_FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* read the dir block */
	dirblock = dirnode.in_first;		
	d = crfs_read_block(dirblock, buf);
	r = crfs_lookup_dirent(d, path, strlen(path), &de);
	if (r == 0) {
		errno = EEXIST;
		return -1;
	}
	
	/* for each dirent - find an unused one */
	for (i = 0; i < CRFS_DIR_NENTS; i++) {
		if (d->d_entries[i].de_inode == 0) {
			int inum;
			int blocknum;
			crfs_inode inode;
			crfs_directory newdir;

			/* make a new inode */
			inum = freemap_next(inode_freemap);
			blocknum = freemap_next(block_freemap);
			inode.in_inode = inum;
			inode.in_first = blocknum;
			inode.in_last = blocknum;
			inode.in_fsize = 0;
			inode.in_fmask = CRFS_FMASK_DIR;

			/* make a new crfs_directory */
			memset(&newdir, 0, sizeof(newdir));
			newdir.d_entries[0].de_inode = inum;
			strcpy(newdir.d_entries[0].de_name, ".");
			newdir.d_entries[1].de_inode = dirnode.in_inode;
			strcpy(newdir.d_entries[1].de_name, "..");
			crfs_write_block(blocknum, &newdir);

			/* write the new inode */
			r = crfs_write_inode(&inode);
			if (r < 0) {
				errno = -r;
				return -1;
			}

			/* phased-commit, see comment @top of file */
			if (!block_is_dirty(dirblock)) {
				uint16 dirblock2;

				dirblock2 = freemap_next(block_freemap);
				freemap_unmark(block_freemap, dirblock);
				
				dirblock = dirblock2;
				
				dirnode.in_first = dirblock;
				dirnode.in_last = dirblock;
				r = crfs_write_inode(&dirnode);
				if (r < 0) {
					errno = -r;
					return -1;
				}
			}
			
			/* populate the dirent for this inode */
			d->d_entries[i].de_inode = inum;
			strncpy(d->d_entries[i].de_name, path,
				CRFS_FILENAME_MAX);
			d->d_entries[i].de_name[CRFS_FILENAME_MAX]='\0';

			/* save this dir */
			crfs_write_block(dirblock, d);
			
			return 0;
		}
	}

	errno = ENOSPC;
	return -1;
}

/*
 * crfs_opendir
 * 
 * open a directory, get a CO_DIR handle to it
 *
 * returns:
 *	a pointer to a CO_DIR on success
 * 	NULL on error, with errno set
 * errno values:
 * 	ENOMEM if a memory allocation failed
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR if an element of the path used as a dir is not
 *	ENAMETOOLONG if the dir name is too long
 */
CO_DIR *
crfs_opendir(char *dname)
{
	crfs_inode dirnode;
	CO_DIR *dir;
	int r;
	
        /* get the requested directory */
	r = crfs_namei(&dname, &dirnode);
	if (r < 0) {
		errno = -r;
		return NULL;
	}

	/* check name length */
	if (strlen(dname) > CRFS_FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	dirnode = inode_lookup_from_namei(&dirnode, dname);
	if (!(dirnode.in_fmask & CRFS_FMASK_DIR)) {
		errno = ENOTDIR;
		return NULL;
	}

	dir = malloc(sizeof(*dir));
	if (!dir) {
		printf("malloc() failed at %s:%d\n", __FILE__, __LINE__);
		errno = ENOMEM;
		return NULL;
	}
	dir->D_magic = CO_DIR_MAGIC;
	dir->D_inode = dirnode.in_inode;
	dir->D_pos = 0;
		
	return dir;
}

/* 
 * crfs_readdir
 *
 * get a directory entry from a CO_DIR
 *
 * returns:
 * 	a pointer to a crfs_dirent on success
 *	NULL at EOD
 *	-1 on error, with errno set
 * errno values:
 *	EBADF if the CO_DIR pointer is invalid
 */
crfs_dirent * 
crfs_readdir(CO_DIR *dir)
{
	crfs_directory *dirblock;
	crfs_inode inode;
	unsigned char buf[CRFS_BLOCK_SIZE];
	
	/* check magic */
	if (dir->D_magic != CO_DIR_MAGIC) {
		errno = EBADF;
		return NULL;
	}

	inode = crfs_lookup_inode(dir->D_inode);
	dirblock = crfs_read_block(inode.in_first, buf);
	
	while ((dir->D_pos < CRFS_DIR_NENTS) 
	 && (dirblock->d_entries[dir->D_pos].de_inode == 0)) {
		dir->D_pos++;
	}
		
	if (dir->D_pos >= CRFS_DIR_NENTS) {
		return NULL;
	}

	FSDBG("crfs_readdir: inode %d, block %d, fsize %d, pos 0x%x\n", 
		inode.in_inode, inode.in_first, inode.in_fsize, dir->D_pos);

	dir->D_dirent = dirblock->d_entries[dir->D_pos];
	dir->D_pos++;

	return &dir->D_dirent;
}

/* 
 * crfs_closedir
 *
 * close a CO_DIR
 *
 * returns:
 * 	a pointer to a crfs_dirent on success
 *	NULL at EOD
 *	-1 on error, with errno set
 * errno values:
 *	EBADF if the CO_DIR pointer is invalid
 */
int 
crfs_closedir(CO_DIR *dir)
{
	/* check magic */
	if (dir->D_magic != CO_DIR_MAGIC) {
		errno = EBADF;
		return -1;
	}

	dir->D_magic = 0;
	free(dir);
	return 0;
}

/*
 * crfs_mkfs
 *
 * make a CRFS filesystem
 */
int
crfs_mkfs(size_t size)
{
	crfs_super_block sblock;
	unsigned char buf[CRFS_BLOCK_SIZE];
	crfs_itable *itable = (crfs_itable *)buf;
	crfs_directory *dir = (crfs_directory *)buf;
	int i;
	
	/* round size down to a multiple of the block size */
	size &= ~CRFS_BLOCK_SIZE;

	/* clear the super block */
	memset(&sblock, 0, sizeof(sblock));

	/* Set up primary super block  */
	sblock.sb_magic[0] = 'C';
	sblock.sb_magic[1] = 'R';
	sblock.sb_magic[2] = 'f';
	sblock.sb_magic[3] = 's';
	sblock.sb_fsver = 0;
	sblock.sb_checksum = 0;
	sblock.sb_fssize = size;
	sblock.sb_root = 1; /* root dir is on inode 1 */
	sblock.sb_vtable[0] = 2; /* first itable is block 2 */

	/* set up the super blocks */
	for (i = 0; i < CRFS_NSBLOCKS; i++) {
		sblock.sb_blockver = i;
		sblock.sb_checksum = crfs_checksum(&sblock);
		crfs_write_block(SB_BLOCKNUM(i), &sblock);
	}

	/* use the last superblock now, flush() will rotate this value */
	crfs_which_super = CRFS_NSBLOCKS-1;
	super_block = block_dup(NULL, &sblock);
	
	/* Set up first inode table */
	memset(itable, 0, sizeof(*itable));
	itable->it_inodes[1].in_inode = 1; /* root dir */
	itable->it_inodes[1].in_first = 3; /* root dir is at block 3 */
	itable->it_inodes[1].in_last = 3;
	itable->it_inodes[1].in_fsize = 0;
	itable->it_inodes[1].in_fmask = CRFS_FMASK_DIR;
	crfs_write_block(2, itable);

	/* set up root directory */	
	memset(dir, 0, sizeof(*dir));
	dir->d_entries[0].de_inode = 1;
	strcpy(dir->d_entries[0].de_name, ".");
	dir->d_entries[1].de_inode = 1;
	strcpy(dir->d_entries[1].de_name, "..");
	crfs_write_block(3, dir);
	
	/* sync */
	cache_flush();

	return 0;
}	

/*
 * crfs_stat:
 *
 * get info about a file
 *
 * returns:
 *	0 on success
 *	-1 on failure, with errno set
 * errno values:
 *	ENOENT if the file does not exist
 *	ENAMETOOLONG if the requested filename is too long
 *	ENOTDIR if an element of the path used as a dir is not
 */
int
crfs_stat(char *fname, struct crfs_stat *stbuf)
{
	crfs_inode dirnode;
	crfs_inode inode;
	crfs_dirent de;
	block_id_t block_index;
	crfs_directory *d;
	unsigned char buf[CRFS_BLOCK_SIZE];
	int r;
	
        /* get the requested directory */
	r = crfs_namei(&fname, &dirnode);
	if (r < 0) {
		errno = -r;
		return -1;
	}

	/* check filename length */
	if (strlen(fname) > CRFS_FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* read the directory */
	block_index = dirnode.in_first;
	d = crfs_read_block(block_index, buf); 	

	/* try to find the entry */
	r = crfs_lookup_dirent(d, fname, strlen(fname), &de);
	if (r < 0) {
		errno = ENOENT;
		return -1;
	}
	/* read the dirent-pointed inode */
	inode = crfs_lookup_inode(de.de_inode);
	
	stbuf->st_ino = de.de_inode;
	stbuf->st_fmask = inode.in_fmask;
	stbuf->st_size = inode.in_fsize;

	return 0;
}

/* 
 * crfs_remove
 *
 * delete a file
 *
 * returns:
 * 	0 on success
 *	-1 on error, with errno set
 * errno values:
 *	EISDIR if the requested file is a dir
 *	ENAMETOOLONG if the request file's name is > CRFS_FILENAME_MAX
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR an element of the path used as a dierctory is not
 *	ENOMEM if a memory allocation could not be satisfied
 */ 	
int 
crfs_remove(char *fname)
{
	int r;

	r = rm_file(fname, 0);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

/* 
 * crfs_rmdir
 *
 * delete a directory (recurses - assume rm -rf)
 *
 * returns:
 * 	0 on success
 *	-1 on error, with errno set
 * errno values:
 *	ENAMETOOLONG if the request file's name is > CRFS_FILENAME_MAX
 *	ENOENT if an element of the path does not exist
 *	ENOTDIR an element of the path used as a dierctory is not
 *	ENOMEM if a memory allocation could not be satisfied
 */ 
int 
crfs_rmdir(char *dname)
{
	int r;

	r = rm_file(dname, 1);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
