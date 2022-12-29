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
 * Returns:
 * <b>datetime_check_increment</b>(src, incr) == 0 
 *
 *  \param src
 *  \param incr
 *  \return int
 */

int datetime_is_valid_increment(const DateTime * src, const DateTime * incr)
{
    return datetime_check_increment(src, incr) == 0;
}



/*!
 * \brief 
 *
 * This checks if the type of 'incr' is valid for incrementing/decrementing 'src'.  
 * The type (mode/from/to) of the 'src' can be anything.  
 * The incr.mode must be RELATIVE  
 * A timezone in 'src' is allowed - it's presence is ignored.  
 * To aid in setting the 'incr' type,  see  <b>datetime_get_increment_type()</b>. 
 * Returns:
 * <ul>
 <li> 0 valid increment 
 </li>
 <li> 1 src is not a legal DateTime, error code/msg are those set by
 * <b>datetime_is_valid_type()</b>
 </li>
 <li> 2 incr is not a legal DateTime, error code/msg are those set by
 * <b>datetime_is_valid_type()</b>
 </li>
 <li> -1 incr.mode not relative 
 </li>
 <li> -2 incr more precise that src 
 </li>
 <li> -3 illegal incr, must be YEAR-MONTH 
 </li>
 <li> -4 illegal incr, must be DAY-SECOND
 </li></ul>
 *
 *  \param src
 *  \param incr
 *  \return int
 */

int datetime_check_increment(const DateTime * src, const DateTime * incr)
{
    if (!datetime_is_valid_type(src))
	return 1;
    if (!datetime_is_valid_type(incr))
	return 2;

    if (!datetime_is_relative(incr))
	return datetime_error(-1, "datetime increment mode not relative");
    if (incr->to > src->to)
	return datetime_error(-2, "datetime increment too precise");

    if (datetime_in_interval_year_month(src->to) &&
	!datetime_in_interval_year_month(incr->to))
	return datetime_error(-3, "illegal datetime increment interval");

    if (datetime_in_interval_day_second(src->to) &&
	!datetime_in_interval_day_second(incr->to))
	return datetime_error(-4, "illegal datetime increment interval");

    return 0;
}
