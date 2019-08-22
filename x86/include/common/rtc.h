/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: rtc.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 */

#ifndef COMMON_RTC_H
#define COMMON_RTC_H

typedef struct
{
    int sec, alarm_sec;
    int minute, alarm_minute;
    int hour, alarm_hour;
    int day_of_week, day_of_month;
    int mounth, year;
} rtc_time_val;

#define RTC_PORT_BASE		0x70
#define RTC_ADDR_PORT		(RTC_PORT_BASE)
#define RTC_DATA_PORT		(RTC_PORT_BASE+0x01)
#define BCD2BIN(x)		(((x) & 0xf) + (10 * (((x)>>4) & 0xf)))
#define BIN2BCD(x)		((((x)/10)<<4) + ((x)%10))

#define PM_PORT_BASE            0x500
#define PM_ADDR_PORT            (PM_PORT_BASE)
#define PM_DATA_PORT            (PM_PORT_BASE + 1)

enum _rtc_reg_
{
    RTC_SECOND_REG = 0x0,
    RTC_ALARM_SECOND_REG = 0x1,
    RTC_MINUTE_REG = 0x2,
    RTC_ALARM_MINUTE_REG = 0x3,
    RTC_HOUR_REG = 0x4,
    RTC_ALARM_HOUR_REG = 0x5,
    RTC_DAY_OF_WEEK_REG = 0x6,
    RTC_DAY_OF_MONTH_REG = 0x7,
    RTC_MONTH_REG = 0x8,
    RTC_YEAR_REG = 0x9,
    RTC_STATUS_REG_A = 0x0a,
    RTC_STATUS_REG_B = 0x0b,
    RTC_STATUS_REG_C = 0x0c,
    RTC_STATUS_REG_D = 0x0d,
    RTC_BANK_0 = 0x20,
    RTC_BANK_1 = 0x30,
    RTC_BANK_2 = 0x40
};

#define RTC_BANK_MASK(a)        ((a) & 0x8f)

int rtc_init(void);
void rtc_begin_set(void);
void rtc_end_set(void);
char *rtc_get_datestr(void);
void rtc_set_vals(int, int, int, int, int, int, int, int, int, int);
void rtc_set_defaults(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
