/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * delay.c
 *
 * There are some seriously wacky things going on in here.  In order to get
 * microsecond resolutions even CLOSE, we have to be aware of one major fact -
 * there generally is no cache while we're in ROM! 
 *
 * This impacts EVERYTHING.  More so than I expected.  The cost of entering
 * _udelay for 1 usec on a 600 MHz CPU is ~5-6 usecs.  That is why we try to
 * determine a minimum resolution under which to enter the actual _udelay
 * function.  Be VERY aware when changing ANYTHING in the udelay path.
 * 
 * Lastly, we need to re-initialize delays if we diddle with caching AT ALL.
 * FIXME: L1 seems to be OK, L2 blows up with all this...
 * 
 * TPH - 11/21/00
 */
#include <io.h>
#include "common/delay.h"
#include "common/x86.h"
#include "common/rom.h"

/* control whether or not time trials are run */
#undef TIME_TRIALS

static void pit_delay(int msecs);

/* all of these variables are implicitly initialized to zero in .bss */
unsigned long ticks_per_us;
unsigned long ticks_per_ms;
int udelay_min;
int udelay_umin;
int udelay_utable[UDELAY_UTABLE_LEN];

static long delay_adjust;
static long udelay_adjust;
static unsigned long udelay_uadjust;
static long ticks_per_loop;
static unsigned long loops_per_us;
static unsigned long msus_diff;

/* 
 * small helper funcs 
 */
static inline unsigned char
get_pit_status(int n)
{
	outb(0xe0 | 1<<(n+1), 0x43);
	return inb(0x41);
}

/* be careful of overflowing nticks */
static inline void
waste_ticks(unsigned long nticks)
{
	unsigned long t0, t1;

	__asm__ __volatile__("jmp 1f; .align 16; 1:");

	if (nticks == 0) {
		return;
	}

	t0 = rdtscl();

	do {
		t1 = rdtscl();
	} while (t1 - t0 < nticks);
}

/* 
 * read PIT counter #1 (DMA refresh)
 * 
 * we know the PIT is running at a rate of 1193180 Hz (1.19318 MHz)
 * we know that PIT counter #1 is set to 128 counts per cycle (.000107 sec)
 * 
 * 1193 ticks is 1 millisecond
 * 1280 ticks/msec is what we can approximate
 * 1193/1280 = .9320
 * we want to spin 9.32 times as many cycles as msecs
 * 
 * approximately 1 millisecond granularity
 * accurate to about 7%, best with msecs >= 100
 */
static void 
pit_delay(int msecs)
{
	/* adjust for inaccuracy */
	msecs = (msecs * 932)/100;

        /* wait for the current tick edge */
        while (1) {
                if (get_pit_status(1) & 0x80) {
			/* OUT pin went high */
                        break;
                }
        }

	/* we've detected a tick just starting */
	while (msecs) {
		/* wait for high bit to go off */
		while (get_pit_status(1) & 0x80) 
			;
		
		/* now wait for it to come back */
		while (!(get_pit_status(1) & 0x80)) 
			;
		
		/* ...and the rising and the falling were one tick */
		msecs--;
	}
}

/*
 * coarse-grained delay
 *
 * This will delay with ~millisecond granularity
 */
void
delay(int msecs)
{
	if (msecs <= 1000) {
		waste_ticks((msecs * ticks_per_ms) + delay_adjust);
	} else {
		pit_delay(msecs);
	}
}

/* 
 * fine-grained delay
 *
 * This will delay with ~microsecond granularity
 *
 * Because the adjustment factor is constant, not scaled, the larger 
 * the value of usecs you request, the more the error grows
 *
 * From time to time we get strange results for one call, then it seems to 
 * go away.
 */
void 
_udelay(int usecs)
{
	register int n;
	while (usecs > 100000) {
		delay(100);
		usecs -= 100000;
	}
	n = (usecs * ticks_per_us) + udelay_adjust;
	n += (usecs * msus_diff)/1000;
	waste_ticks(n);
}

/*
 * calibrate the delay mechanism
 *
 * delay, based on the PIT, for a short amount of time, and 
 * figure how many TSCs have elapsed.  Then the fun begins.
 * 
 * This routine does it's best to auto-tune for maximum accuracy.
 *
 * See top-of-file comment.
 */
#define NCALIBRATIONS	16
void 
delay_init(void) 
{
	unsigned long long time0, time1;
	unsigned long t0, t1;
	long ttl;
	int i;
	
	/* zero counters here, to be able to re-run this! */
	ticks_per_us = 0;
	ticks_per_ms = 0;
	udelay_min = 0;
	udelay_umin = 0;
	delay_adjust = 0;
	udelay_adjust = 0;
	udelay_uadjust = 0;
	ticks_per_loop = 0;
	loops_per_us = 0;
	for (i = 0; i < UDELAY_UTABLE_LEN; i++) {
		udelay_utable[i] = 0;
	}

	/* keep the delay and denominator below in sync */
	time0 = rdtscll();
	/* pit_delay() is most accurate >= 100 msecs */
	pit_delay(100);
        cpu_serialize(); /* force in-order execution */
	time1 = rdtscll();

	ticks_per_us = (unsigned long)((time1 - time0) / 100000);
	ticks_per_ms = (unsigned long)((time1 - time0) / 100);
	msus_diff = ticks_per_ms - (ticks_per_us * 1000);

	/* 
	 * now that we have an approximation of our CPU speed, lets
	 * see how accurate we can get 
	 */

	/* find an average adjustment for delay() */
	ttl = 0;
	for (i = 0; i < NCALIBRATIONS; i++) {
		unsigned long expect, got;

		t0 = rdtscl();
		delay(i*2);
		t1 = rdtscl();

		expect = (i*2) * ticks_per_ms;
		got = t1-t0;

		ttl += (long)(expect - got);
	}
	delay_adjust = ttl/NCALIBRATIONS;

	/* find a minimum parameter for udelay() (note: udelay_min == 0) */
	ttl = 0;
	for (i = 0; i < NCALIBRATIONS; i++) {
		unsigned long got;
		
		t0 = rdtscl();
		udelay(1);
		t1 = rdtscl();

		got = t1 - t0;

		ttl += got/ticks_per_us;
		if ((got % ticks_per_us) >= (ticks_per_us/5)) {
			/* better to be high, than low here */
			ttl++;
		}
	}
	udelay_min = ttl/NCALIBRATIONS;

	/* find an average adjustment for smallish udelay()s */
	ttl = 0;
	for (i = 0; i < NCALIBRATIONS; i++) {
		unsigned long expect, got;
		int n = (udelay_min+1) + (i*2);

		t0 = rdtscl();
		udelay(n);
		t1 = rdtscl();

		expect = n * ticks_per_us;
		expect += (n*msus_diff)/1000;
		got = t1-t0;

		ttl += (long)(expect - got);
	}
	udelay_adjust = ttl/NCALIBRATIONS;

	/* calibrate for the very low end of udelay() */
	ttl = 0;
	for (i = 0; i < NCALIBRATIONS; i++) {
		int j = 4096;	
		t0 = rdtscl();
		while (j--) ;
		t1 = rdtscl();

		ttl += (t1-t0)/4096;
	}
	ticks_per_loop = ttl/NCALIBRATIONS;
	loops_per_us = ticks_per_us / ticks_per_loop;

	/* find the overhead of the udelay() macro */
	ttl = 0;
	for (i = 0; i < NCALIBRATIONS; i++) {
		t0 = rdtscl();
		udelay(0);
		t1 = rdtscl();

		ttl += (t1-t0)/ticks_per_loop;
	}
	udelay_uadjust = ttl/NCALIBRATIONS;

	/* fill a small table for very small udelay values */
	for (i = 0; i < UDELAY_UTABLE_LEN; i++) {
		int j = (i*loops_per_us) - udelay_uadjust;

		if (j > 0) {
			udelay_utable[i] = j;
		}
	}

	/* 
	 * Now, with all that in place, we should be pretty accurate.
	 * Let's see how we did, and fine tune just a hair more, if need be 
	 */

	/* see how we are on the VERY low end, for udelay()s <= udelay_min */
	for (i = 1; i <= udelay_min; i++) {
		int j;
		int avg0, avg1, avg2;
		int orig = udelay_utable[i];
		
		/* 
		 * Try the current value in the table, as well as one higher
		 * and one lower, use the most accurate one.  This assumes the
		 * above calibrations did their job to +/- 1 loop, which seems
		 * to be a safe bet.
		 */
		ttl = 0;
		for (j = 0; j < NCALIBRATIONS; j++) {
			unsigned long expect, got;

			t0 = rdtscl();
			udelay(i);
			t1 = rdtscl();
		
			expect = i * ticks_per_us;
			got = t1-t0;

			ttl += (long)(expect - got);
		}
		avg0 = ttl/NCALIBRATIONS;
		if (avg0 < 0) {
			avg0 *= -1;
		}

		/* try one more... */
		udelay_utable[i] = orig + 1;
		ttl = 0;
		for (j = 0; j < NCALIBRATIONS; j++) {
			unsigned long expect, got;

			t0 = rdtscl();
			udelay(i);
			t1 = rdtscl();
		
			expect = i * ticks_per_us;
			got = t1-t0;

			ttl += (long)(expect - got);
		}
		avg1 = ttl/NCALIBRATIONS;
		if (avg1 < 0) {
			avg1 *= -1;
		}

		/* careful not to wrap into negative land */
		if (orig > 0) {
			/* try one less... */
			udelay_utable[i] = orig - 1;
			ttl = 0;
			for (j = 0; j < NCALIBRATIONS; j++) {
				unsigned long expect, got;

				t0 = rdtscl();
				udelay(i);
				t1 = rdtscl();
		
				expect = i * ticks_per_us;
				got = t1-t0;

				ttl += (long)(expect - got);
			}
			avg2 = ttl/NCALIBRATIONS;
			if (avg2 < 0) {
				avg2 *= -1;
			}
		} else {
			/* set it very big */
			avg2 = INT_MAX;
		}

		/* which is best? */
		if (avg0 <= avg1 && avg0 <= avg2) {
			udelay_utable[i] = orig;
		} else if (avg1 <= avg0 && avg1 <= avg2) {
			udelay_utable[i] = orig + 1;
		} else {
			udelay_utable[i] = orig - 1;
		}
	}

	/* find udelay_umin */
	i = 0;
	while (udelay_utable[i] == 0 && i <= udelay_min) {
		i++;
	}
	udelay_umin = i-1;
}

unsigned long long 
delay_get_ticks(void)
{
	return rdtscll();
}

unsigned long long 
delay_ticks2usec(unsigned long long ticks)
{
	return (unsigned long long)(ticks / ticks_per_us);
}

unsigned long long 
delay_usec2ticks(unsigned long long usec)
{
	return (unsigned long long)(usec * ticks_per_us);
}


void
delay_time_trials(void)
{
#ifdef TIME_TRIALS
#define ABS(x)	((x) < 0 ? (0-(x)) : (x))
	int i;
	int n;
	unsigned long ttl;
	int nttl;

	printf("\n\n");
	printf("delay testing:\n\r");
	printf("     ticks per us: %d\n\r", (unsigned int)ticks_per_us);
	printf("     ticks per ms: %d\n\r", (unsigned int)ticks_per_ms);
	printf("     udelay min: %d\n\r", udelay_min);

	/* test delay() */
	ttl = nttl = 0;
	for (n = 1; n <= 1000; n*=10) {
		for (i = 0; i < 3; i++) {
		    unsigned long t0, t1;
		    int expect, got;

		    t0 = rdtscl();
		    delay(n);
		    t1 = rdtscl();

		    expect = n*ticks_per_ms;
		    got = t1-t0;
		    printf("%d: delay(%d): %d ticks, %d expected, %d off\n\r",
		        i, n, got, expect, got-expect);
		    ttl += (unsigned long)ABS(got-expect);
		    nttl++;
		}
	}
	printf("avg off +/- %d\n\n", (int)(ttl/nttl));

	/* test udelay() for a broad range */
	ttl = nttl = 0;
	for (n = 1; n <= 1000000; n*=10) {
		for (i = 0; i < 2; i++) {
		    unsigned long t0, t1;
		    int expect, got;

		    t0 = rdtscl();
		    udelay(n);
		    t1 = rdtscl();

		    expect = n*ticks_per_us;
		    expect += (n*msus_diff)/1000;
		    got = t1-t0;
		    printf("%d: udelay(%d): %d ticks, %d expected, %d off\n\r",
		        i, n, got, expect, got-expect);
		    ttl += (unsigned long)ABS(got-expect);
		    nttl++;
		}
	}
	printf("avg off +/- %d\n\n", (int)(ttl/nttl));
	
	/* test udelay() for the low-end */
	ttl = nttl = 0;
	for (n = 1; n <= 300; n++) {
		for (i = 0; i < 2; i++) {
		    unsigned long t0, t1;
		    int expect, got;
		    
		    t0 = rdtscl();
		    udelay(n);
		    t1 = rdtscl();

		    expect = n*ticks_per_us;
		    expect += (n*msus_diff)/1000;
		    got = t1-t0;
		    printf("%d: udelay(%d): %d ticks, %d expected, %d off\n\r",
		        i, n, got, expect, got-expect);
		    ttl += (unsigned long)ABS(got-expect);
		    nttl++;
		}
	}
	printf("avg off +/- %d\n\n", (int)(ttl/nttl));
#endif
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
