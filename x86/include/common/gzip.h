/* gzip.h -- common declarations for all gzip modules */
#ifndef COMMON_GZIP_H
#define COMMON_GZIP_H

#include <string.h>

#define memzero(s, n)     memset ((s), 0, (n))

/* Return codes from gzip */
#define OK      0
#define ERROR   1
#define WARNING 2

/* Compression methods (see algorithm.doc) */
#define STORED     0
#define COMPRESSED 1
#define PACKED     2
#define DEFLATED   8

extern unsigned long bytes_out;	/* # of uncompressed bytes */
extern int method;		/* compression method */

#define INBUFSIZ	1024	/* input buffer size */
#define WSIZE		0x8000	/* window size--must be a power of two, and */
				/*  at least 32K for zip's deflate method */

unsigned char *inbuf;	/* input buffer */
unsigned char *window;	/* sliding window and suffix table (unlzw) */

extern unsigned insize; /* valid bytes in inbuf */
extern unsigned inptr;  /* index of next byte to be processed in inbuf */
extern unsigned outcnt; /* bytes in output buffer */

#define	GZIP_MAGIC     "\037\213" /* Magic header for gzip files, 1F 8B */
#define	OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

/* internal file attribute */
#define UNKNOWN (-1)
#define BINARY  0
#define ASCII   1

/* The minimum and maximum match lengths */
#define MIN_MATCH  3
#define MAX_MATCH  258

/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1 */
#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)

/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE */
#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())
#define put_char(c) {window[outcnt++]=(unsigned char)(c); if (outcnt==WSIZE) flush_window();}

/* Macros for getting two-byte and four-byte header values */
#define SH(p) ((unsigned short)(unsigned char)((p)[0]) | ((unsigned short)(unsigned char)((p)[1]) << 8))
#define LG(p) ((unsigned long)(SH(p)) | ((unsigned long)(SH((p)+2)) << 16))

/* unzip.c */
extern int unzip (void);

/* misczip.c: */
extern unsigned long updcrc(unsigned char *s, unsigned n);
extern void error(char *m);
extern void flush_window(void);
extern void clear_bufs(void);
extern int fill_inbuf(void);

/* inflate.c */
extern int inflate (void);

#endif /* GZIP_H */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
