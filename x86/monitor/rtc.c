/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: rtc.c,v 1.1.1.1 2003/06/10 22:42:22 iceblink Exp $
 */
#include <io.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>

#include "common/rom.h"
#include "common/rtc.h"

#include "rtc.h"

int read_rtc_func(int argc, char *argv[] __attribute__ ((unused)))
{
	unsigned int tmp;

	if (argc != 1)
        	return EBADNUMARGS;

	outb(RTC_YEAR_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	outb(RTC_MONTH_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	outb(RTC_DAY_OF_MONTH_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	outb(RTC_DAY_OF_WEEK_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xf;
	tmp = BCD2BIN(tmp);
	printf("%d", tmp);

	outb(RTC_HOUR_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	outb(RTC_MINUTE_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	outb(RTC_SECOND_REG, 0x70);
	tmp = inb(RTC_DATA_PORT) & 0xff;
	tmp = BCD2BIN(tmp);
	printf("%s%d", tmp<10?"0":"", tmp);

	printf("\n");

	return 0;
}

int write_rtc_func(int argc, char *argv[])
{
	long sec, minute, hour;
	long day_of_week, day_of_month;
	long month, year;
	char *csec, *cminute, *chour;
	char *cday_of_week, *cday_of_month;
	char *cmonth, *cyear;

	if (argc != 2)
		return EBADNUMARGS;

	if (strlen(argv[1]) != 13)
		return EBADARG;
	
	cyear = strdup(argv[1]);
	cyear[2] = '\0';

	cmonth = strdup(&argv[1][2]);
	cmonth[2] = '\0';

	cday_of_month = strdup(&argv[1][4]);
	cday_of_month[2] = '\0';

	cday_of_week = strdup(&argv[1][6]);
	cday_of_week[1] = '\0';

	chour = strdup(&argv[1][7]);
	chour[2] = '\0';

	cminute = strdup(&argv[1][9]);
	cminute[2] = '\0';

	csec = strdup(&argv[1][11]);
	csec[2] = '\0';

	if (stoli(cyear, &year)
	||  stoli(cmonth, &month)
	||  stoli(cday_of_month, &day_of_month)
	||  stoli(cday_of_week, &day_of_week)
	||  stoli(chour, &hour)
	||  stoli(cminute, &minute)
	||  stoli(csec, &sec)) {
		free(cyear);
		free(cmonth);
		free(cday_of_month);
		free(cday_of_week);
		free(chour);
		free(cminute);
		free(csec);

		return EBADNUM;
	}

	rtc_set_vals(BIN2BCD(sec), 0,
		     BIN2BCD(minute), 0,
		     BIN2BCD(hour), 0,
		     BIN2BCD(day_of_week),
		     BIN2BCD(day_of_month), 
		     BIN2BCD(month),
		     BIN2BCD(year));

	free(cyear);
	free(cmonth);
	free(cday_of_month);
	free(cday_of_week);
	free(chour);
	free(cminute);
	free(csec);

	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
