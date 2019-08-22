/* $Id: ctype.c,v 1.1.1.1 2003/06/10 22:41:40 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <ctype.h>

char __ctype[1 + 256] = {
        0,
/*       0       1       2       3       4       5       6       7  */
/* 0 */ _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
/* 0 */ _C,     _C|_S|_B,_C|_S,  _C|_S,  _C|_S,  _C|_S,  _C,     _C,
/* 1 */ _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
/* 1 */ _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
/* 2 */ _S|_B,  _P,     _P,     _P,     _P,     _P,     _P,     _P,
/* 2 */ _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P,
/* 3 */ _N,     _N,     _N,     _N,     _N,     _N,     _N,     _N,
/* 3 */ _N,     _N,     _P,     _P,     _P,     _P,     _P,     _P,
/* 4 */ _P,     _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U,
/* 4 */ _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
/* 5 */ _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
/* 5 */ _U,     _U,     _U,     _P,     _P,     _P,     _P,     _P,
/* 6 */ _P,     _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L,
/* 6 */ _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
/* 7 */ _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
/* 7 */ _L,     _L,     _L,     _P,     _P,     _P,     _P,     _C
};
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
