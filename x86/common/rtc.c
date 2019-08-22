/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
#include <stdlib.h>
#include <io.h>
#include <string.h>

#include "common/systype.h"
#include "common/rom.h"
#include "common/rtc.h"
#include "common/cmos.h"


static int select_bank(const int base, const int bank)
{
  int val, oldbank;

  outb(0x0a, base);
  oldbank = val = inb(base + 1);
  val &= 0x8f;
  val |= bank;
  outb(val, base + 1);
  return oldbank & 0x50;
}

static int get_base(int dev, int start)
{
  int base;

  outb(start, dev);
  base = inb(dev + 1);
  outb(start + 1, dev);
  base |= inb(dev + 1) << 8;
  return base;
}

static int set_fer2(const int base, const int value)
{
  int val;

  outb(0x01, base);
  val = inb(base + 1);
  outb(value, base + 1);
  return val;
}


static inline void write_rtc(const int base, const int index, const int data)
{
  outb(index, base);
  outb(data, base + 1);
}


static inline int read_rtc(const int base, const int index)
{
  outb(index, base);
  return inb(base + 1);
}


static void rtc_reset(void)
{
  int val, base;

  val = select_bank(RTC_ADDR_PORT, 1);
  write_rtc(RTC_ADDR_PORT, 0x48, 0x0); /* day of month alarm=0*/
  write_rtc(RTC_ADDR_PORT, 0x49, 0x0); /* month alarm=0 */
  write_rtc(RTC_ADDR_PORT, 0x4a, 0x0); /* centry = 0 */

  select_bank(RTC_ADDR_PORT, 2);
  write_rtc(RTC_ADDR_PORT, 0x40, 0x00); /* turn off MOAP bit */
  write_rtc(RTC_ADDR_PORT, 0x41, 0x0);
  read_rtc(RTC_ADDR_PORT, 0x42);
  write_rtc(RTC_ADDR_PORT, 0x43, 0xc0); /* Wake Day Of Week = X */
  write_rtc(RTC_ADDR_PORT, 0x44, 0xc0); /* Wake date = X */
  write_rtc(RTC_ADDR_PORT, 0x45, 0xc0); /* Wake month = X */
  write_rtc(RTC_ADDR_PORT, 0x46, 0xc0); /* Wake year = X */
  write_rtc(RTC_ADDR_PORT, 0x47, 0xc0); /* RAMLOCK = no lock */
  write_rtc(RTC_ADDR_PORT, 0x48, 0xc0); /* Wake centry = X */
  write_rtc(RTC_ADDR_PORT, 0x49, 0xa0); /* GPIO10 = falling edge */
  write_rtc(RTC_ADDR_PORT, 0x4a, 0x2d); /* PME1 and PME2 = falling edge */
  write_rtc(RTC_ADDR_PORT, 0x4b, 0x2d); /* IIRX1 and IIRX2 = falling edge */
  write_rtc(RTC_ADDR_PORT, 0x4c, 0xad); /* GPIO12 and GPIO13 = falling edge
					 * Mask ONCTL
					 */
  write_rtc(RTC_ADDR_PORT, 0x4d, 0x15); /* P12 = falling edge
					 * Power Supply Portect Mode = off
					 */
  write_rtc(RTC_ADDR_PORT, 0x4f, 0xc9); /* Day of month alarm address = bank 1 0x49 */
  write_rtc(RTC_ADDR_PORT, 0x50, 0xca); /* Month alarm address = bank1 0x4a */
  write_rtc(RTC_ADDR_PORT, 0x51, 0xc8); /* Centry Address = bank1 0x48 */
  select_bank(RTC_ADDR_PORT, val);

  val = set_fer2(PM_ADDR_PORT, 0x78);

  /* pm 1 evt */
  if ((base = get_base(PM_ADDR_PORT, 0x08))) {
    outb(0xff, base);
    outb(0xff, base + 0x01);
    outb(0xff, base + 0x02);
    outb(0x00, base + 0x03);
  }

  /* pm 1 ctl */
  if ((base = get_base(PM_ADDR_PORT, 0x0c))) {
    outb(0x0, base);
    outb(0x0, base + 0x01);
  }

  /* gp1 */
  if ((base = get_base(PM_ADDR_PORT, 0x0e))) {
    outb(0xff, base);
    outb(0x0, base + 0x04);
    outb(0x0, base + 0x08);
  }

  set_fer2(PM_ADDR_PORT, val);
}


int rtc_init( void )
{
	rtc_begin_set();
	if (first_boot) {
	    /* fix up nat semi superio buglet that prevents power-off
	       from working correctly */
	    if (boardtype() == BOARDTYPE_MONTEREY)
		rtc_reset(); 

	    /* set the initial date/time */
	    rtc_set_vals(0, 0, BIN2BCD(59), 0, BIN2BCD(10), 0, BIN2BCD(3), 
		 BIN2BCD(2), BIN2BCD(3), BIN2BCD(76));
	    
	    /* 32.768 KHz, 976 usec periodic interrupt */
	    outb(RTC_STATUS_REG_A, RTC_ADDR_PORT);
	    outb(0x26, RTC_DATA_PORT);
	    
	    /* 24 hour mode, BCD, no DST */
	    outb(RTC_STATUS_REG_B, RTC_ADDR_PORT);
	    outb(0x02, RTC_DATA_PORT);
	    
	    /* nothing to set on register 0xc */
	    
	    /* valid Ram/Time, no alarm */
	    outb(RTC_STATUS_REG_D, RTC_ADDR_PORT);
	    outb(0x26, RTC_DATA_PORT);
	} else {
	    unsigned char temp;
	    
	    outb(RTC_STATUS_REG_A, RTC_ADDR_PORT);
	    temp = inb(RTC_DATA_PORT);
	    /* set to 32.768 KHz, 976 usec periodic int, oscillator enabled */
	    outb((0x26|temp) & ~0x50, RTC_DATA_PORT);
	    
	    outb(RTC_STATUS_REG_B, RTC_ADDR_PORT); 
	    temp = inb( RTC_DATA_PORT);

	    /* turn on BCD format (clear bit 2)
	     * disable Alarm INTR (clear bit 5) */
	    temp &= ~0x24;
	    outb(temp, RTC_DATA_PORT);
	}
	rtc_end_set();

    	return 0;
}

void
rtc_set_defaults(void)
{
    rtc_begin_set();

    /* set the initial date/time */
    rtc_set_vals(0, 0, BIN2BCD(59), 0, BIN2BCD(10), 0, BIN2BCD(3), 
	BIN2BCD(2), BIN2BCD(3), BIN2BCD(76));

    /* 32.768 KHz, 976 usec periodic interrupt */
    outb(RTC_STATUS_REG_A, RTC_ADDR_PORT);
    outb(0x26, RTC_DATA_PORT);

    /* 24 hour mode, BCD, no DST */
    outb(RTC_STATUS_REG_B, RTC_ADDR_PORT);
    outb(0x02, RTC_DATA_PORT);

    /* nothing to set on register 0xc */

    /* valid Ram/Time, no alarm */
    outb(RTC_STATUS_REG_D, RTC_ADDR_PORT);
    outb(0x26, RTC_DATA_PORT);
    
    rtc_end_set();
}

void 
rtc_set_vals(int sec, int alarm_sec, int minute, int alarm_minute,
	int hour, int alarm_hour, int day_of_week, int day_of_month,
	int month, int year)
{
    rtc_begin_set();

    outb(RTC_SECOND_REG, RTC_ADDR_PORT);
    outb(sec & 0xff, RTC_DATA_PORT);

    outb(RTC_ALARM_SECOND_REG, RTC_ADDR_PORT);
    outb(alarm_sec & 0xff, RTC_DATA_PORT);

    outb(RTC_MINUTE_REG, RTC_ADDR_PORT);
    outb(minute & 0xff, RTC_DATA_PORT);

    outb(RTC_ALARM_MINUTE_REG, RTC_ADDR_PORT);
    outb(alarm_minute & 0xff, RTC_DATA_PORT);

    outb(RTC_HOUR_REG, RTC_ADDR_PORT);
    outb(hour & 0xff, RTC_DATA_PORT);

    outb(RTC_ALARM_HOUR_REG, RTC_ADDR_PORT);
    outb(alarm_hour & 0xff, RTC_DATA_PORT);

    outb(RTC_DAY_OF_WEEK_REG, RTC_ADDR_PORT);
    outb(day_of_week & 0xff, RTC_DATA_PORT);

    outb(RTC_DAY_OF_MONTH_REG, RTC_ADDR_PORT);
    outb(day_of_month & 0xff, RTC_DATA_PORT);

    outb(RTC_MONTH_REG, RTC_ADDR_PORT);
    outb(month & 0xff, RTC_DATA_PORT);

    outb(RTC_YEAR_REG, RTC_ADDR_PORT);
    outb(year & 0xff, RTC_DATA_PORT);

    rtc_end_set();
}

void 
rtc_begin_set(void)
{
	int tmp;

	outb(RTC_STATUS_REG_B, RTC_ADDR_PORT);
	tmp = inb(RTC_DATA_PORT);
	outb(tmp | 0x80, RTC_DATA_PORT);
}

void 
rtc_end_set(void)
{
	int tmp;

	outb(RTC_STATUS_REG_B, RTC_ADDR_PORT);
	tmp = inb(RTC_DATA_PORT);
	outb(tmp & ~0x80, RTC_DATA_PORT);
}

char *
rtc_get_datestr(void)
{
	unsigned int tmp;
	static char buf[128];
	char *p;
	char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
				"Aug", "Sep", "Oct", "Nov", "Dec", "Bad"};

	p = buf;

	outb(RTC_MONTH_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	if (tmp > 12) {
		tmp = 13;
	}
	sprintf(p, "%s ", months[tmp-1]);
	p += 4;

	outb(RTC_DAY_OF_MONTH_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	sprintf(p, "%s%d ", tmp<10?"0":"", tmp);
	p += 3;

	outb(RTC_HOUR_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	sprintf(p, "%s%d:", tmp<10?"0":"", tmp);
	p += 3;

	outb(RTC_MINUTE_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	sprintf(p, "%s%d:", tmp<10?"0":"", tmp);
	p += 3;

	outb(RTC_SECOND_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	sprintf(p, "%s%d UTC ", tmp<10?"0":"", tmp);
	p += 7;

	outb(RTC_YEAR_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	tmp += (tmp < 70 ? 2000 : 1900);
	sprintf(p, "%d", tmp);
	p += 4;

	*p = '\0';
	return buf;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
