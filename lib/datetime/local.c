/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <time.h>
#include <grass/datetime.h>

extern struct tm *localtime();
extern struct tm *gmtime();

/* 
 ** NOTE:  the extern variable "timezone" seems to be treated
 ** differently by different OS, and the tm_zone element of struct tm
 ** is missing in some OS (IRIX), so we're converting localtime() and
 ** gmtime() structures to datetimes, then doing a difference to get the
 ** timezone offset.  -Bill Brown 5/31/95 
 */


/*!
 * \brief 
 *
 * Returns:
 * 0 OK  
 * -1 local timezone info not available 
 *
 *  \param minutes
 *  \return int
 */

int datetime_get_local_timezone(int *minutes)
{
    struct tm *local, *gm;
    time_t clock;
    DateTime dtl, dtg, dtdiff;

    time(&clock);

    local = localtime(&clock);

    datetime_set_type(&dtl, DATETIME_ABSOLUTE, DATETIME_YEAR, DATETIME_SECOND,
		      0);

    /* now put current {year,month,day,hour,minute,second} into local */
    datetime_set_year(&dtl, (int)local->tm_year + 1900);
    datetime_set_month(&dtl, (int)local->tm_mon + 1);
    datetime_set_day(&dtl, (int)local->tm_mday);
    datetime_set_hour(&dtl, (int)local->tm_hour);
    datetime_set_minute(&dtl, (int)local->tm_min);
    datetime_set_second(&dtl, (double)local->tm_sec);

    gm = gmtime(&clock);

    datetime_set_type(&dtg, DATETIME_ABSOLUTE, DATETIME_YEAR, DATETIME_SECOND,
		      0);

    /* now put current {year,month,day,hour,minute,second} into gmt */
    datetime_set_year(&dtg, (int)gm->tm_year + 1900);
    datetime_set_month(&dtg, (int)gm->tm_mon + 1);
    datetime_set_day(&dtg, (int)gm->tm_mday);
    datetime_set_hour(&dtg, (int)gm->tm_hour);
    datetime_set_minute(&dtg, (int)gm->tm_min);
    datetime_set_second(&dtg, (double)gm->tm_sec);

    datetime_set_type(&dtdiff, DATETIME_RELATIVE,
		      DATETIME_DAY, DATETIME_SECOND, 0);
    datetime_difference(&dtl, &dtg, &dtdiff);
    datetime_change_from_to(&dtdiff, DATETIME_MINUTE, DATETIME_MINUTE, 0);

    *minutes = dtdiff.positive ? dtdiff.minute : -dtdiff.minute;
    return 0;
}


/*!
 * \brief 
 *
 * set mode/from/to ABSOLUTE/YEAR/SECOND  
 * set the local time into 'dt'  does not set timezone.  
 *
 *  \param dt
 *  \return void
 */

void datetime_get_local_time(DateTime * dt)
{
    time_t clock;
    struct tm *local;

    /* first set dt to absolute full date */
    datetime_set_type(dt, DATETIME_ABSOLUTE, DATETIME_YEAR, DATETIME_SECOND,
		      0);

    /* get the current date/time */
    time(&clock);
    local = localtime(&clock);

    /* now put current {year,month,day,hour,minute,second} into dt */
    datetime_set_year(dt, (int)local->tm_year + 1900);
    datetime_set_month(dt, (int)local->tm_mon + 1);
    datetime_set_day(dt, (int)local->tm_mday);
    datetime_set_hour(dt, (int)local->tm_hour);
    datetime_set_minute(dt, (int)local->tm_min);
    datetime_set_second(dt, (double)local->tm_sec);
}
