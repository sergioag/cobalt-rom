/*
 * $Id: user.c,v 1.1.1.1 2003/06/10 22:42:29 iceblink Exp $
 *
 * Userland test harness
 */
#include <stdio.h>
#include <stdlib.h>

#include "memtest.h"

#define TESTSIZE  (1024 * 1024 )

int main( int argc, char *argv[] )
{
    void *base;

    base = malloc( TESTSIZE );

    printf("8 bit: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_BYTE ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("16 bit: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_WORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("32 bit: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_LONGWORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("64 bit: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_LONGLONGWORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("8 bit block: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_BLOCKBYTE ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("16 bit block: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_BLOCKWORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("32 bit block: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_BLOCKLONGWORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }

    printf("64 bit block: "); fflush(stdout);
    if( memtest( base, TESTSIZE, MEMTEST_BY_BLOCKLONGLONGWORD ) < 0 )
    {
	printf( "failed\n" );
    }
    else
    {
	printf( "passed\n" );
    }


}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
