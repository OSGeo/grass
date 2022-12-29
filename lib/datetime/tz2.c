/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <grass/datetime.h>


/*!
 * \brief 
 *
 * if dt has a timezone, increment dt by minutes-dt.tz MINUTES and set dt.tz = minutes
 * Returns:  
 * 0 OK   
 * <b>datetime_check_timezone</b> (dt) if not  
 * -4 if minutes invalid  
 *
 *  \param dt
 *  \param minutes
 *  \return int
 */

int datetime_change_timezone(DateTime * dt, int minutes)
{				/* new timezone in minutes */
    int stat;
    int old_minutes, diff_minutes;
    DateTime incr;

    stat = datetime_get_timezone(dt, &old_minutes);
    if (stat != 0)
	return stat;
    if (!datetime_is_valid_timezone(minutes))
	return datetime_error(-4, "invalid datetime timezone");

    /* create a relative minute increment */
    datetime_set_type(&incr, DATETIME_RELATIVE, DATETIME_MINUTE,
		      DATETIME_MINUTE, 0);

    /* BB - needed to set SIGN here */
    diff_minutes = minutes - old_minutes;
    if (diff_minutes >= 0) {
	datetime_set_minute(&incr, diff_minutes);
    }
    else {
	datetime_invert_sign(&incr);
	datetime_set_minute(&incr, -diff_minutes);
    }

    return datetime_increment(dt, &incr);
}


/*!
 * \brief 
 *
 * Return <b>datetime_change_timezone</b> (dt, 0);
 *
 *  \param dt
 *  \return int
 */

int datetime_change_to_utc(DateTime * dt)
{
    return datetime_change_timezone(dt, 0);
}


/*!
 * \brief 
 *
 * tz = abs(tz)  
 * *hour = tz/60  
 * *minute = tz%60 
 * Note: hour,minute are non-negative. Must look at sign of tz itself to see if
 * the tz is negative offset or not. This routine would be used to format tz for
 * output. For example if tz=-350 this would be hour=5 minute=50, but negative.
 * Output might encode this as -0550: printf ("%s%02d%02d", tz<0?"-":"",
 * hour, minute)
 *
 *  \param tz
 *  \param hours
 *  \param minutes
 *  \return void
 */

void datetime_decompose_timezone(int tz, int *hours, int *minutes)
{
    if (tz < 0)
	tz = -tz;

    *hours = tz / 60;
    *minutes = tz % 60;
}
