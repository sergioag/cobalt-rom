/*
 * $Id: memtest.h,v 1.1.1.1 2003/06/10 22:42:29 iceblink Exp $
 * 
 * Memtest stuff be here!
 */

#ifndef __memtest_h__
#define __memtest_h__


#define MEMTEST_BY_BYTE                  0
#define MEMTEST_BY_WORD                  1
#define MEMTEST_BY_LONGWORD              2
#define MEMTEST_BY_LONGLONGWORD          3
#define MEMTEST_BY_BLOCKBYTE             4
#define MEMTEST_BY_BLOCKWORD             5
#define MEMTEST_BY_BLOCKLONGWORD         6
#define MEMTEST_BY_BLOCKLONGLONGWORD     7

int memtest( volatile void *base, unsigned int size, int type );

#endif /* __memtest_h__ */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
