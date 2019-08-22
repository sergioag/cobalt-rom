/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: raw2hex.c,v 1.1.1.1 2003/06/10 22:41:37 iceblink Exp $ */
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>

int main( int argc, char *argv[] )
{
    FILE *infp, *outfp;
    struct stat stat_buff;
    int i,j;
    unsigned char inbuff[0x20];
    int sum;

    if( argc != 3 )
    {
	printf("usage: raw2hex <infile> <outfile>\n");
	return -1;
    }

    if( stat( argv[1], &stat_buff ) < 0 )
    {
	perror( "stat" );
	return -1;
    }

    if( (infp = fopen( argv[1], "rb" )) == NULL )
    {
	fprintf(stderr, "Cant open file \"%s\"\n", argv[1] );
	return -1;
    }

    if( (outfp = fopen( argv[2], "wb" )) == NULL )
    {
	fprintf(stderr, "Cant open file \"%s\"\n", argv[2] );
	return -1;
    }

    for( i=0 ; i< stat_buff.st_size - (stat_buff.st_size % 0x20) ; i+=0x20 )
    {
	if( !(i%(0x100)) )
	{
	    fprintf( outfp,":02000002%04X",i/0x10 );
	    sum= 0x02+ 02+((i/0x10)&0xff) + ((i/0x1000)&0xff) ;
	    sum = (~(sum-1)) &0xff;
	}
	sum=0x20;
	fprintf(outfp,":20%04X00", i&0xff );
	sum += (i&0xff);
	
	if( fread( inbuff, 0x20, 1, infp ) < 1 )
	{
	    fprintf( stderr, "Problem reading infile at pos 0x%x.\n", i );
	    return -1;
	}

	for( j=0 ; j<0x20 ; j++ )
	{
	    sum += inbuff[j];
	    fprintf( outfp, "%02X", inbuff[j] );
	}
	sum = (~(sum-1)) &0xff;
	
	
	fprintf( outfp, "%02X\n", sum );
    }

    if( i > stat_buff.st_size )
    {
	if( fread( inbuff, stat_buff.st_size % 0x20 , 1, infp ) < 1 )
	{
	    fprintf( stderr, "Problem reading infile at pos 0x%x.\n", i );
	    return -1;
	}
	fprintf(outfp, ":20%04X00" );
	sum = 0;
	for( j=0 ; j<(stat_buff.st_size % 0x20) ; j++ )
	{
	    sum += inbuff[j];
	    fprintf( outfp, "%02X", inbuff[j] );
	}
	sum = (0xff - sum)&0xff;
	fprintf( outfp, "%02X\n",sum );
    }
    fprintf(outfp, ":00000001FF\n");
    fclose( outfp );
    fclose( infp );
    return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
