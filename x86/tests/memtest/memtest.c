/*
 * $Id: memtest.c,v 1.1.1.1 2003/06/10 22:42:29 iceblink Exp $
 *
 * Strides through memory and looks for errors.  ECC should not be enabled
 * so we can get down and dirty.
 */

#ifdef USERTEST
#include <stdio.h>
#else
#include <printf.h>
#endif

#include "memtest.h"

static int puthex(unsigned int n);

#define N_PATTERNS (int)(sizeof(patterns)/sizeof(patterns[0]))
static unsigned int patterns[] = { 0x00000000, 
				   0x55555555, 
				   0xaaaaaaaa, 
				   0x5a5a5a5a,
				   0xa5a5a5a5,
				   0xffffffff };

static int memtest_by_byte( volatile unsigned char *base, unsigned int size );
static int memtest_by_word( volatile unsigned short int *base, unsigned int size );
static int memtest_by_longword( volatile unsigned  int *base, unsigned int size );
static int memtest_by_longlongword( volatile unsigned int *base, unsigned int size );
static int memtest_by_blockbyte( volatile unsigned char *base, unsigned int size );
static int memtest_by_blockword( volatile unsigned short int *base, unsigned int size );
static int memtest_by_blocklongword( volatile unsigned int *base, unsigned int size );
static int memtest_by_blocklonglongword( volatile unsigned int *base, unsigned int size );


/*
 * memtest
 *  base - pointer to the base of the memory region to test
 *  size - size of the memory region to test
 *  type - type of test
 *          0 - bytewise
 *          1 - wordwise
 *          2 - longwordwise
 *          3 - longlongwordwise (not implemented yet)
 *          4 - block bytewise 
 *          5 - block wordwise 
 *          6 - block longwordwise 
 *          7 - block longlongwordwise (not implemented yet)
 *
 * does a memtest looking for errros.  Returns -1 if an error is
 * found, 0 if no errors are found.
 */

int memtest(volatile void *base, unsigned int size, int type)
{
    switch( type )
    {
	case MEMTEST_BY_BYTE:
	    return memtest_by_byte( (unsigned char *)base, size );
	    break;

	case MEMTEST_BY_WORD:
	    return memtest_by_word( (unsigned short int*)base, size );
	    break;

	case MEMTEST_BY_LONGWORD:
	    return memtest_by_longword( (unsigned int*)base, size );
	    break;

	case MEMTEST_BY_LONGLONGWORD:
	    return memtest_by_longlongword( (unsigned int*)base, size );
	    break;

	case MEMTEST_BY_BLOCKBYTE:
	    return memtest_by_blockbyte( (unsigned char*)base, size );
	    break;

	case MEMTEST_BY_BLOCKWORD:
	    return memtest_by_blockword( (unsigned short int*)base, size );
	    break;

	case MEMTEST_BY_BLOCKLONGWORD:
	    return memtest_by_blocklongword( (unsigned int*)base, size );
	    break;

	case MEMTEST_BY_BLOCKLONGLONGWORD:
	    return memtest_by_blocklonglongword( (unsigned int*)base, size );
	    break;

	default:
	    return -1;
    }
    
}

static int memtest_by_byte( volatile unsigned char *base, unsigned int size )
{
    int pattern;
    unsigned int i;
    unsigned char val, a,b,c;
    int ret = 0;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ] & 0xff;
	for( i=0 ; i<size ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    base[i] = val;
	}

	for( i=0 ; i<size ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    a = base[i];
	    b = base[i];
	    c = base[i];

	    if( (a == b) &&
		(b == c) )
	    {
		if( a != val )
		{
		    puts("Write error at ");
		    puthex((unsigned int)base+i);
		    puts("\n");
		    ret = -1;
		}
	    }
	    else
	    {
		puts("Read error at ");
		puthex((unsigned int)base+i);
		puts("\n");
		ret = -1;
	    }

	}
    }

    return ret;
}

static int memtest_by_word( volatile unsigned short int *base, unsigned int size )
{
    int pattern;
    unsigned int i;
    unsigned short int val, a,b,c;
    int ret = 0;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ] & 0xffff;

	for( i=0 ; i<(size>>1) ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    base[i] = val;
	}

	for( i=0 ; i<(size>>1) ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    a = base[i];
	    b = base[i];
	    c = base[i];

	    if( (a == b) &&
		(b == c) )
	    {
		if( a != val )
		{
		    puts("Write error at ");
		    puthex((unsigned int)base+(i<<1));
		    puts("\n");
		    ret = -1;
		}
	    }
	    else
	    {
		puts("Read error at ");
		puthex((unsigned int)base+(i<<1));
		puts("\n");
		ret = -1;
	    }

	}
    }

    return ret;

}

static int memtest_by_longword( volatile unsigned  int *base, unsigned int size )
{
    int pattern;
    unsigned int i;
    unsigned short int val, a,b,c;
    int ret = 0;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ];

	for( i=0 ; i<(size>>2) ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    base[i] = val;
	}

	for( i=0 ; i<(size>>2) ; i++ )
	{
	    if (i % (1024*1024) == 0) puts(".");
	    a = base[i];
	    b = base[i];
	    c = base[i];

	    if( (a == b) &&
		(b == c) )
	    {
		if( a != val )
		{
		    puts("Write error at ");
		    puthex((unsigned int)base+(i<<2));
		    puts("\n");
		    ret = -1;
		}
	    }
	    else
	    {
		puts("Read error at ");
		puthex((unsigned int)base+(i<<2));
		puts("\n");
		ret = -1;
	    }

	}
    }

    return ret;


}

static int memtest_by_longlongword( volatile unsigned int *base, unsigned int size )
{
    if (!base) size = 0;
    return -1;
}

static int memtest_by_blockbyte( volatile unsigned char *base, unsigned int size )
{
    int pattern; 
    unsigned int i;
    unsigned char val, a,b,c;
    int ret = 0;
    volatile unsigned char *mem;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ] & 0xff;
	mem = base;
	
	    /* do a block set */
	asm( "cld\n"
	     "rep\n"
	     "stosb\n" 
	     : : "c" (size), "a" (val), "D" (mem) : "memory" );

	
	i = size;

	while( i > 0 )
	{
	    asm( "cld\n"
		 "repe\n"
		 "scasb" 
		 : "=c" (i), "=D" (mem) 
		 : "c" (i), "a" (val), "D" (mem) : "memory" );

	    if( i > 0 )
	    {
		a = base[i];
		b = base[i];
		c = base[i];
		
		if( (a == b) &&
		    (b == c) )
		{
		    if( a != val )
		    {
			puts("Write error at ");
			puthex((unsigned int)base);
			puts("\n");
			ret = -1;
		    }
		}
		else
		{
		    puts("Read error at ");
		    puthex((unsigned int)base);
		    puts("\n");
		    ret = -1;
		}
		
	    }
	}
	
    }    

    return ret;
}

static int memtest_by_blockword( volatile unsigned short int *base, unsigned int size )
{
    int pattern; 
    unsigned int i;
    unsigned short int val, a,b,c;
    int ret = 0;
    volatile unsigned short int *mem;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ] & 0xffff;
	mem = base;
	
	i = size>>1;

	    /* do a block set */
	asm( "cld\n"
	     "rep\n"
	     "stosw\n" 
	     : : "c" (i), "a" (val), "D" (mem) : "memory" );

	

	while( i > 0 )
	{
	    asm( "cld\n"
		 "repe\n"
		 "scasw" 
		 : "=c" (i), "=D" (mem) 
		 : "c" (i), "a" (val), "D" (mem) : "memory" );

	    if( i > 0 )
	    {
		a = base[i];
		b = base[i];
		c = base[i];
		
		if( (a == b) &&
		    (b == c) )
		{
		    if( a != val )
		    {
			puts("Write error at ");
			puthex((unsigned int)base);
			puts("\n");
			ret = -1;
		    }
		}
		else
		{
		    puts("Read error at ");
		    puthex((unsigned int)base);
		    puts("\n");
		    ret = -1;
		}
		
	    }
	}
	
    }    

    return ret;

}

static int memtest_by_blocklongword( volatile unsigned int *base, unsigned int size )
{
    int pattern; 
    unsigned int i;
    unsigned int val, a,b,c;
    int ret = 0;
    volatile unsigned int *mem;

    for( pattern=0 ; pattern<N_PATTERNS ; pattern++ )
    {
	val = patterns[ pattern ];
	mem = base;

	i = size>>2;

	
	    /* do a block set */
	asm( "cld\n"
	     "rep\n"
	     "stosl\n" 
	     : : "c" (i), "a" (val), "D" (mem) : "memory" );

	

	while( i > 0 )
	{
	    asm( "cld\n"
		 "repe\n"
		 "scasl" 
		 : "=c" (i), "=D" (mem) 
		 : "c" (i), "a" (val), "D" (mem) : "memory" );

	    if( i > 0 )
	    {
		a = base[i];
		b = base[i];
		c = base[i];
		
		if( (a == b) &&
		    (b == c) )
		{
		    if( a != val )
		    {
			puts("Write error at ");
			puthex((unsigned int)base);
			puts("\n");
			ret = -1;
		    }
		}
		else
		{
		    puts("Read error at ");
		    puthex((unsigned int)base);
		    puts("\n");
		    ret = -1;
		}
		
	    }
	}
	
    }    

    return ret;
}

static int memtest_by_blocklonglongword( volatile unsigned int *base, unsigned int size )
{
    if (!base) size = 0;
    return -1;
}

static int
puthex(unsigned int n)
{
	char nbuf[16];
	char *p;
	static char *chars = "0123456789abcdef";

        /* write the number, from the right */
        nbuf[sizeof(nbuf) - 1] = '\0';
        p = &(nbuf[sizeof(nbuf) - 2]);

        /* must have at least one char */
        *p = chars[n % 16];
        n /= 16;
        while (p >= nbuf && n) {
                p--;
                *p = chars[n % 16];
                n /= 16;
        }
	puts("0x");
	return puts(p);
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
