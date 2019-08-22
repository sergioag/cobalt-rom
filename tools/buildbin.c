/* $Id: buildbin.c,v 1.1.1.1 2003/06/10 22:41:37 iceblink Exp $ */

/*
 * 
 * Filename: buildrom.c
 * 
 * Description: Code for building an image from multiple files
 * 
 * Author(s): Tim Hockin
 * 
 * Copyright 1997-2000 Cobalt Networks, Inc.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int cvt_int(char *numstr, int *errp);
void usage(void);
int parse_cmdline(int argc, char *argv[]);
struct section *mk_section(char *offset, char *file);
int insert(struct section *new);
void dump();

struct section {
	unsigned int offset;
	char *filename;
	unsigned int size;
	struct section *next;
};

char *progname;
char *outfile;
int debug;
struct section *sections;
int imglen;
int stdoutflag;

int 
main(int argc, char *argv[])
{
	unsigned char *rom;
	int nsecs;
	int fdout;
	int fdin;
	int pos = 0;
	struct section *cur;
	unsigned char ff = 0xff;
	
	nsecs = parse_cmdline(argc, argv);
	if (nsecs <= 0) { 
		usage();
		exit(1);
	}
	dump();


	rom = malloc(imglen);
	if (!rom) {
		perror("malloc");
		exit(16);
	}
	memset(rom, 0, imglen);

	fdout = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fdout < 0) {
		fprintf(stderr, "open(%s): %s\n", outfile, strerror(errno));
		exit(17);
	}

	/* write each section */
	cur = sections;
	while (cur) {
		int i;
		unsigned char c;

		while (pos < cur->offset) {
			write(fdout, &ff, 1);
			pos++;
		}
		
		fdin = open(cur->filename, O_RDONLY);
		if (fdin < 0) {
			fprintf(stderr, "open(%s): %s\n", cur->filename, 
				strerror(errno));
			exit(18);
		}

		/* read only as many bytes as we stat()ed earlier */
		for (i = 0; i < cur->size; i++) {
			if (read(fdin, &c, 1) != 1) {
				fprintf(stderr, "read(%s): %s\n", cur->filename, 
					strerror(errno));
				exit(18);
			}
			write(fdout, &c, 1);
		}
		pos += i;
		close(fdin);

		cur = cur->next;
	}

	while (pos < imglen) {
		write(fdout, &ff, 1);
		pos++;
	}
	close(fdout);

	if (stdoutflag) {
		printf("\n");
	}

	printf("-------------------------------\n");
	printf("Total       : %ld bytes.\n", pos);
	printf("\n");
	
	return 0;
}

int 
cvt_int(char *numstr, int *errp)
{               
	long val;   
	char *cvt;

	val = strtol(numstr, &cvt, 0);

	if (cvt == numstr) {
		*errp = 1;  
		return 0;       
	}               

	switch (*cvt) {  
		case 'm': 
		case 'M':
			val *= 1024 * 1024;
			break;
	
		case 'k': 
		case 'K':
			val *= 1024;
			break;
	}

	*errp = 0;
	return val;
}

void 
usage(void)
{
	fprintf(stderr, 
		"usage: %s -o <outfile> -l <image size> <sections>\n"
		"       section: -s <offset> <file>\n\n", progname);
}

int
parse_cmdline(int argc, char *argv[])
{
	int nsecs = 0;
	
	progname = argv[0];
	argc--; argv++;

	if (!argc) {
		usage();
		exit(1);
	}

	while (argc && argv[0][0] == '-') {
		switch (argv[0][1]) {
		  case 'd': {
		  	/* debug */
			argc--; argv++;
			debug++;
			break;
		  }
		  case 'l': {
			/* image size */
			int err = 0;
		  	argc--; argv++;
		  	if (imglen || !argc) {
				usage();
				exit(1);
			}

			imglen = cvt_int(argv[0], &err);
			if (err) {
				usage();
				exit(1);
			}
		  	argc--; argv++;
		  	break;
		  }
		  case 'o': {
		  	/* output file */
		  	argc--; argv++;
		  	if (outfile || !argc) {
				usage();
				exit(1);
			}

			outfile = argv[0];
		  	argc--; argv++;
		  	break;
		  }
		  case 's': {
			/* new section */
		  	struct section *newsec;
			char *off, *file;
		  	argc--; argv++;
		  	if (!argc) {
				usage();
				exit(1);
			}

			off = argv[0];
		  	argc--; argv++;
		  	if (!argc) {
				usage();
				exit(1);
			}
			file = argv[0];
		  	argc--; argv++;

			newsec = mk_section(off, file);
			if (!newsec) {
				usage();
				exit(1);
			}
		  	
			if (insert(newsec) < 0) {
				exit(2);
			}
			nsecs++;
		  	break;
		  }
		  default:
		  	usage();
			exit(1);
			break;
		}
	}

	/* should be no leftovers */
	if (argc) {
		usage();
		exit(1);
	}

	/* last sanity checks */
	if (!outfile || !strcmp(outfile, "-")) {
		outfile = "/dev/stdout";
		stdoutflag = 1;
	}

	if (imglen <= 0) {
		fprintf(stderr, "must specify image size > 0\n");
		exit(3);
	}

	{
		struct section *p = sections;
		while (p->next) {
			p = p->next;
		}
		if (p->offset + p->size >= imglen) {
			fprintf(stderr, "sections are larger than image size\n");
			exit(4);
		}
	}
			

	return nsecs;
}

struct section *
mk_section(char *offset, char *file)
{
	struct section *new;
	int off;
	int err = 0;
	struct stat sbuf;

	off = cvt_int(offset, &err);
	if (err) {
		fprintf(stderr, "numeric conversion error: \'%s\'\n", offset);
		return NULL;
	}

	if (stat(file, &sbuf)) {
		fprintf(stderr, "file not found: \'%s\'\n", file);
		return NULL;
	}

	new = malloc(sizeof(*new));
	if (!new) {
		perror("malloc");
		return NULL;
	}

	new->offset = off;
	new->filename = file;
	new->size = sbuf.st_size;
	new->next = NULL;

	return new;
}

int
insert(struct section *new)
{
	struct section *cur = sections;
	struct section *prev = NULL;
	
	if (!sections) {
		sections = new;
		return 0;
	}

	while (cur) {
		if (new->offset >= cur->offset + cur->size) {
			/* keep looking */
			prev = cur;
			cur = cur->next;
		} else if ((new->offset > cur->offset) 
		  || (new->offset + new->size > cur->offset)) {
			/* collision */
			fprintf(stderr, "files %s and %s overlap\n", 
				cur->filename, new->filename);
			return -1;
		} else {
			break;
		}
	}

	/* found it */
	new->next = cur;
	prev->next = new;

	return 0;
}

void
dump()
{
	struct section *cur = sections;
	int n = 1;

	printf("sections:\n");
	while (cur) {
		printf("    %d: %s : 0x%08x - 0x%08x (%u bytes)\n", n++, 
			cur->filename, cur->offset, cur->offset + cur->size, 
			cur->size);
		cur = cur->next;
	}
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
