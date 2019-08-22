/*
 * From timbOS
 * $Id: io.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $
 */ 

#ifndef IO_H__
#define IO_H__

#include <extra_types.h>

/* an out? insn */
#define _ASM_OUT(len)							\
	__asm__ __volatile__("out" #len " %0, %w1"			\
		: : "a" (val), "Nd" (port))

/* an in? insn */
#define _ASM_IN(len)							\
	__asm__ __volatile__("in" #len " %w1, %0"			\
		: "=a" (val) : "Nd" (port))

/* a slight delay */
#define _DELAY1		"jmp .+2 \n"
#define _ASM_DELAY							\
	__asm__ __volatile__(_DELAY1 _DELAY1 _DELAY1 _DELAY1)		\

/* an out? function */
#define _IO_OUT(len, type)						\
	static inline void out ##len ( type val, uint16 port) 		\
	{ 								\
		_ASM_OUT(len);						\
	}								\
	static inline void out ##len ##_p( type val, uint16 port)	\
	{ 								\
		_ASM_OUT(len);						\
		_ASM_DELAY;						\
	}

/* an in? function */
#define _IO_IN(len, type)						\
	static inline type in ##len (uint16 port)			\
	{								\
		type val;						\
		_ASM_IN(len);						\
		return val;						\
	}								\
	static inline type in ##len ##_p(uint16 port)			\
	{ 								\
		type val;						\
		_ASM_IN(len);						\
		_ASM_DELAY;						\
		return val;						\
	}
		
/* all the functions for a given datasize */
#define _IO_DECL(len, type)		_IO_OUT(len, type)  		\
					_IO_IN(len, type)

/* our IO functions - all that work for this */
_IO_DECL(b, uint8)
_IO_DECL(w, uint16)
_IO_DECL(l, uint32)

#undef _ASM_OUT
#undef _ASM_IN
#undef _DELAY1
#undef _ASM_DELAY
#undef _IO_OUT
#undef _IO_IN
#undef _IO_DECL

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
