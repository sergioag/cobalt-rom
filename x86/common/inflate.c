#if 0
#define DEBG(x) printf("%s",x)
#define DEBGINT(x,y) printf("%s 0x%x ",x,y)
#else
#define DEBG(x)
#define DEBGINT(x,y)
#endif

/*
 * inflate.c -- Not copyrighted 1992 by Mark Adler
 * version c10p1, 10 January 1993
 */

/* 
 * Adapted for booting Linux by Hannu Savolainen 1993
 * based on gzip-1.0.3 
 */

#include <string.h>
#include <printf.h>
#include <stdlib.h>

#include "common/rom.h"
#include "common/gzip.h"

#define free(x)	
#define slide window

struct huft {
	unsigned char e;	/* number of extra bits or operation */
	unsigned char b;	/* number of bits in this code or subcode */
	union {
		unsigned short n;	/* literal, length base, or distance base */
		struct huft *t;	/* pointer to next level of table */
	} v;
};


/* Function prototypes */
int huft_build (unsigned *, unsigned, unsigned, unsigned short *,
		unsigned short *, struct huft **, int *);
int huft_free (struct huft *);
int inflate_codes (struct huft *, struct huft *, int, int);
int inflate_stored (void);
int inflate_fixed (void);
int inflate_dynamic (void);
int inflate_block (int *);
int inflate (void);

#define wp outcnt
#define flush_output(w) (wp=(w),flush_window())

unsigned border[19];
unsigned short cplens[31],cplext[31],cpdist[30],cpdext[30],mask_bits[17];

unsigned long bb;		/* bit buffer */
unsigned bk;			/* bits in bit buffer */
unsigned hufts;                 /* track memory usage */


#define NEXTBYTE()  (unsigned char)get_byte()
#define NEEDBITS(n) {while(k<(n)){b|=((unsigned long)NEXTBYTE())<<k;k+=8;}}
#define DUMPBITS(n) {b>>=(n);k-=(n);}

int lbits;          /* bits in base literal/length lookup table */
int dbits;          /* bits in base distance lookup table */


/*
 * If BMAX needs to be larger than 16, then h and x[] should be
 * unsigned long.
 */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */

void make_tables(void)
{
	unsigned short ctr;

	/* Tables for deflate from PKZIP's appnote.txt. */
	unsigned l_border[] = {
		/* Order of the bit length code lengths */
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	unsigned short l_cplens[] = {
		/* Copy lengths for literal codes 257..285 */
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
	};

	/* note: see note #13 above about the 258 in this list. */
	unsigned short l_cplext[] = {
		/* Extra bits for literal codes 257..285 */
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99	/* 99==invalid */
	};

	unsigned short l_cpdist[] = {
		/* Copy offsets for distance codes 0..29 */
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577
	};

	unsigned short l_cpdext[] = {
		/* Extra bits for distance codes */
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
		12, 12, 13, 13
	};

	unsigned short l_mask_bits[] = {
		0x0000,
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};

	lbits = 9;
	dbits = 6;
 
	for (ctr=0; ctr<19; ctr++)
		border[ ctr ] = l_border[ ctr ];
	for (ctr=0; ctr<31; ctr++)
		cplens[ ctr ] = l_cplens[ ctr ];
	for (ctr=0; ctr<31; ctr++)
		cplext[ ctr ] = l_cplext[ ctr ];
	for (ctr=0; ctr<30; ctr++)
		cpdist[ ctr ] = l_cpdist[ ctr ];
	for (ctr=0; ctr<30; ctr++)
		cpdext[ ctr ] = l_cpdext[ ctr ];
	for (ctr=0; ctr<17; ctr++)
		mask_bits[ ctr ] = l_mask_bits[ ctr ];
}


/*
 * Given a list of code lengths and a maximum table size, make a set
 * of tables to decode that set of codes.  Return zero on success, one
 * if the given code set is incomplete (the tables are still built in
 * this case), two if the input is invalid (all zero length codes or
 * an oversubscribed set of lengths), and three if not enough
 * memory.
 */
int
huft_build(b, n, s, d, e, t, m)
  unsigned *b;            /* code lengths in bits (all assumed <= BMAX) */
  unsigned n;             /* number of codes (assumed <= N_MAX) */
  unsigned s;             /* number of simple-valued codes (0..s-1) */
  unsigned short *d;                 /* list of base values for non-simple codes */
  unsigned short *e;                 /* list of extra bits for non-simple codes */
  struct huft **t;        /* result: starting table */
  int *m;                 /* maximum lookup bits, returns actual */
{
	unsigned a;                   /* counter for codes of length k */
	unsigned c[BMAX+1];           /* bit length count table */
	unsigned f;                   /* i repeats in table every f entries */
	int g;                        /* maximum code length */
	int h;                        /* table level */
	register unsigned i;          /* counter, current code */
	register unsigned j;          /* counter */
	register int k;               /* number of bits in current code */
	int l;                        /* bits per table (returned in m) */
	register unsigned *p;         /* pointer into c[], b[], or v[] */
	register struct huft *q;      /* points to current table */
	struct huft r;                /* table entry for structure assignment */
	struct huft *u[BMAX];         /* table stack */
	unsigned v[N_MAX];            /* values in order of bit length */
	register int w;               /* bits before this table == (l * h) */
	unsigned x[BMAX+1];           /* bit offsets, then code stack */
	unsigned *xp;                 /* pointer into x */
	int y;                        /* number of dummy codes added */
	unsigned z;                   /* number of entries in current table */

DEBG("huft1 ");
	/* Generate counts for each bit length */
	memzero(c, sizeof(c));
	p = b;  i = n;
	do {
		c[*p++]++;                  /* assume all entries <= BMAX */
	} while (--i);

	/* null input--all zero length codes */
	if (c[0] == n) {
		*t = (struct huft *)NULL;
		*m = 0;
		return 0;
	}

DEBG("huft2 ");
	/* Find minimum and maximum length, bound *m by those */
	l = *m;
	for (j = 1; j <= BMAX; j++)
		if (c[j])
			break;

	/* minimum code length */
	k = j;
	if ((unsigned)l < j)
		l = j;
	for (i = BMAX; i; i--)
		if (c[i])
			break;

	/* maximum code length */
	g = i;
	if ((unsigned)l > i)
		l = i;
	*m = l;

DEBG("huft3 ");
	/* Adjust last length count to fill out codes, if needed */
	for (y = 1 << j; j < i; j++, y <<= 1)
		if ((y -= c[j]) < 0)
			return 2;                 /* bad input: more codes than bits */
	if ((y -= c[i]) < 0)
		return 2;
	c[i] += y;

DEBG("huft4 ");
	/* Generate starting offsets into the value table for each length */
	x[1] = j = 0;
	p = c + 1;  xp = x + 2;
	while (--i) {                 /* note that i == g from above */
		*xp++ = (j += *p++);
	}

DEBG("huft5 ");
	/* Make a table of values in order of bit lengths */
	p = b; i = 0;
	do {
		if ((j = *p++) != 0)
			v[x[j]++] = i;
	} while (++i < n);

DEBG("h6 ");
	/* Generate the Huffman codes and for each, make the table entries */
	x[0] = i = 0;                 /* first Huffman code is zero */
	p = v;                        /* grab values in bit order */
	h = -1;                       /* no tables yet--level -1 */
	w = -l;                       /* bits decoded == (l * h) */
	u[0] = (struct huft *)NULL;   /* just to keep compilers happy */
	q = (struct huft *)NULL;      /* ditto */
	z = 0;                        /* ditto */

DEBG("h6a ");
	/* go through the bit lengths (k already is bits in shortest code) */
	for (; k <= g; k++)
	{
DEBG("h6b ");
		a = c[k];
		while (a--)
		{
DEBG("h6b1 ");
			/* here i is the Huffman code of length k bits for value *p */
			/* make tables up to required level */
			while (k > w + l)
			{
DEBG("1 ");
				h++;
				w += l;                 /* previous table always l bits */

				/* compute minimum size table less than or equal to l bits */
				z = ((unsigned)z = (unsigned)g - (unsigned)w) > (unsigned)l ? l : z;  /* upper limit on table size */
				if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
				{                       /* too few codes for k-w bit table */
DEBG("2 ");
					f -= a + 1;           /* deduct codes from patterns left */
					xp = c + k;
					while (++j < z)       /* try smaller tables up to z bits */
					{
						if ((f <<= 1) <= *++xp)
							break;            /* enough codes to use up j bits */
						f -= *xp;           /* else deduct codes from patterns */
					}
				}
DEBG("3 ");
				z = 1 << j;             /* table entries for j-bit table */

				/* allocate and link in new table */
				if ((q = (struct huft *)malloc((z + 1)*sizeof(struct huft))) == 0) {
					if (h)
						huft_free(u[0]);
					return 3;       /* not enough memory */
				}
DEBG("4 ");
				hufts += z + 1;         /* track memory usage */
				*t = q + 1;             /* link to list for huft_free() */
				*(t = &(q->v.t)) = (struct huft *)NULL;
				u[h] = ++q;             /* table starts after link */

DEBG("5 ");
				/* connect to last table, if there is one */
				if (h)
				{
					x[h] = i;             /* save pattern for backing up */
					r.b = (unsigned char)l;         /* bits to dump before this table */
					r.e = (unsigned char)(16 + j);  /* bits in this table */
					r.v.t = q;            /* pointer to this table */
					j = i >> (w - l);     /* (get around Turbo C bug) */
					u[h-1][j] = r;        /* connect to last table */
				}
DEBG("6 ");
			}
DEBG("h6c ");

			/* set up table entry in r */
			r.b = (unsigned char)(k - w);
			if (p >= v + n)
				r.e = 99;               /* out of values--invalid code */
			else if (*p < s)
			{
				r.e = (unsigned char)(*p < 256 ? 16 : 15);    /* 256 is end-of-block code */
				r.v.n = *p++;           /* simple code is just the value */
			}
			else
			{
				r.e = (unsigned char)e[*p - s];   /* non-simple--look up in lists */
				r.v.n = d[*p++ - s];
			}
DEBG("h6d ");
      
			/* fill code-like entries with r */
			f = 1 << (k - w);
			for (j = i >> w; j < z; j += f)
				q[j] = r;

			/* backwards increment the k-bit code i */
			for (j = 1 << (k - 1); i & j; j >>= 1)
				i ^= j;
			i ^= j;

			/* backup over finished tables */
			while ((i & ((1 << w) - 1)) != x[h])
			{
				h--;                    /* don't need to update q */
				w -= l;
			}
DEBG("h6e ");
		}
DEBG("h6f ");
	}
DEBG("huft7 ");

	/* Return true (1) if we were given an incomplete table */
	return y != 0 && g != 1;
}



/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
int huft_free(t)
  struct huft *t;         /* table to free */
{
	register struct huft *p, *q;

	/* Go through linked list, freeing from the malloced (t[-1]) address. */
	p = t;
	while (p != (struct huft *)NULL) {
		q = (--p)->v.t;
		free(p);
		p = q;
	} 
	return 0;
}


/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
int inflate_codes(tl, td, bl, bd)
  struct huft *tl, *td;   /* literal/length and distance decoder tables */
  int bl, bd;             /* number of bits decoded by tl[] and td[] */
{
	register unsigned e;  /* table entry flag/number of extra bits */
	unsigned n, d;        /* length and index for copy */
	unsigned w;           /* current window position */
	struct huft *t;       /* pointer to table entry */
	unsigned ml, md;      /* masks for bl and bd bits */
	register unsigned long b;	/* bit buffer */
	register unsigned k;		/* number of bits in bit buffer */


	/* make local copies of globals */
	b = bb;                       /* initialize bit buffer */
	k = bk;
	w = wp;                       /* initialize window position */

	/* inflate the coded data */
	ml = mask_bits[bl];           /* precompute masks for speed */
	md = mask_bits[bd];
	for (;;)                      /* do until end of block */
	{
		NEEDBITS((unsigned)bl);
		if ((e = (t = tl + ((unsigned)b & ml))->e) > 16)
			do {
				if (e == 99)
					return 1;
				DUMPBITS(t->b);
				e -= 16;
				NEEDBITS(e);
			} while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);

		DUMPBITS(t->b);
		if (e == 16) {               /* then it's a literal */
DEBG("l ");
			slide[w++] = (unsigned char)t->v.n;
			if (w == WSIZE)	{
				flush_output(w);
				w = 0;
			}
		}
		else {                       /* it's an EOB or a length */
			/* exit if end of block */
			if (e == 15)
				break;

			/* get length of block to copy */
			NEEDBITS(e);
			n = t->v.n + ((unsigned)b & mask_bits[e]);
			DUMPBITS(e);

			/* decode distance of block to copy */
			NEEDBITS((unsigned)bd);
			if ((e = (t = td + ((unsigned)b & md))->e) > 16)
				do {
					if (e == 99)
						return 1;
					DUMPBITS(t->b);
					e -= 16;
					NEEDBITS(e);
				} while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
			DUMPBITS(t->b);
			NEEDBITS(e);
			d = w - t->v.n - ((unsigned)b & mask_bits[e]);
			DUMPBITS(e);

DEBG("D ");
DEBGINT("w-d is", w-d);
DEBGINT("n is", n);

			/* do the copy */
			do {
				n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
DEBG("memcpy d to w size e");
DEBGINT("d is", d);
DEBGINT("w is", w); 
DEBGINT("e is", e);
DEBG("\n");

#if !defined(NOMEMCPY) && !defined(DEBUG)
				if (w - d >= e) {        /* (this test assumes unsigned comparison) */
					memcopy(slide + w, slide + d, e);
					w += e;
					d += e;
				}
				else                      /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
					do {
						slide[w++] = slide[d++];
					} while (--e);
				if (w == WSIZE)	{
					flush_output(w);
					w = 0;
				}
			} while (n);
		}
	}

	/* restore the globals from the locals */
	wp = w;                       /* restore global window pointer */
	bb = b;                       /* restore global bit buffer */
	bk = k;

	/* done */
	return 0;
}


/* "decompress" an inflated type 0 (stored) block. */
int inflate_stored()
{
	unsigned n;           /* number of bytes in block */
	unsigned w;           /* current window position */
	register unsigned long b;	/* bit buffer */
	register unsigned k;		/* number of bits in bit buffer */

DEBG("<stor");

	/* make local copies of globals */
	b = bb;                       /* initialize bit buffer */
	k = bk;
	w = wp;                       /* initialize window position */

	/* go to byte boundary */
	n = k & 7;
	DUMPBITS(n);

	/* get the length and its complement */
	NEEDBITS(16);

	n = ((unsigned)b & 0xffff);

	DUMPBITS(16);
	NEEDBITS(16);

	if (n != (unsigned)((~b) & 0xffff))
		return 1;                   /* error in compressed data */

	DUMPBITS(16);

	/* read and output the compressed data */
	while (n--) {
		NEEDBITS(8);
		slide[w++] = (unsigned char)b;
		if (w == WSIZE)	{
			flush_output(w);
			w = 0;
		}
		DUMPBITS(8);
	}

	/* restore the globals from the locals */
	wp = w;                       /* restore global window pointer */
	bb = b;                       /* restore global bit buffer */
	bk = k;

DEBG(">");
	/* done */
	return 0;
}

/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
int inflate_fixed()
{
	int i;                /* temporary variable */
	struct huft *tl;      /* literal/length code table */
	struct huft *td;      /* distance code table */
	int bl;               /* lookup bits for tl */
	int bd;               /* lookup bits for td */
	unsigned l[288];      /* length list for huft_build */

DEBG("<fix");
	/* set up literal table */
	for (i = 0; i < 144; i++)
		l[i] = 8;
	for (; i < 256; i++)
		l[i] = 9;
	for (; i < 280; i++)
		l[i] = 7;
	for (; i < 288; i++)          /* make a complete, but wrong code set */
		l[i] = 8;
	bl = 7;
	if ((i = huft_build(l, 288, 257, cplens, cplext, &tl, &bl)) != 0)
		return i;


	/* set up distance table */
	for (i = 0; i < 30; i++)      /* make an incomplete code set */
		l[i] = 5;
	bd = 5;
	if ((i = huft_build(l, 30, 0, cpdist, cpdext, &td, &bd)) > 1) {
		huft_free(tl);
DEBG(">");
		return i;
	}

	/* decompress until an end-of-block code */
	if (inflate_codes(tl, td, bl, bd))
		return 1;

	/* free the decoding tables, return */
	huft_free(tl);
	huft_free(td);

	/* done */
	return 0;
}


/* decompress an inflated type 2 (dynamic Huffman codes) block. */
int inflate_dynamic()
{
	int i;                /* temporary variables */
	unsigned j;
	unsigned l;           /* last length */
	unsigned m;           /* mask for bit lengths table */
	unsigned n;           /* number of lengths to get */
	struct huft *tl;      /* literal/length code table */
	struct huft *td;      /* distance code table */
	int bl;               /* lookup bits for tl */
	int bd;               /* lookup bits for td */
	unsigned nb;          /* number of bit length codes */
	unsigned nl;          /* number of literal/length codes */
	unsigned nd;          /* number of distance codes */
	unsigned ll[286+30];  /* literal/length and distance code lengths */
	register unsigned long b;	/* bit buffer */
	register unsigned k;		/* number of bits in bit buffer */

#if 0
  for(i = 0; i < 288+32; i++)
    if (ll[i]) {
       printf("non-zero ll index at %d\n", i);
    }
#endif
DEBG("<dyn");

	/* make local bit buffer */
	b = bb;
	k = bk;

	/* read in table lengths */
	NEEDBITS(5);
	nl = 257 + ((unsigned)b & 0x1f);      /* number of literal/length codes */
	DUMPBITS(5);
	NEEDBITS(5);
	nd = 1 + ((unsigned)b & 0x1f);        /* number of distance codes */
	DUMPBITS(5);
	NEEDBITS(4);
	nb = 4 + ((unsigned)b & 0xf);         /* number of bit length codes */
	DUMPBITS(4);
#ifdef PKZIP_BUG_WORKAROUND
	if (nl > 288 || nd > 32)
#else
	if (nl > 286 || nd > 30)
#endif
		return 1;                   /* bad lengths */

DEBGINT("nl is", nl);
DEBGINT("nd is", nd);
DEBGINT("nb is", nb);
DEBG("dyn1 ");

	/* read in bit-length-code lengths */
	for (j = 0; j < nb; j++) {
		NEEDBITS(3);
		ll[border[j]] = (unsigned)b & 7;
		DUMPBITS(3);
	}
	for (; j < 19; j++)
		ll[border[j]] = 0;

	/* build decoding table for trees--single level, 7 bit lookup */
	bl = 7;
	if ((i = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl)) != 0) {
		if (i == 1)
			huft_free(tl);
		return i;                   /* incomplete code set */
	}

DEBG("dyn3 ");

	/* read in literal and distance code lengths */
	n = nl + nd;
	m = mask_bits[bl];
	i = l = 0;
	while ((unsigned)i < n)
	{
		NEEDBITS((unsigned)bl);
		j = (td = tl + ((unsigned)b & m))->b;
		DUMPBITS(j);
		j = td->v.n;
DEBGINT("j1 is", j);
		if (j < 16)                 /* length of code in bits (0..15) */
			ll[i++] = l = j;          /* save last length in l */
		else if (j == 16) {          /* repeat last length 3 to 6 times */
			NEEDBITS(2);
			j = 3 + ((unsigned)b & 3);
			DUMPBITS(2);
DEBGINT("j2 is", j);
			if ((unsigned)i + j > n)
				return 1;
			while (j--)
				ll[i++] = l;
		}
		else if (j == 17) {          /* 3 to 10 zero length codes */
			NEEDBITS(3);
			j = 3 + ((unsigned)b & 7);
			DUMPBITS(3);
DEBGINT("j3 is", j);
			if ((unsigned)i + j > n)
				return 1;
			while (j--)
				ll[i++] = 0;
			l = 0;
		}
		else {                       /* j == 18: 11 to 138 zero length codes */
			NEEDBITS(7);
			j = 11 + ((unsigned)b & 0x7f);
			DUMPBITS(7);
DEBGINT("j4 is", j);
			if ((unsigned)i + j > n)
				return 1;
			while (j--)
				ll[i++] = 0;
			l = 0;
		}
	}
DEBG("dyn4 ");
	/* free decoding table for trees */
	huft_free(tl);
DEBG("dyn5 ");
	/* restore the global bit buffer */
	bb = b;
	bk = k;
DEBG("dyn5a ");
	/* build the decoding tables for literal/length and distance codes */
	bl = lbits;
	if ((i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl)) != 0) {
DEBG("dyn5b ");
		if (i == 1) {
			error(" incomplete literal tree\n");
			huft_free(tl);
		}
		return i;                   /* incomplete code set */
	}
DEBG("dyn5c ");
	bd = dbits;
	if ((i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd)) != 0) {
DEBG("dyn5d ");
		if (i == 1) {
			error(" incomplete distance tree\n");
#ifdef PKZIP_BUG_WORKAROUND
			i = 0;
		}
#else
		        huft_free(td);
	        }
	        huft_free(tl);
	        return i;                   /* incomplete code set */
#endif
        }

DEBG("dyn6 ");

	/* decompress until an end-of-block code */
DEBGINT("inflate_codes inptr is", inptr);
	if (inflate_codes(tl, td, bl, bd))
		return 1;
	
DEBGINT("AFTER inflate_codes inptr is", inptr);
DEBG("dyn7 ");

	/* free the decoding tables, return */
	huft_free(tl);
	huft_free(td);

	/* done */
DEBG(">");
	return 0;
}


/* decompress an inflated block
 */
int inflate_block(e)
  int *e;                 /* last block flag */
{
	unsigned t;			/* block type */
	register unsigned long b;	/* bit buffer */
	register unsigned k;		/* number of bits in bit buffer */

DEBG("<blk(");
DEBGINT("inptr is", inptr);
DEBG(")");

	/* make local bit buffer */
	b = bb;
	k = bk;

	/* read in last block bit */
	NEEDBITS(1);
	*e = (int)b & 1;
	DUMPBITS(1);

	/* read in block type */
	NEEDBITS(2);
	t = (unsigned)b & 3;
	DUMPBITS(2);

	/* restore the global bit buffer */
	bb = b;
	bk = k;

	/* inflate that block type */
	if (t == 2)
		return inflate_dynamic();
	if (t == 0)
		return inflate_stored();
	if (t == 1)
		return inflate_fixed();

DEBG(">");

	/* bad block type */
	return 2;
}


/* decompress an inflated entry */
int inflate()
{
	int e;                /* last block flag */
	int r;                /* result code */
	unsigned h;           /* maximum struct huft's malloc'ed */

	/* initialize window, bit buffer */
	wp = 0;
	bk = 0;
	bb = 0;

DEBG("INFLATE()");

	/* decompress until the last block */
	h = 0;
	do {
		hufts = 0;
		if ((r = inflate_block(&e)) != 0) {
			printf("ERROR %d while inflating block %d\n", r, e);
			return r;
		}
		if (hufts > h)
			h = hufts;
	} while (!e);

DEBG("ALL DONE\n");

	/* Undo too much lookahead. The next read will be byte aligned so we
	 * can discard unused bits in the last meaningful byte.
	 */
	while (bk >= 8) {
		bk -= 8;
		inptr--;
	}

	/* flush out slide */
	flush_output(wp);

	/* return success */
	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
