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
 * This returns the components of a type
 * (mode/from/to/fracsec) that can be used to construct a DateTime object that
 * can be used to increment the 'src'. Also see
 * <b>datetime_set_increment_type()</b>.
 * returns:  
 * 0 dt is legal  
 * !=0 why dt is illegal 
 * Implemented as follows:       
 \code
 *mode    = RELATIVE
 *to      = src.to
 *fracsec = src.fracsec
 if src.mode is ABSOLUTE
 if src.to is in {YEAR,MONTH} then
 *from = YEAR
 if src.to is in {DAY,HOUR,MINUTE,SECOND} then 
 *from = DAY
 if src.mode is RELATIVE, then
 *from = src.from
 \endcode
 *
 *  \param dt
 *  \param mode
 *  \param from
 *  \param to
 *  \param fracsec
 *  \return int
 */

int
datetime_get_increment_type(const DateTime * dt, int *mode, int *from,
			    int *to, int *fracsec)
{
    if (!datetime_is_valid_type(dt))
	return datetime_error_code();

    *mode = DATETIME_RELATIVE;
    *to = dt->to;
    *fracsec = dt->fracsec;

    if (datetime_is_absolute(dt)) {
	if (datetime_in_interval_year_month(dt->to))
	    *from = DATETIME_YEAR;
	else
	    *from = DATETIME_DAY;
    }
    else {
	*from = dt->from;
    }
    return 0;
}


/*!
 * \brief 
 *
 * src must be legal 
 * This is a convenience routine which is implemented as follows:  
 \code
 int mode, from ,to;
 int fracsec;
 if(<b>datetime_get_increment_type</b>(src, &mode, &from, &to, &fracsec))
 return <b>datetime_get_error_code()</b>;
 return <b>datetime_set_type</b> (incr, mode, from, to, fracsec);
 \endcode
 * Timezone Timezones are represented in minutes from GMT in the range
 * [-720,+780]. For a DateTime to have a timezone, it must be of type ABSOLUTE,
 * and "to" must be in {MINUTE,SECOND}. 
 *
 *  \param src
 *  \param incr
 *  \return int
 */

int datetime_set_increment_type(const DateTime * src, DateTime * incr)
{
    int mode, from, to, fracsec;

    if (datetime_get_increment_type(src, &mode, &from, &to, &fracsec) != 0)
	return datetime_error_code();
    return datetime_set_type(incr, mode, from, to, fracsec);
}
