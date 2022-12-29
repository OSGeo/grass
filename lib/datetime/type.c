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
 * 
 * <ul>
 <li> This routine must be called can be made with other datetime functions.
 * </li>
 <li> initialize all the elements in dt.
 * </li>
 <li> Set all values to zero except:
 * tz (set to illegal value - 99*24)
 * positive (set to 1 for positive)
 * </li>
 <li> Set the type info in dt: mode, from, to, fracsec
 * </li>
 <li> validate the mode/from/to/fracsec (according to the rules for the mode)
 * </li>
 <li> return the return value from <tt>datetime_check_type</tt>(dt)
 </li></ul>
 *
 *  \param mode
 *  \param from
 *  \param to
 *  \param fracsec
 *  \return int
 */

int datetime_set_type(DateTime * dt, int mode, int from, int to, int fracsec)
{
    dt->mode = mode;
    dt->from = from;
    dt->to = to;
    dt->fracsec = fracsec;

    dt->year = 0;
    dt->month = 0;
    dt->day = 0;
    dt->hour = 0;
    dt->minute = 0;
    dt->second = 0.0;
    datetime_unset_timezone(dt);

    dt->positive = 1;

    return datetime_check_type(dt);
}

int
datetime_get_type(const DateTime * dt, int *mode, int *from, int *to,
		  int *fracsec)
{
    *mode = dt->mode;
    *to = dt->to;
    *from = dt->from;
    *fracsec = dt->fracsec;
    return datetime_check_type(dt);
}


/*!
 * \brief 
 *
 * Returns:  
 * 1 if <b>datetime_check_type()</b> returns 0  
 * 0 if not. 
 *
 *  \param dt
 *  \return int
 */

int datetime_is_valid_type(const DateTime * dt)
{
    /* Returns 0 if DateTime structure is not valid. */
    return datetime_check_type(dt) == 0;
}


/*!
 * \brief 
 *
 * checks the mode/from/to/fracsec in dt.
 * Returns: 
 * <ul>
 <li> 0: OK
 </li>
 <li> -1: mode is invalid - not one of {ABSOLUTE,RELATIVE} 
 </li>
 <li> -2: from is invalid - not one of {YEAR,MONTH,DAY,HOUR,MINUTE,SECOND}
 </li>
 <li> -3: to is invalid - not one of {YEAR,MONTH,DAY,HOUR,MINUTE,SECOND}
 </li>
 <li> -4: from/to are reversed (from>to is illegal)
 </li>
 <li> -5: invalid from/to combination for RELATIVE mode:  
 * from in {YEAR,MONTH} but to is not, or
 * from in {DAY,HOUR,MINUTE,SECOND} but to is not
 </li>
 <li> -6: from is invalid for ABSOLUTE mode (from != YEAR is illegal)
 </li>
 <li> -7: fracsec is negative (only if to==SECOND)
 </li></ul>
 *
 *  \param dt
 *  \return int
 */

int datetime_check_type(const DateTime * dt)
{
    /* Returns 0 for a valid DateTime structure.
       Sets the error code and error message if the structure is not
       valid.  Returns error code. */
    switch (dt->mode) {
    case DATETIME_ABSOLUTE:
    case DATETIME_RELATIVE:
	break;
    default:
	return datetime_error(-1, "invalid datetime 'mode'");
    }

    if (!datetime_is_between(dt->from, DATETIME_YEAR, DATETIME_SECOND))
	return datetime_error(-2, "invalid datetime 'from'");
    if (!datetime_is_between(dt->to, DATETIME_YEAR, DATETIME_SECOND))
	return datetime_error(-3, "invalid datetime 'to'");
    if (dt->from > dt->to)
	return datetime_error(-4, "invalid datetime 'from-to'");
    if (dt->mode == DATETIME_RELATIVE) {
	if (datetime_in_interval_year_month(dt->from)
	    && !datetime_in_interval_year_month(dt->to))
	    return datetime_error(-5, "invalid relative datetime 'from-to'");
	if (datetime_in_interval_day_second(dt->from)
	    && !datetime_in_interval_day_second(dt->to))
	    return datetime_error(-5, "invalid relative datetime 'from-to'");
    }
    if (dt->mode == DATETIME_ABSOLUTE && dt->from != DATETIME_YEAR)
	return datetime_error(-6, "invalid absolute datetime 'from'");
    if (dt->to == DATETIME_SECOND && dt->fracsec < 0)
	return datetime_error(-7, "invalid datetime 'fracsec'");

    return 0;
}

int datetime_in_interval_year_month(int x)
{
    return datetime_is_between(x, DATETIME_YEAR, DATETIME_MONTH);
}

int datetime_in_interval_day_second(int x)
{
    return datetime_is_between(x, DATETIME_DAY, DATETIME_SECOND);
}


/*!
 * \brief 
 *
 * Returns:  
 * 1 if dt.mode is absolute  
 * 0 if not (even if dt.mode is not defined) 
 *
 *  \param dt
 *  \return int
 */

int datetime_is_absolute(const DateTime * dt)
{
    return (dt->mode == DATETIME_ABSOLUTE);
}


/*!
 * \brief 
 *
 * Returns:
 * 1 if dt.mode is relative  
 * 0 if not (even if dt.mode is not defined) 
 *
 *  \param dt
 *  \return int
 */

int datetime_is_relative(const DateTime * dt)
{
    return (dt->mode == DATETIME_RELATIVE);
}
