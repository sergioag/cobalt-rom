#include <printf.h>
#include <ctype.h>

#include <common/systype.h>
#include <common/delay.h>
#include <common/serial.h>
#include <common/cmos.h>
#include <common/cache.h>
#include <common/msr.h>
#include <common/pci.h>

#include <io.h>

#include "memtest.h"

static void clear_bss(void);
static void total_memtest(void *base, unsigned int size);
static void do_read_blast( unsigned int base, unsigned int size );
static void do_write_blast( unsigned int base, unsigned int size );
static void do_write_toggle( unsigned int base, unsigned int size );
static void do_read_toggle( unsigned int base, unsigned int size );
static int puthex(unsigned int n);
static void scrub_memory(void);

/*static void cache_shiznat( void );*/

/* these are needed for cache.c */
int do_first_boot();
unsigned int mem_size;
unsigned int gBase, gSize; /* These are here because I'm lazy */

/* XXXXX _start must be the first function defined for kernel faking to work */
#ifndef FAKEKERN
#define main_func _start
#else
void main_func(void *base, unsigned int size);

void _start( void )
{
    unsigned int size;
    outb( 0x01, 0x80 );
    size = *((unsigned long *)(0x90000 +0x1E0 ));
    size += 2048;
    size <<= 10; /* KB -> B */

    size -= ( 32 << 20 );

    main_func( (void *) (32 << 20 ), size );
}

#endif

void
main_func(void *base, unsigned int size)
{

	/* make sure the .bss space is nice for us */
    clear_bss();
    
	/* we need systype to do anything */
    systype_init();
    
	/* we need delays for various things */
    delay_init();
    
	/* need serial for output */
    serial_init(cmos_read_flag(CMOS_CONSOLE_FLAG));
    
#ifndef FAKEKERN
    cache_init(1);
    mtrr_init();
#endif
    mem_size = (unsigned int) base + size;
    gBase = (unsigned int) base;
    gSize = (unsigned int) size;

    
    
    
#ifndef FAKE_KERN
    scrub_memory();
#endif
    
    
    while (1) {
	unsigned char command;
	
	printf( "base : 0x%x\n", gBase );
	printf( "size : 0x%x\n", gSize );
	puts( "memory tester\n" );
	puts( "----------------\n");
	puts( "a - check all memory\n" );
	puts( "r - blast read a section\n" );
	puts( "w - blast write a section\n" );
	puts( "W - write Shaily's mem test\n");
	puts( "R - read Shaily's mem test\n");
	puts( "\n" );
	puts( ">" );
	
	serial_inb(&command);
	putc( command ); putc( '\n' );
	
	switch( command )
	{
	    case 'a':
		while (1) {
		    total_memtest(base, size);
		}		    
		break;
		
	    case 'r':
		do_read_blast( (unsigned int) base, size );
		break;
		
	    case 'w':
		do_write_blast( (unsigned int) base, size );
		break;
		
	    case 'W':
		do_write_toggle( (unsigned int) base, size );
		break;
		
	    case 'R':
		do_read_toggle( (unsigned int) base, size );
		break;
		
	    default:
		puts( "Unknown command\n" );
		break;
	}
    }
}

static int read_hex64( unsigned long long *val )
{
    unsigned char c;
    int read = 0;
    
    *val = 0;
    
    while(1)
    {
	serial_inb( &c );
	putc( c );
	
	if( (c == '\n') || (c == '\r'))
	{
	    if( c == '\r' )
		putc ('\n');

	    if( read )
		return 0;
	    else
		return -1;
	}

	if( isspace( c ) )
	    continue;

	if( ! isxdigit( c ) )
	    return -1;
	
	*val <<= 4;

	if( (c >= '0') && (c <= '9') ) 
	{
	    *val += c - '0';
	}
	else if( (c >= 'A') && (c <= 'F') )
	{
	    *val += (c - 'A') + 0xa;
	}
	else if( (c >= 'a') && (c <= 'f') )
	{
	    *val += (c - 'a') + 0xa;
	}
	
	read = 1;
    }

    return -1;
}

static int read_hex( unsigned int *val )
{
    unsigned long long val64;
    int ret;

    if( (ret = read_hex64( &val64 ) ) == 0 )
	*val = val64 & 0xffffffff;
    
    return ret;
}



static void get_blast_args( unsigned int *start, unsigned int *end,
			    unsigned int *pattern )
{
    puts( "Start address: " );
    while( read_hex( start ) < 0 )
	puts( "invalid address try again: " );

    puts( "end address: " );
    while( read_hex( end ) < 0 )
	puts( "invalid address try again: " );

    if( pattern )
    {
	puts( "pattern: " );
	while( read_hex( pattern ) < 0 )
	    puts( "invalid pattern try again: " );
    }
	
}

static void get_toggle_args( unsigned int *start, unsigned int *end,
			    unsigned long long *pattern, int *times )
{
    puts( "Start address (min " );
    puthex( gBase );
    puts( "): " );
    while( read_hex( start ) < 0 )
	puts( "invalid address try again: " );

    puts( "end address (max " );
    puthex( gBase + gSize );
    puts( "): " );
    while( read_hex( end ) < 0 )
	puts( "invalid address try again: " );

    if( pattern )
    {
	puts( "pattern: " );
	while( read_hex64( pattern ) < 0 )
	    puts( "invalid pattern try again: " );
    }

    if( times )
    {
	puts( "number of times (in hex): ");
	while( read_hex( times ) < 0 )
	    puts( "invalid hex value try again: " );
    }

}

static int check_args( unsigned int base, unsigned int size, 
		       unsigned int start, unsigned int end )
{
    if( start < base )
    {
	puts( "Start address less than 1M\n" );
	return -1;
    }
    if( end > (base+size) )
    {
	puts( "End address greater than end of memory\n" );
	return -1;
    }
    return 0;
}
static void do_read_blast( unsigned int base, unsigned int size )
{
    unsigned int start, end;
    
    do
    {
	get_blast_args( &start, &end, NULL );
	start &= ~ 0x3;
	end &= ~ 0x3;
    } while( check_args( base, size, start, end ) < 0 );

    while( 1 )
    {
	asm( "cld\n"
	     "rep\n"
	     "lodsl\n" 
	     : : "c" (end-start), "S" (start) : "memory" );
    }
}

static void do_write_blast( unsigned int base, unsigned int size )
{
    unsigned int start, end, pattern;
    
    get_blast_args( &start, &end, &pattern );
    start &= ~ 0x3;
    end &= ~ 0x3;

    do
    {
	get_blast_args( &start, &end, &pattern );
	start &= ~ 0x3;
	end &= ~ 0x3;
    } while( check_args( base, size, start, end ) < 0 );

    while( 1 )
    {
	asm( "cld\n"
	     "rep\n"
	     "stosl\n" 
	     : : "c" (end-start), "a" (pattern), "D" (start) : "memory" );
    }


}

static void do_write_toggle( unsigned int base, unsigned int size )
{
    unsigned int start, end,tmp;
    unsigned long long pattern[2];
    int times,i;

    do
    {
	get_toggle_args( &start, &end, pattern, &times );
	start &= ~ 0x3;
	end &= ~ 0x3;
    } while( check_args( base, size, start, end ) < 0 );

    pattern[1] = ~pattern[0];
    
/*    cache_shiznat();*/

#ifndef FAKEKERN
    puts( "turning L1 on: " );
    cache_l1_on();
    puts( "done\n" );
    puts( "turning L2 on: " );
    cache_l2_on(); 
    puts( "done\n" );
#endif

    puts( "start: " );
    puthex( start );
    puts( "\nend: " );
    puthex( end );
    puts( "\npattern: " );
    puthex( *pattern & 0xffffffff );
    puts( "\ntimes: " );
    puthex( times );
    puts( "\n" );
    
    for( i=0 ; i<times ; i++ )
    {
	puthex( i );
	puts( "\n" );
	tmp=start;
	asm( "movq 0(%%eax), %%mm0\n" /* move the long words into mmx registers */
	     "movq 8(%%eax), %%mm1\n"
	     "movq 0(%%eax), %%mm2\n"
	     "movq 8(%%eax), %%mm3\n"
	     "1:\n"
	     "cmp %%edx, %%ecx\n"
	     "jge 2f\n"
	     "movq %%mm0, 0x00(%%ecx)\n"
	     "movq %%mm1, 0x08(%%ecx)\n"
	     "movq %%mm2, 0x10(%%ecx)\n"
	     "movq %%mm3, 0x18(%%ecx)\n"
	     "add $0x20, %%ecx\n"
	     "jmp 1b\n"
	     "2:"
	     : :
	     "c" (tmp),
	     "d" (end), 
	     "a" (pattern) : "memory");
    }


}

static void do_read_toggle( unsigned int base, unsigned int size )
{
    unsigned int start, end,tmp;
    int times,i;

    do
    {
	get_toggle_args( &start, &end, NULL, &times );
	start &= ~ 0x3;
	end &= ~ 0x3;
    } while( check_args( base, size, start, end ) < 0 );

    
/*    cache_shiznat();*/

#ifndef FAKEKERN
    puts( "turning L1 on: " );
    cache_l1_on();
    puts( "done\n" );
    puts( "turning L2 on: " );
    cache_l2_on();
    puts( "done\n" );
#endif
    puts( "start: " );
    puthex( start );
    puts( "\nend: " );
    puthex( end );
    puts( "\ntimes: " );
    puthex( times );
    puts( "\n" );
    
    for( i=0 ; i<times ; i++ )
    {
	puthex( i );
	puts( "\n" );
	tmp=start;
	asm( "1:\n"
	     "cmp %%edx, %%ecx\n"
	     "jge 2f\n"
	     "movq 0x00(%%ecx), %%mm0\n"
	     "movq 0x20(%%ecx), %%mm1\n"
	     "movq 0x40(%%ecx), %%mm2\n"
	     "movq 0x60(%%ecx), %%mm3\n"
	     "movq 0x80(%%ecx), %%mm4\n"
	     "movq 0xa0(%%ecx), %%mm5\n"
	     "movq 0xc0(%%ecx), %%mm6\n"
	     "movq 0xe0(%%ecx), %%mm7\n"
	     "add $0x100, %%ecx\n"
	     "jmp 1b\n"
	     "2:"
	     : :
	     "c" (tmp),
	     "d" (end)
	     : "memory");
    }


}

extern char _bss;
extern char _ebss;
static void
clear_bss(void)
{
	unsigned long bss_start, bss_end;
	char *ptr;
	bss_start = (unsigned long)&_bss;
	bss_end = (unsigned long)&_ebss;

	/* kill it all */
	ptr = (char *)bss_start;

	memset(ptr, 0, bss_end - bss_start);
}

static void
total_memtest(void *base, unsigned int size)
{
    puts("8 bit: ");
    if (memtest( base, size, MEMTEST_BY_BYTE ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

    puts("16 bit: "); 
    if (memtest( base, size, MEMTEST_BY_WORD ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

    puts("32 bit: "); 
    if (memtest( base, size, MEMTEST_BY_LONGWORD ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

#if 0
    puts("64 bit: "); 
    if (memtest( base, size, MEMTEST_BY_LONGLONGWORD ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }
#endif

    puts("8 bit block: "); 
    if (memtest( base, size, MEMTEST_BY_BLOCKBYTE ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

    puts("16 bit block: "); 
    if (memtest( base, size, MEMTEST_BY_BLOCKWORD ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

    puts("32 bit block: "); 
    if (memtest( base, size, MEMTEST_BY_BLOCKLONGWORD ) < 0) {
	puts( "failed\n" );
    } else {
	puts( "passed\n" );
    }

#if 0
    puts("64 bit block: "); 
    if (memtest( base, size, MEMTEST_BY_BLOCKLONGLONGWORD ) < 0) {
	puts( "failed\n" );
    }
    else
    {
	puts( "passed\n" );
    }
#endif 
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

#define MTRRphysType_UC		0x0
#define MTRRphysType_WC		0x1
#define MTRRphysType_WT		0x4
#define MTRRphysType_WP		0x5
#define MTRRphysType_WB		0x6
#define MTRRfixType_WP		0x0505050505050505
#define MTRRfixType_WB		0x0606060606060606
#define MTRRphysRange		0x0000000ffffff000	/* mask bits 12-35 */
#define MTRRtopBase		0xfc000000UL
#define MTRRtopSize		0x04000000UL	/* 64 MB */
#if 0
static void cache_shiznat( void )
{
    unsigned long long msr;
    unsigned int mtrr_vmax, mtrr_num, lo, hi;
    unsigned int ram_size=mem_size, ram_base=0, bit;
    

	/* first look at the MTRR capability lists */
    msr = msr_get(MSR_MTRR_CAP);
    if (!(msr & 0x100)) {
	return;
    }

	/* disable fixed & variable MTRRs */
    msr_set_off(MSR_MTRRdefType, MSR_MTRRfixEnable | MSR_MTRRvarEnable);

	/* number of variable MTRRs supported */
    mtrr_vmax = (int)(msr & 0xff);

    	/* reset max MTRR count because we must leave 2 for OS */	
    mtrr_vmax -= 2;

	/* setup Variable MTRR registers 
	 *
	 * 1. find largest power of 2 <= ramsize
	 * 2. set mtrr to this value
	 * 3. repeat for remaining ramsize
	 * 4. stop when ramsize <4MB (no sense using MTRRs that small)
	 */
    for (mtrr_num=0, bit=31; 
	 (ram_size>0x3ffff) && (bit>21) && (mtrr_num<mtrr_vmax); bit--) {
	unsigned int size = (1 << bit);
	
	if (!(ram_size & size))
	    continue;
	
	    /* set the base and type (writeback) */
	msr_set(MSR_MTRRphysBase(mtrr_num),
		MTRRphysType_UC | (ram_base & MTRRphysRange));
	
	    /* set the size and enable bit */
	lo = (~(size - 1) & MTRRphysRange) | MSR_MTRRvarEnable;
	hi = (~((size >> 4) - 1) >> 28) & 0xf;
	wrmsr(MSR_MTRRphysMask(mtrr_num), lo, hi);
	
	    /* DPRINTF("MTRR: %dM - %dM=write-back", 
	     *	(ram_base>>20), ((ram_base+size)>>20)); 
	     */
	
	    /* move base pointer */
	ram_base += size;

	    /* select next variable mtrr msr */
	mtrr_num++;
	
	    /* subtract size from total */
	ram_size &= ~size;
    }

	/* enable Fixed & Variable MTRRs */
    msr_set_on(MSR_MTRRdefType, MSR_MTRRfixEnable | MSR_MTRRvarEnable);
}
#endif

/* 
 * scrub (zero) from 32Mb to (mem_size - 16Mb (stack)) 
 * the rest has been cleared in romcode 
 */
static void scrub_memory(void)
{
	switch (systype()) {
	case SYSTYPE_3K:
		break;

	case SYSTYPE_5K:
	{
		unsigned int i, val;


		/* mask the NMI (you figure it out)
		 * because we may get errors */
		outb(inb(0x70) | 0x80, 0x70);
		
		/* turn off parity NMIs */
		outb(inb(0x61) | 0xc, 0x61);

		/* disable ECC */
		val = pci_readb(0, 0, 0xe0);
		pci_writeb(0, 0, 0xe0, val & ~0x4);

		/* scrub 1 MB at a time */
		for (i = 32; i < (mem_size >> 20); i++) {
			void *p;
			p = (void *)(i * 1024 * 1024);
			memset(p, 0, 1024 * 1024);
		}
	
		/* clear any ECC errors we've incurred */
		val = pci_readb(0, 0, 0x47);
		pci_writeb(0, 0, 0x47, val);
	
		/* turn on NMI generation from PERR/SERR */
		outb(inb(0xc14) & ~0x0c, 0xc14);

		/* clear parity NMI bits, and enable parity NMIs */
		outb(inb(0x61) | 0xc, 0x61);
		outb(inb(0x61) & ~0xc, 0x61);

		/* re-enable ECC */
		val = pci_readb(0, 0, 0xe0);
		pci_writeb(0, 0, 0xe0, val | 0x4);

		/* unmask NMI - leave this for the kernel to re-enable*/
		/*outb(inb(0x70) & ~0x80, 0x70);*/

	}
	default:
	}
}
int do_first_boot(){return 0;}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
