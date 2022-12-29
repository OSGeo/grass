/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <stdlib.h>
#include <grass/datetime.h>
#include "math.h"

/*************************************************************/
/*
   This performs the formula:   result = a - b;

   both a and b must be absolute.
   result will be relative
   If a is "earlier" than b, then result should be set negative.

   b must be no more "precise" than a.
   (a copy of b is "extended" to the precision of a)

   datetime_copy (tb, b)
   datetime_reset_from_to (tb, b.from, a.to, a.fracsec))


   If result.to == SECOND, then result.fracsec is a.fracsec

   result will have the following from/to based on a.to:

   result
   a.to      from    to
   YEAR      YEAR    YEAR
   MONTH     YEAR    MONTH
   DAY       DAY     DAY
   HOUR      DAY     HOUR
   MINUTE    DAY     MINUTE
   SECOND    DAY     SECOND

   If either 'a' or 'b' has a timezone, both must have a timezone.
   The difference will account for the differences in the time zones.
 */

static int _datetime_ymd_to_ddays(const DateTime *, double *);
static int _datetime_compare(const DateTime *, const DateTime *);


/*!
 * \brief 
 *
 * 
 * This performs the formula:   result = a - b; 
 * <ul>
 <li> both a and b must be absolute.
 * </li>
 <li> result will be relative
 * </li>
 <li> If a is "earlier" than b, then result will be set negative.
 * </li>
 <li> b must be no more "precise" than a.
 * (a copy of b is "extended" to the precision of a)
 * </li>
 <li> If result.to == SECOND, then result.fracsec is a.fracsec
 * </li>
 <li> result will have the following from/to based
 * on a.to: result a.to      from    to YEAR      YEAR    YEAR MONTH     YEAR
 * MONTH DAY       DAY     DAY HOUR      DAY     HOUR MINUTE    DAY   
 * MINUTE SECOND    DAY     SECOND [LAYOUT ??? - see HTML]
 * </li>
 <li> If either 'a' or 'b' has a timezone, both must have a timezone. The
 * difference will account for the differences in the time zones.
 </li></ul>

 *
 *  \param a
 *  \param b
 *  \param result
 *  \return int
 */

int
datetime_difference(const DateTime * a, const DateTime * b, DateTime * result)
{
    DateTime tb, ta, *early, *late;
    int compare, tzmin;

    /* if not both absolute, return error */

    datetime_copy(&tb, b);
    datetime_change_from_to(&tb, DATETIME_YEAR, a->to, a->fracsec);

    datetime_copy(&ta, a);
    if (datetime_get_timezone(&ta, &tzmin) == 0 ||
	datetime_get_timezone(&tb, &tzmin) == 0) {
	if (datetime_get_timezone(&ta, &tzmin) == 0 &&
	    datetime_get_timezone(&tb, &tzmin) == 0) {
	    datetime_change_to_utc(&ta);
	    datetime_change_to_utc(&tb);
	}
	else
	    return datetime_error(-1,
				  "only one opperand contains valid timezone");
    }

    /* initialize result */
    datetime_set_type(result, DATETIME_RELATIVE,
		      ta.to < DATETIME_DAY ? DATETIME_YEAR : DATETIME_DAY,
		      ta.to, ta.fracsec);
    compare = _datetime_compare(&ta, &tb);
    if (compare > 0) {
	early = &tb;
	late = &ta;
	result->positive = 1;
    }
    else if (compare < 0) {
	early = &ta;
	late = &tb;
	result->positive = 0;
    }
    else {			/* equal */
	return (0);
    }

    /* now the work */
    if (datetime_in_interval_year_month(ta.to)) {
	int dm;

	if (ta.positive == tb.positive) {
	    /* change if we use doubles! */
	    result->year = abs(late->year - early->year);
	}
	else {
	    result->year = late->year + early->year - 2;
	}
	dm = late->month - early->month;
	if (dm >= 0)
	    result->month = dm;
	else {
	    result->year -= 1;
	    result->month = dm + 12;
	}
    }
    else {
	DateTime erel, lrel;
	double latedays, earlydays;

	datetime_set_increment_type(a, &erel);
	_datetime_ymd_to_ddays(early, &earlydays);
	/* copy day -> down */
	erel.day = earlydays;
	erel.hour = early->hour;
	erel.minute = early->minute;
	erel.second = early->second;

	datetime_set_increment_type(a, &lrel);
	_datetime_ymd_to_ddays(late, &latedays);
	/* copy day -> down */
	lrel.day = latedays;
	lrel.hour = late->hour;
	lrel.minute = late->minute;
	lrel.second = late->second;

	datetime_invert_sign(&erel);
	datetime_increment(&erel, &lrel);

	/* copy erel back to result */
	result->day = erel.day;
	result->hour = erel.hour;
	result->minute = erel.minute;
	result->second = erel.second;

	/* need carry? */
    }

    return (0);
}

/*************************************************************/
/* returns 1 if a is later than b, 
   -1 if a is earlier than a,
   0 otherwise 
 */
/* only looks at from-to fields defined by a */

static int _datetime_compare(const DateTime * a, const DateTime * b)
{
    int i;

    if (a->positive && !b->positive)
	return (1);
    else if (b->positive && !a->positive)
	return (-1);

    /* same signs */
    for (i = a->from; i <= a->to; i++) {
	switch (i) {

	case DATETIME_SECOND:
	    if (a->second > b->second)
		return (1);
	    else if (a->second < b->second)
		return (-1);
	    break;

	case DATETIME_MINUTE:
	    if (a->minute > b->minute)
		return (1);
	    else if (a->minute < b->minute)
		return (-1);
	    break;

	case DATETIME_HOUR:
	    if (a->hour > b->hour)
		return (1);
	    else if (a->hour < b->hour)
		return (-1);
	    break;

	case DATETIME_DAY:
	    if (a->day > b->day)
		return (1);
	    else if (a->day < b->day)
		return (-1);
	    break;

	case DATETIME_MONTH:
	    if (a->month > b->month)
		return (1);
	    else if (a->month < b->month)
		return (-1);
	    break;

	case DATETIME_YEAR:	/* only place sign matters */
	    if (a->positive) {
		if (a->year > b->year)
		    return (1);
		else if (a->year < b->year)
		    return (-1);
	    }
	    else {
		if (a->year < b->year)
		    return (1);
		else if (a->year > b->year)
		    return (-1);
	    }
	    break;
	}
    }
    return (0);


}

/*************************************************************/

static int _datetime_ymd_to_ddays(const DateTime * dtymd, double *days)
{				/* note extra precision! */
    int yr, mo;


    *days = 0.0;

    if (dtymd->positive) {
	*days = dtymd->day - 1;	/* start w/ days - 1 */
	for (mo = dtymd->month - 1; mo > 0; mo--) {	/* add earlier months */
	    *days += datetime_days_in_month(dtymd->year, mo, dtymd->positive);
	}
	for (yr = dtymd->year - 1; yr > 0; yr--) {	/* add earlier years */
	    *days += datetime_days_in_year(yr, dtymd->positive);
	}
    }
    else {
	for (yr = dtymd->year - 1; yr > 0; yr--) {	/* add later years */
	    *days += datetime_days_in_year(yr, dtymd->positive);
	}
	for (mo = 12; mo >= dtymd->month; mo--) {	/*add current & later months */
	    *days += datetime_days_in_month(dtymd->year, mo, dtymd->positive);
	}
	*days -= dtymd->day;	/* subtract current days */
    }

    return 0;
}

/*************************************************************/

/*************************************************************/
