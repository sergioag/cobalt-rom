#include <stdlib.h>
#include <printf.h>

#define BZ_NO_STDIO
#ifndef BZ_NO_STDIO
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#endif

#include <common/bzlib.h>



/*-- General stuff. --*/

#define BZ_VERSION  "1.0.1, 23-June-2000"

typedef char            Char;
typedef unsigned char   Bool;
typedef unsigned char   UChar;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef short           Int16;
typedef unsigned short  UInt16;

#define True  ((Bool)1)
#define False ((Bool)0)

#ifndef __GNUC__
#define __inline__  /* */
#endif 

#ifndef BZ_NO_STDIO
extern void BZ2_bz__AssertH__fail ( int errcode );
#define AssertH(cond,errcode) \
   { if (!(cond)) BZ2_bz__AssertH__fail ( errcode ); }
#if BZ_DEBUG
#define AssertD(cond,msg) \
   { if (!(cond)) {       \
      fprintf ( stderr,   \
        "\n\nlibbzip2(debug build): internal error\n\t%s\n", msg );\
      exit(1); \
   }}
#else
#define AssertD(cond,msg) /* */
#endif
#define VPrintf0(zf) \
   fprintf(stderr,zf)
#define VPrintf1(zf,za1) \
   fprintf(stderr,zf,za1)
#define VPrintf2(zf,za1,za2) \
   fprintf(stderr,zf,za1,za2)
#define VPrintf3(zf,za1,za2,za3) \
   fprintf(stderr,zf,za1,za2,za3)
#define VPrintf4(zf,za1,za2,za3,za4) \
   fprintf(stderr,zf,za1,za2,za3,za4)
#define VPrintf5(zf,za1,za2,za3,za4,za5) \
   fprintf(stderr,zf,za1,za2,za3,za4,za5)
#else
#define AssertH(cond,errcode) \
   { if (!(cond)) printf( "BZip Internal Error Code #%d\n", errcode ); }
#define AssertD(cond,msg) /* */
#define VPrintf0(zf) \
   printf(zf)
#define VPrintf1(zf,za1) \
   printf(zf,za1)
#define VPrintf2(zf,za1,za2) \
   printf(zf,za1,za2)
#define VPrintf3(zf,za1,za2,za3) \
   printf(zf,za1,za2,za3)
#define VPrintf4(zf,za1,za2,za3,za4) \
   printf(zf,za1,za2,za3,za4)
#define VPrintf5(zf,za1,za2,za3,za4,za5) \
   printf(zf,,za1,za2,za3,za4,za5)
#endif


#define BZALLOC(nnn) (strm->bzalloc)(strm->opaque,(nnn),1)
#define BZFREE(ppp)  (strm->bzfree)(strm->opaque,(ppp))


/*-- Constants for the back end. --*/

#define BZ_MAX_ALPHA_SIZE 258
#define BZ_MAX_CODE_LEN    23

#define BZ_RUNA 0
#define BZ_RUNB 1

#define BZ_N_GROUPS 6
#define BZ_G_SIZE   50
#define BZ_N_ITERS  4

#define BZ_MAX_SELECTORS (2 + (900000 / BZ_G_SIZE))



/*-- Stuff for randomising repetitive blocks. --*/

extern Int32 BZ2_rNums[512];

#define BZ_RAND_DECLS                          \
   Int32 rNToGo;                               \
   Int32 rTPos                                 \

#define BZ_RAND_INIT_MASK                      \
   s->rNToGo = 0;                              \
   s->rTPos  = 0                               \

#define BZ_RAND_MASK ((s->rNToGo == 1) ? 1 : 0)

#define BZ_RAND_UPD_MASK                       \
   if (s->rNToGo == 0) {                       \
      s->rNToGo = BZ2_rNums[s->rTPos];         \
      s->rTPos++;                              \
      if (s->rTPos == 512) s->rTPos = 0;       \
   }                                           \
   s->rNToGo--;



/*-- Stuff for doing CRCs. --*/

extern UInt32 BZ2_crc32Table[256];

#define BZ_INITIALISE_CRC(crcVar)              \
{                                              \
   crcVar = 0xffffffffL;                       \
}

#define BZ_FINALISE_CRC(crcVar)                \
{                                              \
   crcVar = ~(crcVar);                         \
}

#define BZ_UPDATE_CRC(crcVar,cha)              \
{                                              \
   crcVar = (crcVar << 8) ^                    \
            BZ2_crc32Table[(crcVar >> 24) ^    \
                           ((UChar)cha)];      \
}



/*-- States and modes for compression. --*/

#define BZ_M_IDLE      1
#define BZ_M_RUNNING   2
#define BZ_M_FLUSHING  3
#define BZ_M_FINISHING 4

#define BZ_S_OUTPUT    1
#define BZ_S_INPUT     2

#define BZ_N_RADIX 2
#define BZ_N_QSORT 12
#define BZ_N_SHELL 18
#define BZ_N_OVERSHOOT (BZ_N_RADIX + BZ_N_QSORT + BZ_N_SHELL + 2)




/*-- Structure holding all the compression-side stuff. --*/

typedef
   struct {
      /* pointer back to the struct bz_stream */
      bz_stream* strm;

      /* mode this stream is in, and whether inputting */
      /* or outputting data */
      Int32    mode;
      Int32    state;

      /* remembers avail_in when flush/finish requested */
      UInt32   avail_in_expect;

      /* for doing the block sorting */
      UInt32*  arr1;
      UInt32*  arr2;
      UInt32*  ftab;
      Int32    origPtr;

      /* aliases for arr1 and arr2 */
      UInt32*  ptr;
      UChar*   block;
      UInt16*  mtfv;
      UChar*   zbits;

      /* for deciding when to use the fallback sorting algorithm */
      Int32    workFactor;

      /* run-length-encoding of the input */
      UInt32   state_in_ch;
      Int32    state_in_len;
      BZ_RAND_DECLS;

      /* input and output limits and current posns */
      Int32    nblock;
      Int32    nblockMAX;
      Int32    numZ;
      Int32    state_out_pos;

      /* map of bytes used in block */
      Int32    nInUse;
      Bool     inUse[256];
      UChar    unseqToSeq[256];

      /* the buffer for bit stream creation */
      UInt32   bsBuff;
      Int32    bsLive;

      /* block and combined CRCs */
      UInt32   blockCRC;
      UInt32   combinedCRC;

      /* misc administratium */
      Int32    verbosity;
      Int32    blockNo;
      Int32    blockSize100k;

      /* stuff for coding the MTF values */
      Int32    nMTF;
      Int32    mtfFreq    [BZ_MAX_ALPHA_SIZE];
      UChar    selector   [BZ_MAX_SELECTORS];
      UChar    selectorMtf[BZ_MAX_SELECTORS];

      UChar    len     [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      Int32    code    [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      Int32    rfreq   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      /* second dimension: only 3 needed; 4 makes index calculations faster */
      UInt32   len_pack[BZ_MAX_ALPHA_SIZE][4];

   }
   EState;



/*-- externs for compression. --*/
/*
extern void 
BZ2_blockSort ( EState* );

extern void 
BZ2_compressBlock ( EState*, Bool );

extern void 
BZ2_bsInitWrite ( EState* );

extern void 
BZ2_hbAssignCodes ( Int32*, UChar*, Int32, Int32, Int32 );

extern void 
BZ2_hbMakeCodeLengths ( UChar*, Int32*, Int32, Int32 );
*/


/*-- states for decompression. --*/

#define BZ_X_IDLE        1
#define BZ_X_OUTPUT      2

#define BZ_X_MAGIC_1     10
#define BZ_X_MAGIC_2     11
#define BZ_X_MAGIC_3     12
#define BZ_X_MAGIC_4     13
#define BZ_X_BLKHDR_1    14
#define BZ_X_BLKHDR_2    15
#define BZ_X_BLKHDR_3    16
#define BZ_X_BLKHDR_4    17
#define BZ_X_BLKHDR_5    18
#define BZ_X_BLKHDR_6    19
#define BZ_X_BCRC_1      20
#define BZ_X_BCRC_2      21
#define BZ_X_BCRC_3      22
#define BZ_X_BCRC_4      23
#define BZ_X_RANDBIT     24
#define BZ_X_ORIGPTR_1   25
#define BZ_X_ORIGPTR_2   26
#define BZ_X_ORIGPTR_3   27
#define BZ_X_MAPPING_1   28
#define BZ_X_MAPPING_2   29
#define BZ_X_SELECTOR_1  30
#define BZ_X_SELECTOR_2  31
#define BZ_X_SELECTOR_3  32
#define BZ_X_CODING_1    33
#define BZ_X_CODING_2    34
#define BZ_X_CODING_3    35
#define BZ_X_MTF_1       36
#define BZ_X_MTF_2       37
#define BZ_X_MTF_3       38
#define BZ_X_MTF_4       39
#define BZ_X_MTF_5       40
#define BZ_X_MTF_6       41
#define BZ_X_ENDHDR_2    42
#define BZ_X_ENDHDR_3    43
#define BZ_X_ENDHDR_4    44
#define BZ_X_ENDHDR_5    45
#define BZ_X_ENDHDR_6    46
#define BZ_X_CCRC_1      47
#define BZ_X_CCRC_2      48
#define BZ_X_CCRC_3      49
#define BZ_X_CCRC_4      50



/*-- Constants for the fast MTF decoder. --*/

#define MTFA_SIZE 4096
#define MTFL_SIZE 16



/*-- Structure holding all the decompression-side stuff. --*/

typedef
   struct {
      /* pointer back to the struct bz_stream */
      bz_stream* strm;

      /* state indicator for this stream */
      Int32    state;

      /* for doing the final run-length decoding */
      UChar    state_out_ch;
      Int32    state_out_len;
      Bool     blockRandomised;
      BZ_RAND_DECLS;

      /* the buffer for bit stream reading */
      UInt32   bsBuff;
      Int32    bsLive;

      /* misc administratium */
      Int32    blockSize100k;
      Bool     smallDecompress;
      Int32    currBlockNo;
      Int32    verbosity;

      /* for undoing the Burrows-Wheeler transform */
      Int32    origPtr;
      UInt32   tPos;
      Int32    k0;
      Int32    unzftab[256];
      Int32    nblock_used;
      Int32    cftab[257];
      Int32    cftabCopy[257];

      /* for undoing the Burrows-Wheeler transform (FAST) */
      UInt32   *tt;

      /* for undoing the Burrows-Wheeler transform (SMALL) */
      UInt16   *ll16;
      UChar    *ll4;

      /* stored and calculated CRCs */
      UInt32   storedBlockCRC;
      UInt32   storedCombinedCRC;
      UInt32   calculatedBlockCRC;
      UInt32   calculatedCombinedCRC;

      /* map of bytes used in block */
      Int32    nInUse;
      Bool     inUse[256];
      Bool     inUse16[16];
      UChar    seqToUnseq[256];

      /* for decoding the MTF values */
      UChar    mtfa   [MTFA_SIZE];
      Int32    mtfbase[256 / MTFL_SIZE];
      UChar    selector   [BZ_MAX_SELECTORS];
      UChar    selectorMtf[BZ_MAX_SELECTORS];
      UChar    len  [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];

      Int32    limit  [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      Int32    base   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      Int32    perm   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
      Int32    minLens[BZ_N_GROUPS];

      /* save area for scalars in the main decompress code */
      Int32    save_i;
      Int32    save_j;
      Int32    save_t;
      Int32    save_alphaSize;
      Int32    save_nGroups;
      Int32    save_nSelectors;
      Int32    save_EOB;
      Int32    save_groupNo;
      Int32    save_groupPos;
      Int32    save_nextSym;
      Int32    save_nblockMAX;
      Int32    save_nblock;
      Int32    save_es;
      Int32    save_N;
      Int32    save_curr;
      Int32    save_zt;
      Int32    save_zn; 
      Int32    save_zvec;
      Int32    save_zj;
      Int32    save_gSel;
      Int32    save_gMinlen;
      Int32*   save_gLimit;
      Int32*   save_gBase;
      Int32*   save_gPerm;

   }
   DState;



/*-- Macros for decompression. --*/

#define BZ_GET_FAST(cccc)                     \
    s->tPos = s->tt[s->tPos];                 \
    cccc = (UChar)(s->tPos & 0xff);           \
    s->tPos >>= 8;

#define BZ_GET_FAST_C(cccc)                   \
    c_tPos = c_tt[c_tPos];                    \
    cccc = (UChar)(c_tPos & 0xff);            \
    c_tPos >>= 8;

#define SET_LL4(i,n)                                          \
   { if (((i) & 0x1) == 0)                                    \
        s->ll4[(i) >> 1] = (s->ll4[(i) >> 1] & 0xf0) | (n); else    \
        s->ll4[(i) >> 1] = (s->ll4[(i) >> 1] & 0x0f) | ((n) << 4);  \
   }

#define GET_LL4(i)                             \
   ((((UInt32)(s->ll4[(i) >> 1])) >> (((i) << 2) & 0x4)) & 0xF)

#define SET_LL(i,n)                          \
   { s->ll16[i] = (UInt16)(n & 0x0000ffff);  \
     SET_LL4(i, n >> 16);                    \
   }

#define GET_LL(i) \
   (((UInt32)s->ll16[i]) | (GET_LL4(i) << 16))

#define BZ_GET_SMALL(cccc)                            \
      cccc = BZ2_indexIntoF ( s->tPos, s->cftab );    \
      s->tPos = GET_LL(s->tPos);


/*-- externs for decompression. --*/
static Int32 
BZ2_indexIntoF ( Int32, Int32* );
/*
extern Int32 
BZ2_decompress ( DState* );

extern void 
BZ2_hbCreateDecodeTables ( Int32*, Int32*, Int32*, UChar*,
                           Int32,  Int32, Int32 );
*/


/*-- BZ_NO_STDIO seems to make NULL disappear on some platforms. --*/

#ifdef BZ_NO_STDIO
#ifndef NULL
#define NULL 0
#endif
#endif

/*---------------------------------------------------*/
__inline__ Int32 BZ2_indexIntoF ( Int32 indx, Int32 *cftab )
{
   Int32 nb, na, mid;
   nb = 0;
   na = 256;
   do {
      mid = (nb + na) >> 1;
      if (indx >= cftab[mid]) nb = mid; else na = mid;
   }
   while (na - nb != 1);
   return nb;
}


/*---------------------------------------------*/
#define fswap(zz1, zz2) \
   { Int32 zztmp = zz1; zz1 = zz2; zz2 = zztmp; }

#define fvswap(zzp1, zzp2, zzn)       \
{                                     \
   Int32 yyp1 = (zzp1);               \
   Int32 yyp2 = (zzp2);               \
   Int32 yyn  = (zzn);                \
   while (yyn > 0) {                  \
      fswap(fmap[yyp1], fmap[yyp2]);  \
      yyp1++; yyp2++; yyn--;          \
   }                                  \
}


#define fmin(a,b) ((a) < (b)) ? (a) : (b)

#define fpush(lz,hz) { stackLo[sp] = lz; \
                       stackHi[sp] = hz; \
                       sp++; }

#define fpop(lz,hz) { sp--;              \
                      lz = stackLo[sp];  \
                      hz = stackHi[sp]; }

#define FALLBACK_QSORT_SMALL_THRESH 10
#define FALLBACK_QSORT_STACK_SIZE   100


#undef fmin
#undef fpush
#undef fpop
#undef fswap
#undef fvswap
#undef FALLBACK_QSORT_SMALL_THRESH
#undef FALLBACK_QSORT_STACK_SIZE


/*---------------------------------------------*/
/* Pre:
      nblock > 0
      eclass exists for [0 .. nblock-1]
      ((UChar*)eclass) [0 .. nblock-1] holds block
      ptr exists for [0 .. nblock-1]

   Post:
      ((UChar*)eclass) [0 .. nblock-1] holds block
      All other areas of eclass destroyed
      fmap [0 .. nblock-1] holds sorted order
      bhtab [ 0 .. 2+(nblock/32) ] destroyed
*/

#define       SET_BH(zz)  bhtab[(zz) >> 5] |= (1 << ((zz) & 31))
#define     CLEAR_BH(zz)  bhtab[(zz) >> 5] &= ~(1 << ((zz) & 31))
#define     ISSET_BH(zz)  (bhtab[(zz) >> 5] & (1 << ((zz) & 31)))
#define      WORD_BH(zz)  bhtab[(zz) >> 5]
#define UNALIGNED_BH(zz)  ((zz) & 0x01f)


#undef       SET_BH
#undef     CLEAR_BH
#undef     ISSET_BH
#undef      WORD_BH
#undef UNALIGNED_BH


/*---------------------------------------------*/
/*--
   The following is an implementation of
   an elegant 3-way quicksort for strings,
   described in a paper "Fast Algorithms for
   Sorting and Searching Strings", by Robert
   Sedgewick and Jon L. Bentley.
--*/

#define mswap(zz1, zz2) \
   { Int32 zztmp = zz1; zz1 = zz2; zz2 = zztmp; }

#define mvswap(zzp1, zzp2, zzn)       \
{                                     \
   Int32 yyp1 = (zzp1);               \
   Int32 yyp2 = (zzp2);               \
   Int32 yyn  = (zzn);                \
   while (yyn > 0) {                  \
      mswap(ptr[yyp1], ptr[yyp2]);    \
      yyp1++; yyp2++; yyn--;          \
   }                                  \
}

#define mmin(a,b) ((a) < (b)) ? (a) : (b)

#define mpush(lz,hz,dz) { stackLo[sp] = lz; \
                          stackHi[sp] = hz; \
                          stackD [sp] = dz; \
                          sp++; }

#define mpop(lz,hz,dz) { sp--;             \
                         lz = stackLo[sp]; \
                         hz = stackHi[sp]; \
                         dz = stackD [sp]; }


#define mnextsize(az) (nextHi[az]-nextLo[az])

#define mnextswap(az,bz)                                        \
   { Int32 tz;                                                  \
     tz = nextLo[az]; nextLo[az] = nextLo[bz]; nextLo[bz] = tz; \
     tz = nextHi[az]; nextHi[az] = nextHi[bz]; nextHi[bz] = tz; \
     tz = nextD [az]; nextD [az] = nextD [bz]; nextD [bz] = tz; }


#define MAIN_QSORT_SMALL_THRESH 20
#define MAIN_QSORT_DEPTH_THRESH (BZ_N_RADIX + BZ_N_QSORT)
#define MAIN_QSORT_STACK_SIZE 100


#undef mswap
#undef mvswap
#undef mpush
#undef mpop
#undef mmin
#undef mnextsize
#undef mnextswap
#undef MAIN_QSORT_SMALL_THRESH
#undef MAIN_QSORT_DEPTH_THRESH
#undef MAIN_QSORT_STACK_SIZE


/*---------------------------------------------------*/
#define WEIGHTOF(zz0)  ((zz0) & 0xffffff00)
#define DEPTHOF(zz1)   ((zz1) & 0x000000ff)
#define MYMAX(zz2,zz3) ((zz2) > (zz3) ? (zz2) : (zz3))

#define ADDWEIGHTS(zw1,zw2)                           \
   (WEIGHTOF(zw1)+WEIGHTOF(zw2)) |                    \
   (1 + MYMAX(DEPTHOF(zw1),DEPTHOF(zw2)))

#define UPHEAP(z)                                     \
{                                                     \
   Int32 zz, tmp;                                     \
   zz = z; tmp = heap[zz];                            \
   while (weight[tmp] < weight[heap[zz >> 1]]) {      \
      heap[zz] = heap[zz >> 1];                       \
      zz >>= 1;                                       \
   }                                                  \
   heap[zz] = tmp;                                    \
}

#define DOWNHEAP(z)                                   \
{                                                     \
   Int32 zz, yy, tmp;                                 \
   zz = z; tmp = heap[zz];                            \
   while (True) {                                     \
      yy = zz << 1;                                   \
      if (yy > nHeap) break;                          \
      if (yy < nHeap &&                               \
          weight[heap[yy+1]] < weight[heap[yy]])      \
         yy++;                                        \
      if (weight[tmp] < weight[heap[yy]]) break;      \
      heap[zz] = heap[yy];                            \
      zz = yy;                                        \
   }                                                  \
   heap[zz] = tmp;                                    \
}


/*---------------------------------------------------*/
static void BZ2_hbCreateDecodeTables ( Int32 *limit,
                                Int32 *base,
                                Int32 *perm,
                                UChar *length,
                                Int32 minLen,
                                Int32 maxLen,
                                Int32 alphaSize )
{
   Int32 pp, i, j, vec;

   pp = 0;
   for (i = minLen; i <= maxLen; i++)
      for (j = 0; j < alphaSize; j++)
         if (length[j] == i) { perm[pp] = j; pp++; };

   for (i = 0; i < BZ_MAX_CODE_LEN; i++) base[i] = 0;
   for (i = 0; i < alphaSize; i++) base[length[i]+1]++;

   for (i = 1; i < BZ_MAX_CODE_LEN; i++) base[i] += base[i-1];

   for (i = 0; i < BZ_MAX_CODE_LEN; i++) limit[i] = 0;
   vec = 0;

   for (i = minLen; i <= maxLen; i++) {
      vec += (base[i+1] - base[i]);
      limit[i] = vec-1;
      vec <<= 1;
   }
   for (i = minLen + 1; i <= maxLen; i++)
      base[i] = ((limit[i-1] + 1) << 1) - base[i];
}


/*-------------------------------------------------------------*/
/*--- end                                         huffman.c ---*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*--- Table for doing CRCs                                  ---*/
/*---                                            crctable.c ---*/
/*-------------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-2000 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@acm.org
  bzip2/libbzip2 version 1.0 of 21 March 2000

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/


/*--
  I think this is an implementation of the AUTODIN-II,
  Ethernet & FDDI 32-bit CRC standard.  Vaguely derived
  from code by Rob Warnock, in Section 51 of the
  comp.compression FAQ.
--*/

static UInt32 BZ2_crc32Table[256] = {

   /*-- Ugly, innit? --*/

   0x00000000L, 0x04c11db7L, 0x09823b6eL, 0x0d4326d9L,
   0x130476dcL, 0x17c56b6bL, 0x1a864db2L, 0x1e475005L,
   0x2608edb8L, 0x22c9f00fL, 0x2f8ad6d6L, 0x2b4bcb61L,
   0x350c9b64L, 0x31cd86d3L, 0x3c8ea00aL, 0x384fbdbdL,
   0x4c11db70L, 0x48d0c6c7L, 0x4593e01eL, 0x4152fda9L,
   0x5f15adacL, 0x5bd4b01bL, 0x569796c2L, 0x52568b75L,
   0x6a1936c8L, 0x6ed82b7fL, 0x639b0da6L, 0x675a1011L,
   0x791d4014L, 0x7ddc5da3L, 0x709f7b7aL, 0x745e66cdL,
   0x9823b6e0L, 0x9ce2ab57L, 0x91a18d8eL, 0x95609039L,
   0x8b27c03cL, 0x8fe6dd8bL, 0x82a5fb52L, 0x8664e6e5L,
   0xbe2b5b58L, 0xbaea46efL, 0xb7a96036L, 0xb3687d81L,
   0xad2f2d84L, 0xa9ee3033L, 0xa4ad16eaL, 0xa06c0b5dL,
   0xd4326d90L, 0xd0f37027L, 0xddb056feL, 0xd9714b49L,
   0xc7361b4cL, 0xc3f706fbL, 0xceb42022L, 0xca753d95L,
   0xf23a8028L, 0xf6fb9d9fL, 0xfbb8bb46L, 0xff79a6f1L,
   0xe13ef6f4L, 0xe5ffeb43L, 0xe8bccd9aL, 0xec7dd02dL,
   0x34867077L, 0x30476dc0L, 0x3d044b19L, 0x39c556aeL,
   0x278206abL, 0x23431b1cL, 0x2e003dc5L, 0x2ac12072L,
   0x128e9dcfL, 0x164f8078L, 0x1b0ca6a1L, 0x1fcdbb16L,
   0x018aeb13L, 0x054bf6a4L, 0x0808d07dL, 0x0cc9cdcaL,
   0x7897ab07L, 0x7c56b6b0L, 0x71159069L, 0x75d48ddeL,
   0x6b93dddbL, 0x6f52c06cL, 0x6211e6b5L, 0x66d0fb02L,
   0x5e9f46bfL, 0x5a5e5b08L, 0x571d7dd1L, 0x53dc6066L,
   0x4d9b3063L, 0x495a2dd4L, 0x44190b0dL, 0x40d816baL,
   0xaca5c697L, 0xa864db20L, 0xa527fdf9L, 0xa1e6e04eL,
   0xbfa1b04bL, 0xbb60adfcL, 0xb6238b25L, 0xb2e29692L,
   0x8aad2b2fL, 0x8e6c3698L, 0x832f1041L, 0x87ee0df6L,
   0x99a95df3L, 0x9d684044L, 0x902b669dL, 0x94ea7b2aL,
   0xe0b41de7L, 0xe4750050L, 0xe9362689L, 0xedf73b3eL,
   0xf3b06b3bL, 0xf771768cL, 0xfa325055L, 0xfef34de2L,
   0xc6bcf05fL, 0xc27dede8L, 0xcf3ecb31L, 0xcbffd686L,
   0xd5b88683L, 0xd1799b34L, 0xdc3abdedL, 0xd8fba05aL,
   0x690ce0eeL, 0x6dcdfd59L, 0x608edb80L, 0x644fc637L,
   0x7a089632L, 0x7ec98b85L, 0x738aad5cL, 0x774bb0ebL,
   0x4f040d56L, 0x4bc510e1L, 0x46863638L, 0x42472b8fL,
   0x5c007b8aL, 0x58c1663dL, 0x558240e4L, 0x51435d53L,
   0x251d3b9eL, 0x21dc2629L, 0x2c9f00f0L, 0x285e1d47L,
   0x36194d42L, 0x32d850f5L, 0x3f9b762cL, 0x3b5a6b9bL,
   0x0315d626L, 0x07d4cb91L, 0x0a97ed48L, 0x0e56f0ffL,
   0x1011a0faL, 0x14d0bd4dL, 0x19939b94L, 0x1d528623L,
   0xf12f560eL, 0xf5ee4bb9L, 0xf8ad6d60L, 0xfc6c70d7L,
   0xe22b20d2L, 0xe6ea3d65L, 0xeba91bbcL, 0xef68060bL,
   0xd727bbb6L, 0xd3e6a601L, 0xdea580d8L, 0xda649d6fL,
   0xc423cd6aL, 0xc0e2d0ddL, 0xcda1f604L, 0xc960ebb3L,
   0xbd3e8d7eL, 0xb9ff90c9L, 0xb4bcb610L, 0xb07daba7L,
   0xae3afba2L, 0xaafbe615L, 0xa7b8c0ccL, 0xa379dd7bL,
   0x9b3660c6L, 0x9ff77d71L, 0x92b45ba8L, 0x9675461fL,
   0x8832161aL, 0x8cf30badL, 0x81b02d74L, 0x857130c3L,
   0x5d8a9099L, 0x594b8d2eL, 0x5408abf7L, 0x50c9b640L,
   0x4e8ee645L, 0x4a4ffbf2L, 0x470cdd2bL, 0x43cdc09cL,
   0x7b827d21L, 0x7f436096L, 0x7200464fL, 0x76c15bf8L,
   0x68860bfdL, 0x6c47164aL, 0x61043093L, 0x65c52d24L,
   0x119b4be9L, 0x155a565eL, 0x18197087L, 0x1cd86d30L,
   0x029f3d35L, 0x065e2082L, 0x0b1d065bL, 0x0fdc1becL,
   0x3793a651L, 0x3352bbe6L, 0x3e119d3fL, 0x3ad08088L,
   0x2497d08dL, 0x2056cd3aL, 0x2d15ebe3L, 0x29d4f654L,
   0xc5a92679L, 0xc1683bceL, 0xcc2b1d17L, 0xc8ea00a0L,
   0xd6ad50a5L, 0xd26c4d12L, 0xdf2f6bcbL, 0xdbee767cL,
   0xe3a1cbc1L, 0xe760d676L, 0xea23f0afL, 0xeee2ed18L,
   0xf0a5bd1dL, 0xf464a0aaL, 0xf9278673L, 0xfde69bc4L,
   0x89b8fd09L, 0x8d79e0beL, 0x803ac667L, 0x84fbdbd0L,
   0x9abc8bd5L, 0x9e7d9662L, 0x933eb0bbL, 0x97ffad0cL,
   0xafb010b1L, 0xab710d06L, 0xa6322bdfL, 0xa2f33668L,
   0xbcb4666dL, 0xb8757bdaL, 0xb5365d03L, 0xb1f740b4L
};


/*-------------------------------------------------------------*/
/*--- end                                        crctable.c ---*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*--- Table for randomising repetitive blocks               ---*/
/*---                                           randtable.c ---*/
/*-------------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-2000 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@acm.org
  bzip2/libbzip2 version 1.0 of 21 March 2000

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/



/*---------------------------------------------*/
static Int32 BZ2_rNums[512] = { 
   619, 720, 127, 481, 931, 816, 813, 233, 566, 247, 
   985, 724, 205, 454, 863, 491, 741, 242, 949, 214, 
   733, 859, 335, 708, 621, 574, 73, 654, 730, 472, 
   419, 436, 278, 496, 867, 210, 399, 680, 480, 51, 
   878, 465, 811, 169, 869, 675, 611, 697, 867, 561, 
   862, 687, 507, 283, 482, 129, 807, 591, 733, 623, 
   150, 238, 59, 379, 684, 877, 625, 169, 643, 105, 
   170, 607, 520, 932, 727, 476, 693, 425, 174, 647, 
   73, 122, 335, 530, 442, 853, 695, 249, 445, 515, 
   909, 545, 703, 919, 874, 474, 882, 500, 594, 612, 
   641, 801, 220, 162, 819, 984, 589, 513, 495, 799, 
   161, 604, 958, 533, 221, 400, 386, 867, 600, 782, 
   382, 596, 414, 171, 516, 375, 682, 485, 911, 276, 
   98, 553, 163, 354, 666, 933, 424, 341, 533, 870, 
   227, 730, 475, 186, 263, 647, 537, 686, 600, 224, 
   469, 68, 770, 919, 190, 373, 294, 822, 808, 206, 
   184, 943, 795, 384, 383, 461, 404, 758, 839, 887, 
   715, 67, 618, 276, 204, 918, 873, 777, 604, 560, 
   951, 160, 578, 722, 79, 804, 96, 409, 713, 940, 
   652, 934, 970, 447, 318, 353, 859, 672, 112, 785, 
   645, 863, 803, 350, 139, 93, 354, 99, 820, 908, 
   609, 772, 154, 274, 580, 184, 79, 626, 630, 742, 
   653, 282, 762, 623, 680, 81, 927, 626, 789, 125, 
   411, 521, 938, 300, 821, 78, 343, 175, 128, 250, 
   170, 774, 972, 275, 999, 639, 495, 78, 352, 126, 
   857, 956, 358, 619, 580, 124, 737, 594, 701, 612, 
   669, 112, 134, 694, 363, 992, 809, 743, 168, 974, 
   944, 375, 748, 52, 600, 747, 642, 182, 862, 81, 
   344, 805, 988, 739, 511, 655, 814, 334, 249, 515, 
   897, 955, 664, 981, 649, 113, 974, 459, 893, 228, 
   433, 837, 553, 268, 926, 240, 102, 654, 459, 51, 
   686, 754, 806, 760, 493, 403, 415, 394, 687, 700, 
   946, 670, 656, 610, 738, 392, 760, 799, 887, 653, 
   978, 321, 576, 617, 626, 502, 894, 679, 243, 440, 
   680, 879, 194, 572, 640, 724, 926, 56, 204, 700, 
   707, 151, 457, 449, 797, 195, 791, 558, 945, 679, 
   297, 59, 87, 824, 713, 663, 412, 693, 342, 606, 
   134, 108, 571, 364, 631, 212, 174, 643, 304, 329, 
   343, 97, 430, 751, 497, 314, 983, 374, 822, 928, 
   140, 206, 73, 263, 980, 736, 876, 478, 430, 305, 
   170, 514, 364, 692, 829, 82, 855, 953, 676, 246, 
   369, 970, 294, 750, 807, 827, 150, 790, 288, 923, 
   804, 378, 215, 828, 592, 281, 565, 555, 710, 82, 
   896, 831, 547, 261, 524, 462, 293, 465, 502, 56, 
   661, 821, 976, 991, 658, 869, 905, 758, 745, 193, 
   768, 550, 608, 933, 378, 286, 215, 979, 792, 961, 
   61, 688, 793, 644, 986, 403, 106, 366, 905, 644, 
   372, 567, 466, 434, 645, 210, 389, 550, 919, 135, 
   780, 773, 635, 389, 707, 100, 626, 958, 165, 504, 
   920, 176, 193, 713, 857, 265, 203, 50, 668, 108, 
   645, 990, 626, 197, 510, 357, 358, 850, 858, 364, 
   936, 638
};


/*-------------------------------------------------------------*/
/*--- end                                       randtable.c ---*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*--- Decompression machinery                               ---*/
/*---                                          decompress.c ---*/
/*-------------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-2000 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@acm.org
  bzip2/libbzip2 version 1.0 of 21 March 2000

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/


/*---------------------------------------------------*/
static
void makeMaps_d ( DState* s )
{
   Int32 i;
   s->nInUse = 0;
   for (i = 0; i < 256; i++)
      if (s->inUse[i]) {
         s->seqToUnseq[s->nInUse] = i;
         s->nInUse++;
      }
}


/*---------------------------------------------------*/
#define RETURN(rrr)                               \
   { retVal = rrr; goto save_state_and_return; };

#define GET_BITS(lll,vvv,nnn)                     \
   case lll: s->state = lll;                      \
   while (True) {                                 \
      if (s->bsLive >= nnn) {                     \
         UInt32 v;                                \
         v = (s->bsBuff >>                        \
             (s->bsLive-nnn)) & ((1 << nnn)-1);   \
         s->bsLive -= nnn;                        \
         vvv = v;                                 \
         break;                                   \
      }                                           \
      if (s->strm->avail_in == 0) RETURN(BZ_OK);  \
      s->bsBuff                                   \
         = (s->bsBuff << 8) |                     \
           ((UInt32)                              \
              (*((UChar*)(s->strm->next_in))));   \
      s->bsLive += 8;                             \
      s->strm->next_in++;                         \
      s->strm->avail_in--;                        \
      s->strm->total_in_lo32++;                   \
      if (s->strm->total_in_lo32 == 0)            \
         s->strm->total_in_hi32++;                \
   }

#define GET_UCHAR(lll,uuu)                        \
   GET_BITS(lll,uuu,8)

#define GET_BIT(lll,uuu)                          \
   GET_BITS(lll,uuu,1)

/*---------------------------------------------------*/
#define GET_MTF_VAL(label1,label2,lval)           \
{                                                 \
   if (groupPos == 0) {                           \
      groupNo++;                                  \
      if (groupNo >= nSelectors)                  \
         RETURN(BZ_DATA_ERROR);                   \
      groupPos = BZ_G_SIZE;                       \
      gSel = s->selector[groupNo];                \
      gMinlen = s->minLens[gSel];                 \
      gLimit = &(s->limit[gSel][0]);              \
      gPerm = &(s->perm[gSel][0]);                \
      gBase = &(s->base[gSel][0]);                \
   }                                              \
   groupPos--;                                    \
   zn = gMinlen;                                  \
   GET_BITS(label1, zvec, zn);                    \
   while (1) {                                    \
      if (zn > 20 /* the longest code */)         \
         RETURN(BZ_DATA_ERROR);                   \
      if (zvec <= gLimit[zn]) break;              \
      zn++;                                       \
      GET_BIT(label2, zj);                        \
      zvec = (zvec << 1) | zj;                    \
   };                                             \
   if (zvec - gBase[zn] < 0                       \
       || zvec - gBase[zn] >= BZ_MAX_ALPHA_SIZE)  \
      RETURN(BZ_DATA_ERROR);                      \
   lval = gPerm[zvec - gBase[zn]];                \
}


/*---------------------------------------------------*/
static Int32 BZ2_decompress ( DState* s )
{
   UChar      uc;
   Int32      retVal;
   Int32      minLen, maxLen;
   bz_stream* strm = s->strm;

   /* stuff that needs to be saved/restored */
   Int32  i;
   Int32  j;
   Int32  t;
   Int32  alphaSize;
   Int32  nGroups;
   Int32  nSelectors;
   Int32  EOB;
   Int32  groupNo;
   Int32  groupPos;
   Int32  nextSym;
   Int32  nblockMAX;
   Int32  nblock;
   Int32  es;
   Int32  N;
   Int32  curr;
   Int32  zt;
   Int32  zn; 
   Int32  zvec;
   Int32  zj;
   Int32  gSel;
   Int32  gMinlen;
   Int32* gLimit;
   Int32* gBase;
   Int32* gPerm;

   if (s->state == BZ_X_MAGIC_1) {
      /*initialise the save area*/
      s->save_i           = 0;
      s->save_j           = 0;
      s->save_t           = 0;
      s->save_alphaSize   = 0;
      s->save_nGroups     = 0;
      s->save_nSelectors  = 0;
      s->save_EOB         = 0;
      s->save_groupNo     = 0;
      s->save_groupPos    = 0;
      s->save_nextSym     = 0;
      s->save_nblockMAX   = 0;
      s->save_nblock      = 0;
      s->save_es          = 0;
      s->save_N           = 0;
      s->save_curr        = 0;
      s->save_zt          = 0;
      s->save_zn          = 0;
      s->save_zvec        = 0;
      s->save_zj          = 0;
      s->save_gSel        = 0;
      s->save_gMinlen     = 0;
      s->save_gLimit      = NULL;
      s->save_gBase       = NULL;
      s->save_gPerm       = NULL;
   }

   /*restore from the save area*/
   i           = s->save_i;
   j           = s->save_j;
   t           = s->save_t;
   alphaSize   = s->save_alphaSize;
   nGroups     = s->save_nGroups;
   nSelectors  = s->save_nSelectors;
   EOB         = s->save_EOB;
   groupNo     = s->save_groupNo;
   groupPos    = s->save_groupPos;
   nextSym     = s->save_nextSym;
   nblockMAX   = s->save_nblockMAX;
   nblock      = s->save_nblock;
   es          = s->save_es;
   N           = s->save_N;
   curr        = s->save_curr;
   zt          = s->save_zt;
   zn          = s->save_zn; 
   zvec        = s->save_zvec;
   zj          = s->save_zj;
   gSel        = s->save_gSel;
   gMinlen     = s->save_gMinlen;
   gLimit      = s->save_gLimit;
   gBase       = s->save_gBase;
   gPerm       = s->save_gPerm;

   retVal = BZ_OK;

   switch (s->state) {

      GET_UCHAR(BZ_X_MAGIC_1, uc);
      if (uc != 'B') RETURN(BZ_DATA_ERROR_MAGIC);

      GET_UCHAR(BZ_X_MAGIC_2, uc);
      if (uc != 'Z') RETURN(BZ_DATA_ERROR_MAGIC);

      GET_UCHAR(BZ_X_MAGIC_3, uc)
      if (uc != 'h') RETURN(BZ_DATA_ERROR_MAGIC);

      GET_BITS(BZ_X_MAGIC_4, s->blockSize100k, 8)
      if (s->blockSize100k < '1' || 
          s->blockSize100k > '9') RETURN(BZ_DATA_ERROR_MAGIC);
      s->blockSize100k -= '0';

      if (s->smallDecompress) {
         s->ll16 = BZALLOC( s->blockSize100k * 100000 * sizeof(UInt16) );
         s->ll4  = BZALLOC( 
                      ((1 + s->blockSize100k * 100000) >> 1) * sizeof(UChar) 
                   );
         if (s->ll16 == NULL || s->ll4 == NULL) RETURN(BZ_MEM_ERROR);
      } else {
         s->tt  = BZALLOC( s->blockSize100k * 100000 * sizeof(Int32) );
         if (s->tt == NULL) RETURN(BZ_MEM_ERROR);
      }

      GET_UCHAR(BZ_X_BLKHDR_1, uc);

      if (uc == 0x17) goto endhdr_2;
      if (uc != 0x31) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_BLKHDR_2, uc);
      if (uc != 0x41) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_BLKHDR_3, uc);
      if (uc != 0x59) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_BLKHDR_4, uc);
      if (uc != 0x26) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_BLKHDR_5, uc);
      if (uc != 0x53) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_BLKHDR_6, uc);
      if (uc != 0x59) RETURN(BZ_DATA_ERROR);

      s->currBlockNo++;
      if (s->verbosity >= 2)
         VPrintf1 ( "\n    [%d: huff+mtf ", s->currBlockNo );

      s->storedBlockCRC = 0;
      GET_UCHAR(BZ_X_BCRC_1, uc);
      s->storedBlockCRC = (s->storedBlockCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_BCRC_2, uc);
      s->storedBlockCRC = (s->storedBlockCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_BCRC_3, uc);
      s->storedBlockCRC = (s->storedBlockCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_BCRC_4, uc);
      s->storedBlockCRC = (s->storedBlockCRC << 8) | ((UInt32)uc);

      GET_BITS(BZ_X_RANDBIT, s->blockRandomised, 1);

      s->origPtr = 0;
      GET_UCHAR(BZ_X_ORIGPTR_1, uc);
      s->origPtr = (s->origPtr << 8) | ((Int32)uc);
      GET_UCHAR(BZ_X_ORIGPTR_2, uc);
      s->origPtr = (s->origPtr << 8) | ((Int32)uc);
      GET_UCHAR(BZ_X_ORIGPTR_3, uc);
      s->origPtr = (s->origPtr << 8) | ((Int32)uc);

      if (s->origPtr < 0)
         RETURN(BZ_DATA_ERROR);
      if (s->origPtr > 10 + 100000*s->blockSize100k) 
         RETURN(BZ_DATA_ERROR);

      /*--- Receive the mapping table ---*/
      for (i = 0; i < 16; i++) {
         GET_BIT(BZ_X_MAPPING_1, uc);
         if (uc == 1) 
            s->inUse16[i] = True; else 
            s->inUse16[i] = False;
      }

      for (i = 0; i < 256; i++) s->inUse[i] = False;

      for (i = 0; i < 16; i++)
         if (s->inUse16[i])
            for (j = 0; j < 16; j++) {
               GET_BIT(BZ_X_MAPPING_2, uc);
               if (uc == 1) s->inUse[i * 16 + j] = True;
            }
      makeMaps_d ( s );
      if (s->nInUse == 0) RETURN(BZ_DATA_ERROR);
      alphaSize = s->nInUse+2;

      /*--- Now the selectors ---*/
      GET_BITS(BZ_X_SELECTOR_1, nGroups, 3);
      if (nGroups < 2 || nGroups > 6) RETURN(BZ_DATA_ERROR);
      GET_BITS(BZ_X_SELECTOR_2, nSelectors, 15);
      if (nSelectors < 1) RETURN(BZ_DATA_ERROR);
      for (i = 0; i < nSelectors; i++) {
         j = 0;
         while (True) {
            GET_BIT(BZ_X_SELECTOR_3, uc);
            if (uc == 0) break;
            j++;
            if (j >= nGroups) RETURN(BZ_DATA_ERROR);
         }
         s->selectorMtf[i] = j;
      }

      /*--- Undo the MTF values for the selectors. ---*/
      {
         UChar pos[BZ_N_GROUPS], tmp, v;
         for (v = 0; v < nGroups; v++) pos[v] = v;
   
         for (i = 0; i < nSelectors; i++) {
            v = s->selectorMtf[i];
            tmp = pos[v];
            while (v > 0) { pos[v] = pos[v-1]; v--; }
            pos[0] = tmp;
            s->selector[i] = tmp;
         }
      }

      /*--- Now the coding tables ---*/
      for (t = 0; t < nGroups; t++) {
         GET_BITS(BZ_X_CODING_1, curr, 5);
         for (i = 0; i < alphaSize; i++) {
            while (True) {
               if (curr < 1 || curr > 20) RETURN(BZ_DATA_ERROR);
               GET_BIT(BZ_X_CODING_2, uc);
               if (uc == 0) break;
               GET_BIT(BZ_X_CODING_3, uc);
               if (uc == 0) curr++; else curr--;
            }
            s->len[t][i] = curr;
         }
      }

      /*--- Create the Huffman decoding tables ---*/
      for (t = 0; t < nGroups; t++) {
         minLen = 32;
         maxLen = 0;
         for (i = 0; i < alphaSize; i++) {
            if (s->len[t][i] > maxLen) maxLen = s->len[t][i];
            if (s->len[t][i] < minLen) minLen = s->len[t][i];
         }
         BZ2_hbCreateDecodeTables ( 
            &(s->limit[t][0]), 
            &(s->base[t][0]), 
            &(s->perm[t][0]), 
            &(s->len[t][0]),
            minLen, maxLen, alphaSize
         );
         s->minLens[t] = minLen;
      }

      /*--- Now the MTF values ---*/

      EOB      = s->nInUse+1;
      nblockMAX = 100000 * s->blockSize100k;
      groupNo  = -1;
      groupPos = 0;

      for (i = 0; i <= 255; i++) s->unzftab[i] = 0;

      /*-- MTF init --*/
      {
         Int32 ii, jj, kk;
         kk = MTFA_SIZE-1;
         for (ii = 256 / MTFL_SIZE - 1; ii >= 0; ii--) {
            for (jj = MTFL_SIZE-1; jj >= 0; jj--) {
               s->mtfa[kk] = (UChar)(ii * MTFL_SIZE + jj);
               kk--;
            }
            s->mtfbase[ii] = kk + 1;
         }
      }
      /*-- end MTF init --*/

      nblock = 0;
      GET_MTF_VAL(BZ_X_MTF_1, BZ_X_MTF_2, nextSym);

      while (True) {

         if (nextSym == EOB) break;

         if (nextSym == BZ_RUNA || nextSym == BZ_RUNB) {

            es = -1;
            N = 1;
            do {
               if (nextSym == BZ_RUNA) es = es + (0+1) * N; else
               if (nextSym == BZ_RUNB) es = es + (1+1) * N;
               N = N * 2;
               GET_MTF_VAL(BZ_X_MTF_3, BZ_X_MTF_4, nextSym);
            }
               while (nextSym == BZ_RUNA || nextSym == BZ_RUNB);

            es++;
            uc = s->seqToUnseq[ s->mtfa[s->mtfbase[0]] ];
            s->unzftab[uc] += es;

            if (s->smallDecompress)
               while (es > 0) {
                  if (nblock >= nblockMAX) RETURN(BZ_DATA_ERROR);
                  s->ll16[nblock] = (UInt16)uc;
                  nblock++;
                  es--;
               }
            else
               while (es > 0) {
                  if (nblock >= nblockMAX) RETURN(BZ_DATA_ERROR);
                  s->tt[nblock] = (UInt32)uc;
                  nblock++;
                  es--;
               };

            continue;

         } else {

            if (nblock >= nblockMAX) RETURN(BZ_DATA_ERROR);

            /*-- uc = MTF ( nextSym-1 ) --*/
            {
               Int32 ii, jj, kk, pp, lno, off;
               UInt32 nn;
               nn = (UInt32)(nextSym - 1);

               if (nn < MTFL_SIZE) {
                  /* avoid general-case expense */
                  pp = s->mtfbase[0];
                  uc = s->mtfa[pp+nn];
                  while (nn > 3) {
                     Int32 z = pp+nn;
                     s->mtfa[(z)  ] = s->mtfa[(z)-1];
                     s->mtfa[(z)-1] = s->mtfa[(z)-2];
                     s->mtfa[(z)-2] = s->mtfa[(z)-3];
                     s->mtfa[(z)-3] = s->mtfa[(z)-4];
                     nn -= 4;
                  }
                  while (nn > 0) { 
                     s->mtfa[(pp+nn)] = s->mtfa[(pp+nn)-1]; nn--; 
                  };
                  s->mtfa[pp] = uc;
               } else { 
                  /* general case */
                  lno = nn / MTFL_SIZE;
                  off = nn % MTFL_SIZE;
                  pp = s->mtfbase[lno] + off;
                  uc = s->mtfa[pp];
                  while (pp > s->mtfbase[lno]) { 
                     s->mtfa[pp] = s->mtfa[pp-1]; pp--; 
                  };
                  s->mtfbase[lno]++;
                  while (lno > 0) {
                     s->mtfbase[lno]--;
                     s->mtfa[s->mtfbase[lno]] 
                        = s->mtfa[s->mtfbase[lno-1] + MTFL_SIZE - 1];
                     lno--;
                  }
                  s->mtfbase[0]--;
                  s->mtfa[s->mtfbase[0]] = uc;
                  if (s->mtfbase[0] == 0) {
                     kk = MTFA_SIZE-1;
                     for (ii = 256 / MTFL_SIZE-1; ii >= 0; ii--) {
                        for (jj = MTFL_SIZE-1; jj >= 0; jj--) {
                           s->mtfa[kk] = s->mtfa[s->mtfbase[ii] + jj];
                           kk--;
                        }
                        s->mtfbase[ii] = kk + 1;
                     }
                  }
               }
            }
            /*-- end uc = MTF ( nextSym-1 ) --*/

            s->unzftab[s->seqToUnseq[uc]]++;
            if (s->smallDecompress)
               s->ll16[nblock] = (UInt16)(s->seqToUnseq[uc]); else
               s->tt[nblock]   = (UInt32)(s->seqToUnseq[uc]);
            nblock++;

            GET_MTF_VAL(BZ_X_MTF_5, BZ_X_MTF_6, nextSym);
            continue;
         }
      }

      /* Now we know what nblock is, we can do a better sanity
         check on s->origPtr.
      */
      if (s->origPtr < 0 || s->origPtr >= nblock)
         RETURN(BZ_DATA_ERROR);

      s->state_out_len = 0;
      s->state_out_ch  = 0;
      BZ_INITIALISE_CRC ( s->calculatedBlockCRC );
      s->state = BZ_X_OUTPUT;
      if (s->verbosity >= 2) VPrintf0 ( "rt+rld" );

      /*-- Set up cftab to facilitate generation of T^(-1) --*/
      s->cftab[0] = 0;
      for (i = 1; i <= 256; i++) s->cftab[i] = s->unzftab[i-1];
      for (i = 1; i <= 256; i++) s->cftab[i] += s->cftab[i-1];

      if (s->smallDecompress) {

         /*-- Make a copy of cftab, used in generation of T --*/
         for (i = 0; i <= 256; i++) s->cftabCopy[i] = s->cftab[i];

         /*-- compute the T vector --*/
         for (i = 0; i < nblock; i++) {
            uc = (UChar)(s->ll16[i]);
            SET_LL(i, s->cftabCopy[uc]);
            s->cftabCopy[uc]++;
         }

         /*-- Compute T^(-1) by pointer reversal on T --*/
         i = s->origPtr;
         j = GET_LL(i);
         do {
            Int32 tmp = GET_LL(j);
            SET_LL(j, i);
            i = j;
            j = tmp;
         }
            while (i != s->origPtr);

         s->tPos = s->origPtr;
         s->nblock_used = 0;
         if (s->blockRandomised) {
            BZ_RAND_INIT_MASK;
            BZ_GET_SMALL(s->k0); s->nblock_used++;
            BZ_RAND_UPD_MASK; s->k0 ^= BZ_RAND_MASK; 
         } else {
            BZ_GET_SMALL(s->k0); s->nblock_used++;
         }

      } else {

         /*-- compute the T^(-1) vector --*/
         for (i = 0; i < nblock; i++) {
            uc = (UChar)(s->tt[i] & 0xff);
            s->tt[s->cftab[uc]] |= (i << 8);
            s->cftab[uc]++;
         }

         s->tPos = s->tt[s->origPtr] >> 8;
         s->nblock_used = 0;
         if (s->blockRandomised) {
            BZ_RAND_INIT_MASK;
            BZ_GET_FAST(s->k0); s->nblock_used++;
            BZ_RAND_UPD_MASK; s->k0 ^= BZ_RAND_MASK; 
         } else {
            BZ_GET_FAST(s->k0); s->nblock_used++;
         }

      }

      RETURN(BZ_OK);



    endhdr_2:

      GET_UCHAR(BZ_X_ENDHDR_2, uc);
      if (uc != 0x72) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_ENDHDR_3, uc);
      if (uc != 0x45) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_ENDHDR_4, uc);
      if (uc != 0x38) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_ENDHDR_5, uc);
      if (uc != 0x50) RETURN(BZ_DATA_ERROR);
      GET_UCHAR(BZ_X_ENDHDR_6, uc);
      if (uc != 0x90) RETURN(BZ_DATA_ERROR);

      s->storedCombinedCRC = 0;
      GET_UCHAR(BZ_X_CCRC_1, uc);
      s->storedCombinedCRC = (s->storedCombinedCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_CCRC_2, uc);
      s->storedCombinedCRC = (s->storedCombinedCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_CCRC_3, uc);
      s->storedCombinedCRC = (s->storedCombinedCRC << 8) | ((UInt32)uc);
      GET_UCHAR(BZ_X_CCRC_4, uc);
      s->storedCombinedCRC = (s->storedCombinedCRC << 8) | ((UInt32)uc);

      s->state = BZ_X_IDLE;
      RETURN(BZ_STREAM_END);

      default: AssertH ( False, 4001 );
   }

   AssertH ( False, 4002 );

   save_state_and_return:

   s->save_i           = i;
   s->save_j           = j;
   s->save_t           = t;
   s->save_alphaSize   = alphaSize;
   s->save_nGroups     = nGroups;
   s->save_nSelectors  = nSelectors;
   s->save_EOB         = EOB;
   s->save_groupNo     = groupNo;
   s->save_groupPos    = groupPos;
   s->save_nextSym     = nextSym;
   s->save_nblockMAX   = nblockMAX;
   s->save_nblock      = nblock;
   s->save_es          = es;
   s->save_N           = N;
   s->save_curr        = curr;
   s->save_zt          = zt;
   s->save_zn          = zn;
   s->save_zvec        = zvec;
   s->save_zj          = zj;
   s->save_gSel        = gSel;
   s->save_gMinlen     = gMinlen;
   s->save_gLimit      = gLimit;
   s->save_gBase       = gBase;
   s->save_gPerm       = gPerm;

   return retVal;   
}


/*-------------------------------------------------------------*/
/*--- end                                      decompress.c ---*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*--- Library top-level functions.                          ---*/
/*---                                               bzlib.c ---*/
/*-------------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-2000 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@acm.org
  bzip2/libbzip2 version 1.0 of 21 March 2000

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/

/*--
   CHANGES
   ~~~~~~~
   0.9.0 -- original version.

   0.9.0a/b -- no changes in this file.

   0.9.0c
      * made zero-length BZ_FLUSH work correctly in bzCompress().
      * fixed bzWrite/bzRead to ignore zero-length requests.
      * fixed bzread to correctly handle read requests after EOF.
      * wrong parameter order in call to bzDecompressInit in
        bzBuffToBuffDecompress.  Fixed.
--*/


/*---------------------------------------------------*/
#ifndef BZ_NO_STDIO
void BZ2_bz__AssertH__fail ( int errcode )
{
   fprintf(stderr, 
      "\n\nbzip2/libbzip2: internal error number %d.\n"
      "This is a bug in bzip2/libbzip2, %s.\n"
      "Please report it to me at: jseward@acm.org.  If this happened\n"
      "when you were using some program which uses libbzip2 as a\n"
      "component, you should also report this bug to the author(s)\n"
      "of that program.  Please make an effort to report this bug;\n"
      "timely and accurate bug reports eventually lead to higher\n"
      "quality software.  Thanks.  Julian Seward, 21 March 2000.\n\n",
      errcode,
      BZ2_bzlibVersion()
   );
   exit(3);
}
#endif


/*---------------------------------------------------*/
static
int bz_config_ok ( void )
{
   if (sizeof(int)   != 4) return 0;
   if (sizeof(short) != 2) return 0;
   if (sizeof(char)  != 1) return 0;
   return 1;
}


/*---------------------------------------------------*/
static
void* default_bzalloc ( void* opaque __attribute__ ((unused)),
			Int32 items, Int32 size )
{
   void* v = malloc ( items * size );
   return v;
}

static
void default_bzfree ( void* opaque __attribute__ ((unused)),
		      void* addr )
{
   if (addr != NULL) free ( addr );
}

/*---------------------------------------------------*/
#define ADD_CHAR_TO_BLOCK(zs,zchh0)               \
{                                                 \
   UInt32 zchh = (UInt32)(zchh0);                 \
   /*-- fast track the common case --*/           \
   if (zchh != zs->state_in_ch &&                 \
       zs->state_in_len == 1) {                   \
      UChar ch = (UChar)(zs->state_in_ch);        \
      BZ_UPDATE_CRC( zs->blockCRC, ch );          \
      zs->inUse[zs->state_in_ch] = True;          \
      zs->block[zs->nblock] = (UChar)ch;          \
      zs->nblock++;                               \
      zs->state_in_ch = zchh;                     \
   }                                              \
   else                                           \
   /*-- general, uncommon cases --*/              \
   if (zchh != zs->state_in_ch ||                 \
      zs->state_in_len == 255) {                  \
      if (zs->state_in_ch < 256)                  \
         add_pair_to_block ( zs );                \
      zs->state_in_ch = zchh;                     \
      zs->state_in_len = 1;                       \
   } else {                                       \
      zs->state_in_len++;                         \
   }                                              \
}


/*---------------------------------------------------*/
/*--- Decompression stuff                         ---*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
int BZ_API(BZ2_bzDecompressInit) 
                     ( bz_stream* strm, 
                       int        verbosity,
                       int        small )
{
   DState* s;

   if (!bz_config_ok()) return BZ_CONFIG_ERROR;

   if (strm == NULL) return BZ_PARAM_ERROR;
   if (small != 0 && small != 1) return BZ_PARAM_ERROR;
   if (verbosity < 0 || verbosity > 4) return BZ_PARAM_ERROR;

   if (strm->bzalloc == NULL) strm->bzalloc = default_bzalloc;
   if (strm->bzfree == NULL) strm->bzfree = default_bzfree;

   s = BZALLOC( sizeof(DState) );
   if (s == NULL) return BZ_MEM_ERROR;
   s->strm                  = strm;
   strm->state              = s;
   s->state                 = BZ_X_MAGIC_1;
   s->bsLive                = 0;
   s->bsBuff                = 0;
   s->calculatedCombinedCRC = 0;
   strm->total_in_lo32      = 0;
   strm->total_in_hi32      = 0;
   strm->total_out_lo32     = 0;
   strm->total_out_hi32     = 0;
   s->smallDecompress       = (Bool)small;
   s->ll4                   = NULL;
   s->ll16                  = NULL;
   s->tt                    = NULL;
   s->currBlockNo           = 0;
   s->verbosity             = verbosity;

   return BZ_OK;
}


/*---------------------------------------------------*/
static
void unRLE_obuf_to_output_FAST ( DState* s )
{
   UChar k1;

   if (s->blockRandomised) {

      while (True) {
         /* try to finish existing run */
         while (True) {
            if (s->strm->avail_out == 0) return;
            if (s->state_out_len == 0) break;
            *( (UChar*)(s->strm->next_out) ) = s->state_out_ch;
            BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
            s->state_out_len--;
            s->strm->next_out++;
            s->strm->avail_out--;
            s->strm->total_out_lo32++;
            if (s->strm->total_out_lo32 == 0) s->strm->total_out_hi32++;
         }
   
         /* can a new run be started? */
         if (s->nblock_used == s->save_nblock+1) return;
               
   
         s->state_out_len = 1;
         s->state_out_ch = s->k0;
         BZ_GET_FAST(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 2;
         BZ_GET_FAST(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 3;
         BZ_GET_FAST(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         BZ_GET_FAST(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         s->state_out_len = ((Int32)k1) + 4;
         BZ_GET_FAST(s->k0); BZ_RAND_UPD_MASK; 
         s->k0 ^= BZ_RAND_MASK; s->nblock_used++;
      }

   } else {

      /* restore */
      UInt32        c_calculatedBlockCRC = s->calculatedBlockCRC;
      UChar         c_state_out_ch       = s->state_out_ch;
      Int32         c_state_out_len      = s->state_out_len;
      Int32         c_nblock_used        = s->nblock_used;
      Int32         c_k0                 = s->k0;
      UInt32*       c_tt                 = s->tt;
      UInt32        c_tPos               = s->tPos;
      char*         cs_next_out          = s->strm->next_out;
      unsigned int  cs_avail_out         = s->strm->avail_out;
      /* end restore */

      UInt32       avail_out_INIT = cs_avail_out;
      Int32        s_save_nblockPP = s->save_nblock+1;
      unsigned int total_out_lo32_old;

      while (True) {

         /* try to finish existing run */
         if (c_state_out_len > 0) {
            while (True) {
               if (cs_avail_out == 0) goto return_notr;
               if (c_state_out_len == 1) break;
               *( (UChar*)(cs_next_out) ) = c_state_out_ch;
               BZ_UPDATE_CRC ( c_calculatedBlockCRC, c_state_out_ch );
               c_state_out_len--;
               cs_next_out++;
               cs_avail_out--;
            }
            s_state_out_len_eq_one:
            {
               if (cs_avail_out == 0) { 
                  c_state_out_len = 1; goto return_notr;
               };
               *( (UChar*)(cs_next_out) ) = c_state_out_ch;
               BZ_UPDATE_CRC ( c_calculatedBlockCRC, c_state_out_ch );
               cs_next_out++;
               cs_avail_out--;
            }
         }   
         /* can a new run be started? */
         if (c_nblock_used == s_save_nblockPP) {
            c_state_out_len = 0; goto return_notr;
         };   
         c_state_out_ch = c_k0;
         BZ_GET_FAST_C(k1); c_nblock_used++;
         if (k1 != c_k0) { 
            c_k0 = k1; goto s_state_out_len_eq_one; 
         };
         if (c_nblock_used == s_save_nblockPP) 
            goto s_state_out_len_eq_one;
   
         c_state_out_len = 2;
         BZ_GET_FAST_C(k1); c_nblock_used++;
         if (c_nblock_used == s_save_nblockPP) continue;
         if (k1 != c_k0) { c_k0 = k1; continue; };
   
         c_state_out_len = 3;
         BZ_GET_FAST_C(k1); c_nblock_used++;
         if (c_nblock_used == s_save_nblockPP) continue;
         if (k1 != c_k0) { c_k0 = k1; continue; };
   
         BZ_GET_FAST_C(k1); c_nblock_used++;
         c_state_out_len = ((Int32)k1) + 4;
         BZ_GET_FAST_C(c_k0); c_nblock_used++;
      }

      return_notr:
      total_out_lo32_old = s->strm->total_out_lo32;
      s->strm->total_out_lo32 += (avail_out_INIT - cs_avail_out);
      if (s->strm->total_out_lo32 < total_out_lo32_old)
         s->strm->total_out_hi32++;

      /* save */
      s->calculatedBlockCRC = c_calculatedBlockCRC;
      s->state_out_ch       = c_state_out_ch;
      s->state_out_len      = c_state_out_len;
      s->nblock_used        = c_nblock_used;
      s->k0                 = c_k0;
      s->tt                 = c_tt;
      s->tPos               = c_tPos;
      s->strm->next_out     = cs_next_out;
      s->strm->avail_out    = cs_avail_out;
      /* end save */
   }
}



/*---------------------------------------------------*/
static
void unRLE_obuf_to_output_SMALL ( DState* s )
{
   UChar k1;

   if (s->blockRandomised) {

      while (True) {
         /* try to finish existing run */
         while (True) {
            if (s->strm->avail_out == 0) return;
            if (s->state_out_len == 0) break;
            *( (UChar*)(s->strm->next_out) ) = s->state_out_ch;
            BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
            s->state_out_len--;
            s->strm->next_out++;
            s->strm->avail_out--;
            s->strm->total_out_lo32++;
            if (s->strm->total_out_lo32 == 0) s->strm->total_out_hi32++;
         }
   
         /* can a new run be started? */
         if (s->nblock_used == s->save_nblock+1) return;
               
   
         s->state_out_len = 1;
         s->state_out_ch = s->k0;
         BZ_GET_SMALL(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 2;
         BZ_GET_SMALL(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 3;
         BZ_GET_SMALL(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         BZ_GET_SMALL(k1); BZ_RAND_UPD_MASK; 
         k1 ^= BZ_RAND_MASK; s->nblock_used++;
         s->state_out_len = ((Int32)k1) + 4;
         BZ_GET_SMALL(s->k0); BZ_RAND_UPD_MASK; 
         s->k0 ^= BZ_RAND_MASK; s->nblock_used++;
      }

   } else {

      while (True) {
         /* try to finish existing run */
         while (True) {
            if (s->strm->avail_out == 0) return;
            if (s->state_out_len == 0) break;
            *( (UChar*)(s->strm->next_out) ) = s->state_out_ch;
            BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
            s->state_out_len--;
            s->strm->next_out++;
            s->strm->avail_out--;
            s->strm->total_out_lo32++;
            if (s->strm->total_out_lo32 == 0) s->strm->total_out_hi32++;
         }
   
         /* can a new run be started? */
         if (s->nblock_used == s->save_nblock+1) return;
   
         s->state_out_len = 1;
         s->state_out_ch = s->k0;
         BZ_GET_SMALL(k1); s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 2;
         BZ_GET_SMALL(k1); s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         s->state_out_len = 3;
         BZ_GET_SMALL(k1); s->nblock_used++;
         if (s->nblock_used == s->save_nblock+1) continue;
         if (k1 != s->k0) { s->k0 = k1; continue; };
   
         BZ_GET_SMALL(k1); s->nblock_used++;
         s->state_out_len = ((Int32)k1) + 4;
         BZ_GET_SMALL(s->k0); s->nblock_used++;
      }

   }
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzDecompress) ( bz_stream *strm )
{
   DState* s;
   if (strm == NULL) return BZ_PARAM_ERROR;
   s = strm->state;
   if (s == NULL) return BZ_PARAM_ERROR;
   if (s->strm != strm) return BZ_PARAM_ERROR;

   while (True) {
      if (s->state == BZ_X_IDLE) return BZ_SEQUENCE_ERROR;
      if (s->state == BZ_X_OUTPUT) {
         if (s->smallDecompress)
            unRLE_obuf_to_output_SMALL ( s ); else
            unRLE_obuf_to_output_FAST  ( s );
         if (s->nblock_used == s->save_nblock+1 && s->state_out_len == 0) {
            BZ_FINALISE_CRC ( s->calculatedBlockCRC );
            if (s->verbosity >= 3) 
               VPrintf2 ( " {0x%x, 0x%x}", s->storedBlockCRC, 
                          s->calculatedBlockCRC );
            if (s->verbosity >= 2) VPrintf0 ( "]" );
            if (s->calculatedBlockCRC != s->storedBlockCRC)
               return BZ_DATA_ERROR;
            s->calculatedCombinedCRC 
               = (s->calculatedCombinedCRC << 1) | 
                    (s->calculatedCombinedCRC >> 31);
            s->calculatedCombinedCRC ^= s->calculatedBlockCRC;
            s->state = BZ_X_BLKHDR_1;
         } else {
            return BZ_OK;
         }
      }
      if (s->state >= BZ_X_MAGIC_1) {
         Int32 r = BZ2_decompress ( s );
         if (r == BZ_STREAM_END) {
            if (s->verbosity >= 3)
               VPrintf2 ( "\n    combined CRCs: stored = 0x%x, computed = 0x%x", 
                          s->storedCombinedCRC, s->calculatedCombinedCRC );
            if (s->calculatedCombinedCRC != s->storedCombinedCRC)
               return BZ_DATA_ERROR;
            return r;
         }
         if (s->state != BZ_X_OUTPUT) return r;
      }
   }

   AssertH ( 0, 6001 );

   return 0;  /*NOTREACHED*/
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzDecompressEnd)  ( bz_stream *strm )
{
   DState* s;
   
   if (strm == NULL) return BZ_PARAM_ERROR;
   s = strm->state;
   if (s == NULL) return BZ_PARAM_ERROR;
   if (s->strm != strm) return BZ_PARAM_ERROR;

   if (s->tt   != NULL) BZFREE(s->tt);
   if (s->ll16 != NULL) BZFREE(s->ll16);
   if (s->ll4  != NULL) BZFREE(s->ll4);

   BZFREE(strm->state);
   strm->state = NULL;

   return BZ_OK;
}


#ifndef BZ_NO_STDIO
/*---------------------------------------------------*/
/*--- File I/O stuff                              ---*/
/*---------------------------------------------------*/

#define BZ_SETERR(eee)                    \
{                                         \
   if (bzerror != NULL) *bzerror = eee;   \
   if (bzf != NULL) bzf->lastErr = eee;   \
}

typedef 
   struct {
      FILE*     handle;
      Char      buf[BZ_MAX_UNUSED];
      Int32     bufN;
      Bool      writing;
      bz_stream strm;
      Int32     lastErr;
      Bool      initialisedOk;
   }
   bzFile;


/*---------------------------------------------*/
static Bool myfeof ( FILE* f )
{
   Int32 c = fgetc ( f );
   if (c == EOF) return True;
   ungetc ( c, f );
   return False;
}


/*---------------------------------------------------*/
BZFILE* BZ_API(BZ2_bzWriteOpen) 
                    ( int*  bzerror,      
                      FILE* f, 
                      int   blockSize100k, 
                      int   verbosity,
                      int   workFactor )
{
   Int32   ret;
   bzFile* bzf = NULL;

   BZ_SETERR(BZ_OK);

   if (f == NULL ||
       (blockSize100k < 1 || blockSize100k > 9) ||
       (workFactor < 0 || workFactor > 250) ||
       (verbosity < 0 || verbosity > 4))
      { BZ_SETERR(BZ_PARAM_ERROR); return NULL; };

   if (ferror(f))
      { BZ_SETERR(BZ_IO_ERROR); return NULL; };

   bzf = malloc ( sizeof(bzFile) );
   if (bzf == NULL)
      { BZ_SETERR(BZ_MEM_ERROR); return NULL; };

   BZ_SETERR(BZ_OK);
   bzf->initialisedOk = False;
   bzf->bufN          = 0;
   bzf->handle        = f;
   bzf->writing       = True;
   bzf->strm.bzalloc  = NULL;
   bzf->strm.bzfree   = NULL;
   bzf->strm.opaque   = NULL;

   if (workFactor == 0) workFactor = 30;
   ret = BZ2_bzCompressInit ( &(bzf->strm), blockSize100k, 
                              verbosity, workFactor );
   if (ret != BZ_OK)
      { BZ_SETERR(ret); free(bzf); return NULL; };

   bzf->strm.avail_in = 0;
   bzf->initialisedOk = True;
   return bzf;   
}



/*---------------------------------------------------*/
void BZ_API(BZ2_bzWrite)
             ( int*    bzerror, 
               BZFILE* b, 
               void*   buf, 
               int     len )
{
   Int32 n, n2, ret;
   bzFile* bzf = (bzFile*)b;

   BZ_SETERR(BZ_OK);
   if (bzf == NULL || buf == NULL || len < 0)
      { BZ_SETERR(BZ_PARAM_ERROR); return; };
   if (!(bzf->writing))
      { BZ_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (ferror(bzf->handle))
      { BZ_SETERR(BZ_IO_ERROR); return; };

   if (len == 0)
      { BZ_SETERR(BZ_OK); return; };

   bzf->strm.avail_in = len;
   bzf->strm.next_in  = buf;

   while (True) {
      bzf->strm.avail_out = BZ_MAX_UNUSED;
      bzf->strm.next_out = bzf->buf;
      ret = BZ2_bzCompress ( &(bzf->strm), BZ_RUN );
      if (ret != BZ_RUN_OK)
         { BZ_SETERR(ret); return; };

      if (bzf->strm.avail_out < BZ_MAX_UNUSED) {
         n = BZ_MAX_UNUSED - bzf->strm.avail_out;
         n2 = fwrite ( (void*)(bzf->buf), sizeof(UChar), 
                       n, bzf->handle );
         if (n != n2 || ferror(bzf->handle))
            { BZ_SETERR(BZ_IO_ERROR); return; };
      }

      if (bzf->strm.avail_in == 0)
         { BZ_SETERR(BZ_OK); return; };
   }
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzWriteClose)
                  ( int*          bzerror, 
                    BZFILE*       b, 
                    int           abandon,
                    unsigned int* nbytes_in,
                    unsigned int* nbytes_out )
{
   BZ2_bzWriteClose64 ( bzerror, b, abandon, 
                        nbytes_in, NULL, nbytes_out, NULL );
}


void BZ_API(BZ2_bzWriteClose64)
                  ( int*          bzerror, 
                    BZFILE*       b, 
                    int           abandon,
                    unsigned int* nbytes_in_lo32,
                    unsigned int* nbytes_in_hi32,
                    unsigned int* nbytes_out_lo32,
                    unsigned int* nbytes_out_hi32 )
{
   Int32   n, n2, ret;
   bzFile* bzf = (bzFile*)b;

   if (bzf == NULL)
      { BZ_SETERR(BZ_OK); return; };
   if (!(bzf->writing))
      { BZ_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (ferror(bzf->handle))
      { BZ_SETERR(BZ_IO_ERROR); return; };

   if (nbytes_in_lo32 != NULL) *nbytes_in_lo32 = 0;
   if (nbytes_in_hi32 != NULL) *nbytes_in_hi32 = 0;
   if (nbytes_out_lo32 != NULL) *nbytes_out_lo32 = 0;
   if (nbytes_out_hi32 != NULL) *nbytes_out_hi32 = 0;

   if ((!abandon) && bzf->lastErr == BZ_OK) {
      while (True) {
         bzf->strm.avail_out = BZ_MAX_UNUSED;
         bzf->strm.next_out = bzf->buf;
         ret = BZ2_bzCompress ( &(bzf->strm), BZ_FINISH );
         if (ret != BZ_FINISH_OK && ret != BZ_STREAM_END)
            { BZ_SETERR(ret); return; };

         if (bzf->strm.avail_out < BZ_MAX_UNUSED) {
            n = BZ_MAX_UNUSED - bzf->strm.avail_out;
            n2 = fwrite ( (void*)(bzf->buf), sizeof(UChar), 
                          n, bzf->handle );
            if (n != n2 || ferror(bzf->handle))
               { BZ_SETERR(BZ_IO_ERROR); return; };
         }

         if (ret == BZ_STREAM_END) break;
      }
   }

   if ( !abandon && !ferror ( bzf->handle ) ) {
      fflush ( bzf->handle );
      if (ferror(bzf->handle))
         { BZ_SETERR(BZ_IO_ERROR); return; };
   }

   if (nbytes_in_lo32 != NULL)
      *nbytes_in_lo32 = bzf->strm.total_in_lo32;
   if (nbytes_in_hi32 != NULL)
      *nbytes_in_hi32 = bzf->strm.total_in_hi32;
   if (nbytes_out_lo32 != NULL)
      *nbytes_out_lo32 = bzf->strm.total_out_lo32;
   if (nbytes_out_hi32 != NULL)
      *nbytes_out_hi32 = bzf->strm.total_out_hi32;

   BZ_SETERR(BZ_OK);
   BZ2_bzCompressEnd ( &(bzf->strm) );
   free ( bzf );
}


/*---------------------------------------------------*/
BZFILE* BZ_API(BZ2_bzReadOpen) 
                   ( int*  bzerror, 
                     FILE* f, 
                     int   verbosity,
                     int   small,
                     void* unused,
                     int   nUnused )
{
   bzFile* bzf = NULL;
   int     ret;

   BZ_SETERR(BZ_OK);

   if (f == NULL || 
       (small != 0 && small != 1) ||
       (verbosity < 0 || verbosity > 4) ||
       (unused == NULL && nUnused != 0) ||
       (unused != NULL && (nUnused < 0 || nUnused > BZ_MAX_UNUSED)))
      { BZ_SETERR(BZ_PARAM_ERROR); return NULL; };

   if (ferror(f))
      { BZ_SETERR(BZ_IO_ERROR); return NULL; };

   bzf = malloc ( sizeof(bzFile) );
   if (bzf == NULL) 
      { BZ_SETERR(BZ_MEM_ERROR); return NULL; };

   BZ_SETERR(BZ_OK);

   bzf->initialisedOk = False;
   bzf->handle        = f;
   bzf->bufN          = 0;
   bzf->writing       = False;
   bzf->strm.bzalloc  = NULL;
   bzf->strm.bzfree   = NULL;
   bzf->strm.opaque   = NULL;
   
   while (nUnused > 0) {
      bzf->buf[bzf->bufN] = *((UChar*)(unused)); bzf->bufN++;
      unused = ((void*)( 1 + ((UChar*)(unused))  ));
      nUnused--;
   }

   ret = BZ2_bzDecompressInit ( &(bzf->strm), verbosity, small );
   if (ret != BZ_OK)
      { BZ_SETERR(ret); free(bzf); return NULL; };

   bzf->strm.avail_in = bzf->bufN;
   bzf->strm.next_in  = bzf->buf;

   bzf->initialisedOk = True;
   return bzf;   
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzReadClose) ( int *bzerror, BZFILE *b )
{
   bzFile* bzf = (bzFile*)b;

   BZ_SETERR(BZ_OK);
   if (bzf == NULL)
      { BZ_SETERR(BZ_OK); return; };

   if (bzf->writing)
      { BZ_SETERR(BZ_SEQUENCE_ERROR); return; };

   if (bzf->initialisedOk)
      (void)BZ2_bzDecompressEnd ( &(bzf->strm) );
   free ( bzf );
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzRead) 
           ( int*    bzerror, 
             BZFILE* b, 
             void*   buf, 
             int     len )
{
   Int32   n, ret;
   bzFile* bzf = (bzFile*)b;

   BZ_SETERR(BZ_OK);

   if (bzf == NULL || buf == NULL || len < 0)
      { BZ_SETERR(BZ_PARAM_ERROR); return 0; };

   if (bzf->writing)
      { BZ_SETERR(BZ_SEQUENCE_ERROR); return 0; };

   if (len == 0)
      { BZ_SETERR(BZ_OK); return 0; };

   bzf->strm.avail_out = len;
   bzf->strm.next_out = buf;

   while (True) {

      if (ferror(bzf->handle)) 
         { BZ_SETERR(BZ_IO_ERROR); return 0; };

      if (bzf->strm.avail_in == 0 && !myfeof(bzf->handle)) {
         n = fread ( bzf->buf, sizeof(UChar), 
                     BZ_MAX_UNUSED, bzf->handle );
         if (ferror(bzf->handle))
            { BZ_SETERR(BZ_IO_ERROR); return 0; };
         bzf->bufN = n;
         bzf->strm.avail_in = bzf->bufN;
         bzf->strm.next_in = bzf->buf;
      }

      ret = BZ2_bzDecompress ( &(bzf->strm) );

      if (ret != BZ_OK && ret != BZ_STREAM_END)
         { BZ_SETERR(ret); return 0; };

      if (ret == BZ_OK && myfeof(bzf->handle) && 
          bzf->strm.avail_in == 0 && bzf->strm.avail_out > 0)
         { BZ_SETERR(BZ_UNEXPECTED_EOF); return 0; };

      if (ret == BZ_STREAM_END)
         { BZ_SETERR(BZ_STREAM_END);
           return len - bzf->strm.avail_out; };
      if (bzf->strm.avail_out == 0)
         { BZ_SETERR(BZ_OK); return len; };
      
   }

   return 0; /*not reached*/
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzReadGetUnused) 
                     ( int*    bzerror, 
                       BZFILE* b, 
                       void**  unused, 
                       int*    nUnused )
{
   bzFile* bzf = (bzFile*)b;
   if (bzf == NULL)
      { BZ_SETERR(BZ_PARAM_ERROR); return; };
   if (bzf->lastErr != BZ_STREAM_END)
      { BZ_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (unused == NULL || nUnused == NULL)
      { BZ_SETERR(BZ_PARAM_ERROR); return; };

   BZ_SETERR(BZ_OK);
   *nUnused = bzf->strm.avail_in;
   *unused = bzf->strm.next_in;
}
#endif


/*---------------------------------------------------*/
int BZ_API(BZ2_bzBuffToBuffDecompress) 
                           ( char*         dest, 
                             unsigned int* destLen,
                             char*         source, 
                             unsigned int  sourceLen,
                             int           small,
                             int           verbosity )
{
   bz_stream strm;
   int ret;

   if (dest == NULL || destLen == NULL || 
       source == NULL ||
       (small != 0 && small != 1) ||
       verbosity < 0 || verbosity > 4) 
          return BZ_PARAM_ERROR;

   strm.bzalloc = NULL;
   strm.bzfree = NULL;
   strm.opaque = NULL;
   ret = BZ2_bzDecompressInit ( &strm, verbosity, small );
   if (ret != BZ_OK) return ret;

   strm.next_in = source;
   strm.next_out = dest;
   strm.avail_in = sourceLen;
   strm.avail_out = *destLen;

   ret = BZ2_bzDecompress ( &strm );
   if (ret == BZ_OK) goto output_overflow_or_eof;
   if (ret != BZ_STREAM_END) goto errhandler;

   /* normal termination */
   *destLen -= strm.avail_out;
   BZ2_bzDecompressEnd ( &strm );
   return BZ_OK;

   output_overflow_or_eof:
   if (strm.avail_out > 0) {
       BZ2_bzDecompressEnd ( &strm );
       return BZ_UNEXPECTED_EOF;
   } else {
       BZ2_bzDecompressEnd ( &strm );
       return BZ_OUTBUFF_FULL;
   };      

   errhandler:
   BZ2_bzDecompressEnd ( &strm );
   return ret; 
}


/*---------------------------------------------------*/
/*--
   Code contributed by Yoshioka Tsuneo
   (QWF00133@niftyserve.or.jp/tsuneo-y@is.aist-nara.ac.jp),
   to support better zlib compatibility.
   This code is not _officially_ part of libbzip2 (yet);
   I haven't tested it, documented it, or considered the
   threading-safeness of it.
   If this code breaks, please contact both Yoshioka and me.
--*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
/*--
   return version like "0.9.0c".
--*/
const char * BZ_API(BZ2_bzlibVersion)(void)
{
   return BZ_VERSION;
}


#ifndef BZ_NO_STDIO
/*---------------------------------------------------*/

#if defined(_WIN32) || defined(OS2) || defined(MSDOS)
#   include <fcntl.h>
#   include <io.h>
#   define SET_BINARY_MODE(file) setmode(fileno(file),O_BINARY)
#else
#   define SET_BINARY_MODE(file)
#endif
static
BZFILE * bzopen_or_bzdopen
               ( const char *path,   /* no use when bzdopen */
                 int fd,             /* no use when bzdopen */
                 const char *mode,
                 int open_mode)      /* bzopen: 0, bzdopen:1 */
{
   int    bzerr;
   char   unused[BZ_MAX_UNUSED];
   int    blockSize100k = 9;
   int    writing       = 0;
   char   mode2[10]     = "";
   FILE   *fp           = NULL;
   BZFILE *bzfp         = NULL;
   int    verbosity     = 0;
   int    workFactor    = 30;
   int    smallMode     = 0;
   int    nUnused       = 0; 

   if (mode == NULL) return NULL;
   while (*mode) {
      switch (*mode) {
      case 'r':
         writing = 0; break;
      case 'w':
         writing = 1; break;
      case 's':
         smallMode = 1; break;
      default:
         if (isdigit((int)(*mode))) {
            blockSize100k = *mode-'0';
         }
      }
      mode++;
   }
   strcat(mode2, writing ? "w" : "r" );
   strcat(mode2,"b");   /* binary mode */

   if (open_mode==0) {
      if (path==NULL || strcmp(path,"")==0) {
        fp = (writing ? stdout : stdin);
        SET_BINARY_MODE(fp);
      } else {
        fp = fopen(path,mode2);
      }
   } else {
#ifdef BZ_STRICT_ANSI
      fp = NULL;
#else
      fp = fdopen(fd,mode2);
#endif
   }
   if (fp == NULL) return NULL;

   if (writing) {
      /* Guard against total chaos and anarchy -- JRS */
      if (blockSize100k < 1) blockSize100k = 1;
      if (blockSize100k > 9) blockSize100k = 9; 
      bzfp = BZ2_bzWriteOpen(&bzerr,fp,blockSize100k,
                             verbosity,workFactor);
   } else {
      bzfp = BZ2_bzReadOpen(&bzerr,fp,verbosity,smallMode,
                            unused,nUnused);
   }
   if (bzfp == NULL) {
      if (fp != stdin && fp != stdout) fclose(fp);
      return NULL;
   }
   return bzfp;
}


/*---------------------------------------------------*/
/*--
   open file for read or write.
      ex) bzopen("file","w9")
      case path="" or NULL => use stdin or stdout.
--*/
BZFILE * BZ_API(BZ2_bzopen)
               ( const char *path,
                 const char *mode )
{
   return bzopen_or_bzdopen(path,-1,mode,/*bzopen*/0);
}


/*---------------------------------------------------*/
BZFILE * BZ_API(BZ2_bzdopen)
               ( int fd,
                 const char *mode )
{
   return bzopen_or_bzdopen(NULL,fd,mode,/*bzdopen*/1);
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzread) (BZFILE* b, void* buf, int len )
{
   int bzerr, nread;
   if (((bzFile*)b)->lastErr == BZ_STREAM_END) return 0;
   nread = BZ2_bzRead(&bzerr,b,buf,len);
   if (bzerr == BZ_OK || bzerr == BZ_STREAM_END) {
      return nread;
   } else {
      return -1;
   }
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzwrite) (BZFILE* b, void* buf, int len )
{
   int bzerr;

   BZ2_bzWrite(&bzerr,b,buf,len);
   if(bzerr == BZ_OK){
      return len;
   }else{
      return -1;
   }
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzflush) (BZFILE *b)
{
   /* do nothing now... */
   return 0;
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzclose) (BZFILE* b)
{
   int bzerr;
   FILE *fp = ((bzFile *)b)->handle;
   
   if (b==NULL) {return;}
   if(((bzFile*)b)->writing){
      BZ2_bzWriteClose(&bzerr,b,0,NULL,NULL);
      if(bzerr != BZ_OK){
         BZ2_bzWriteClose(NULL,b,1,NULL,NULL);
      }
   }else{
      BZ2_bzReadClose(&bzerr,b);
   }
   if(fp!=stdin && fp!=stdout){
      fclose(fp);
   }
}


/*---------------------------------------------------*/
/*--
   return last error code 
--*/
static char *bzerrorstrings[] = {
       "OK"
      ,"SEQUENCE_ERROR"
      ,"PARAM_ERROR"
      ,"MEM_ERROR"
      ,"DATA_ERROR"
      ,"DATA_ERROR_MAGIC"
      ,"IO_ERROR"
      ,"UNEXPECTED_EOF"
      ,"OUTBUFF_FULL"
      ,"CONFIG_ERROR"
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
};


const char * BZ_API(BZ2_bzerror) (BZFILE *b, int *errnum)
{
   int err = ((bzFile *)b)->lastErr;

   if(err>0) err = 0;
   *errnum = err;
   return bzerrorstrings[err*-1];
}
#endif


/*-------------------------------------------------------------*/
/*--- end                                           bzlib.c ---*/
/*-------------------------------------------------------------*/
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
