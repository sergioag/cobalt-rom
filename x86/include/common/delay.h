/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: delay.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $
 *
 */

#ifndef COMMON_DELAY_H
#define COMMON_DELAY_H

extern unsigned long ticks_per_ms;
extern unsigned long ticks_per_us;
extern int udelay_min;
extern int udelay_umin;
#define UDELAY_UTABLE_LEN 128
extern int udelay_utable[UDELAY_UTABLE_LEN];

/* this is needed - see top of delay.c for why */
#define udelay(n)     \
	do { \
		if ((n) && (n) <= udelay_umin) { \
			; \
		} else if ((n) <= udelay_min) { \
			register int _i = udelay_utable[n]; \
			while (_i--) ; \
		} else { \
			_udelay(n-udelay_min); \
		} \
	} while(0)

/* 
 * delay_init
 *
 * calibrates udelay
 */
void delay_init(void);

/*
 * delay 
 * 
 * delay for mlliseconds
 */
void delay(int msecs);

/*
 * udelay 
 * 
 * delay for microseconds
 */
void _udelay(int usecs);

/*
 * delay_get_ticks
 * 
 * returns the number of ticks of the TSC
 */
unsigned long long delay_get_ticks(void);

/*
 * delay_ticks2usec
 *   tick - number of ticks
 *
 * convert ticks to microseconds and returns the number of
 * microseconds
 */unsigned long long delay_ticks2usec(unsigned long long ticks);

/*
 * delay_usec2ticks
 *   usec - number of microseconds
 *
 * convert microseconds to ticks and returns the number of ticks
 */
unsigned long long delay_usec2ticks(unsigned long long usec);

/* run timing trials (ifdef'ed) */
void delay_time_trials(void);

/*
 * udelay_and_clear
 *  usecs - number of microseconds to wait
 *
 * delays for usecs and clears memory in the process
 */
void udelay_and_clear( int usecs );

/*
 * delay_and_clear
 *  secs - number of milliseconds to wait
 *
 * delays for msecs and clears memory in the process
 */
static inline void delay_and_clear( int msecs )
{
    udelay_and_clear( msecs * 1000 );
}

/*
 * delay_clear_rest
 *  pbar - wether or not to display the progress bar
 *
 * finnish clearing memory
 */
void delay_clear_rest( int pbar );

/*
 * delay_clear_amount
 *
 * returns the number of bytes left to clear
 */
unsigned int delay_clear_amount( void );


#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
