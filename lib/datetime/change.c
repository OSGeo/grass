/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <grass/datetime.h>

static void make_incr();


/*!
 * \brief 
 *
 * Changes the from/to of the type for dt. 
 * The 'from/to' must be legal
 * values for the mode of dt; (if they are not legal, then the original values
 * are preserved, dt is not changed).  
 * Returns:  
 * 0 OK  
 * -1 invalid 'dt'  
 * -2 invalid 'from/to' <br>
 <ul>
 <li> round =   
 * negative implies floor() [decrease magnitude]
 * 0 implies normal rounding, [incr/decr magnitude]
 * positive implies ceil() [increase magnitude]
 </li>
 <li> If dt.from < 'from' (losing "lower" elements), convert the "lost"
 * values to the equivalent value for the new 'from' Lost elements are then set
 * to zero. (This case can only occur for dt.mode relative):
 * months += lost years * 12 ; years = 0
 * hours += lost days * 24 ; days = 0
 * minutes += lost hours * 60 ; hours = 0
 * seconds += lost minutes * 60.0 ; minutes = 0
 </li>
 <li> If dt.from > 'from' (adding "lower" elements), the new elements are set
 * to zero.
 </li>
 <li> If dt.to < 'to' (adding "higher" elements), the new elements are set to
 * zero.
 </li>
 <li> If dt.to > 'to' (losing "higher" elements), the the new 'to' is
 * adjusted according to the value for 'round' After rounding the "lost"
 * elements are set to zero.
 </li></ul>
 *
 *  \param dt
 *  \param from
 *  \param to
 *  \param round
 *  \return int
 */

int datetime_change_from_to(DateTime * dt, int from, int to, int round)
{
    DateTime dummy, incr;
    int pos;
    int carry;
    int ndays;
    int dtfrom;

    /* is 'dt' valid? */
    if (!datetime_is_valid_type(dt))
	return -1;

    /* is new from/to valid for dt->mode? */
    if (datetime_set_type(&dummy, dt->mode, from, to, 0) != 0)
	return -2;

    /* copy dt->from to local variable, then change it
       in the structure so that increment works correctly for RELATIVE.
       Otherwise, since increment "reduces" answers, performing carries, 
       we would carry to invalid units */

    dtfrom = dt->from;

    /* now set the from */
    dt->from = from;

    /* convert the "lost" lower elements to equiv value for the new 'from'
     * NOTE: this only affects DATETIME_RELATIVE 
     *       since absolute will have from==dt->from==YEAR
     */
    for (pos = dtfrom; pos < from; pos++) {
	switch (pos) {
	case DATETIME_YEAR:
	    dt->month += dt->year * 12;
	    dt->year = 0;
	    break;
	case DATETIME_DAY:
	    dt->hour += dt->day * 24;
	    dt->day = 0;
	    break;
	case DATETIME_HOUR:
	    dt->minute += dt->hour * 60;
	    dt->hour = 0;
	    break;
	case DATETIME_MINUTE:
	    dt->second += dt->minute * 60.0;
	    dt->minute = 0;
	    break;
	}
    }

    /* if losing precision, round
     *    round > 0 force up if any lost values not zero
     *    round ==0 increment by all lost values
     */
    if (to < dt->to) {
	if (round > 0) {
	    int x;

	    x = datetime_is_absolute(dt) ? 1 : 0;

	    for (carry = 0, pos = dt->to; carry == 0 && pos > to; pos--) {
		switch (pos) {
		case DATETIME_MONTH:
		    if (dt->month != x)
			carry = 1;
		    break;
		case DATETIME_DAY:
		    if (dt->day != x)
			carry = 1;
		    break;
		case DATETIME_HOUR:
		    if (dt->hour != 0)
			carry = 1;
		    break;
		case DATETIME_MINUTE:
		    if (dt->minute != 0)
			carry = 1;
		    break;
		case DATETIME_SECOND:
		    if (dt->second != 0)
			carry = 1;
		    break;
		}
	    }

	    if (carry) {
		make_incr(&incr, to, to, dt);

		incr.year = 1;
		incr.month = 1;
		incr.day = 1;
		incr.hour = 1;
		incr.minute = 1;
		incr.second = 1.0;

		datetime_increment(dt, &incr);
	    }
	}

	if (round == 0) {
	     /*NEW*/ if (datetime_is_absolute(dt))
		/*NEW*/ ndays = datetime_days_in_year(dt->year, dt->positive);
	     /*NEW*/
	    else
		/*NEW*/ ndays = 0;

	    for (pos = dt->to; pos > to; pos--) {
		make_incr(&incr, pos, pos, dt);

		incr.year = dt->year;
		incr.month = dt->month;
		 /*NEW*/ incr.day = dt->day + ndays / 2;
		incr.hour = dt->hour;
		incr.minute = dt->minute;
		incr.second = dt->second;

		datetime_increment(dt, &incr);
		 /*NEW*/ if (ndays > 0 && pos == DATETIME_DAY)
		    /*NEW*/ break;
	    }
	}
    }

    /* set the new elements to zero */
    for (pos = from; pos < dtfrom; pos++)
	switch (pos) {
	case DATETIME_YEAR:
	    dt->year = 0;
	    break;
	case DATETIME_MONTH:
	    dt->month = 0;
	    break;
	case DATETIME_DAY:
	    dt->day = 0;
	    break;
	case DATETIME_HOUR:
	    dt->hour = 0;
	    break;
	case DATETIME_MINUTE:
	    dt->minute = 0;
	    break;
	case DATETIME_SECOND:
	    dt->second = 0;
	    break;
	}

    for (pos = to; pos > dt->to; pos--)
	switch (pos) {
	case DATETIME_YEAR:
	    dt->year = 0;
	    break;
	case DATETIME_MONTH:
	    dt->month = 0;
	    break;
	case DATETIME_DAY:
	    dt->day = 0;
	    break;
	case DATETIME_HOUR:
	    dt->hour = 0;
	    break;
	case DATETIME_MINUTE:
	    dt->minute = 0;
	    break;
	case DATETIME_SECOND:
	    dt->second = 0;
	    break;
	}

    /* make sure that fracsec is zero if original didn't have seconds */
    if (dt->to < DATETIME_SECOND)
	dt->fracsec = 0;

    /* now set the to */
    dt->to = to;

    return 0;
}

static void make_incr(DateTime * incr, int from, int to, DateTime * dt)
{
    datetime_set_type(incr, DATETIME_RELATIVE, from, to, 0);
    if (datetime_is_relative(dt) && datetime_is_negative(dt))
	datetime_set_negative(incr);
}
