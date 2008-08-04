/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <grass/datetime.h>

static int _datetime_add_field(DateTime *, DateTime *, int);
static int _datetime_subtract_field(DateTime *, DateTime *, int);

/*****************************************************************/
#if 0				/* unused */
static double _debug_decimal(DateTime * dt)
{
    double dtdec = 0.0;

    if (dt->mode == DATETIME_RELATIVE) {
	if (datetime_in_interval_year_month(dt->from)) {
	    dtdec = dt->year + dt->month / 12.;
	}
	else {
	    dtdec = dt->day / 365.25 +
		dt->hour / 8766. + dt->minute / 525960.
		+ dt->second / 31557600.;
	}
    }
    if (dt->positive)
	return (dtdec);
    return (-dtdec);
}
#endif /* unused */

/*****************************************************************/

/*!
 * \brief 
 *
 * This function changes the 'src' date/time data based on the 'incr'  
 * The type (mode/from/to) of the 'src' can be anything.  
 * The mode of the 'incr' must be RELATIVE, and the type (mode/from/to) for
 * 'incr' must be a valid increment for 'src'. See  <b>datetime_is_valid_increment()</b>,
 * <b>datetime_check_increment()</b>
 * Returns:  
 * 0: OK  
 * -1: 'incr' is invalid increment for 'src' 
 * For src.mode ABSOLUTE, 
 * <ul>
 <li> positive 'incr' moves into the future,
 </li>
 <li> negative 'incr' moves into the past.
 </li>
 <li> BC implies the year is negative, but all else is positive. Also, year==0
 * is illegal: adding 1 year to 1[bc] gives 1[ad]
 </li></ul>
 * The 'fracsec' in 'src' is preserved.  
 * The 'from/to' of the 'src' is preserved.  
 * A timezone in 'src' is allowed - it's presence is ignored.  
 * NOTE: There is no datetime_decrement() To decrement, set the 'incr' negative.

 *
 *  \param src
 *  \param incr
 *  \return int
 */

int datetime_increment(DateTime * src, DateTime * incr)
{
    int i, relfrom;
    DateTime cpdt, *dt;

    if (!datetime_is_valid_increment(src, incr))
	return datetime_error_code();

    /* special case - incrementing a relative might try to increment 
       or borrow from a "lower" field than src has, 
       so we use a copy to change from */

    if (src->mode == DATETIME_RELATIVE) {
	datetime_copy(&cpdt, src);
	relfrom = datetime_in_interval_day_second(src->from)
	    ? DATETIME_DAY : DATETIME_YEAR;
	datetime_change_from_to(&cpdt, relfrom, src->to, -1);	/* min. from */
	dt = &cpdt;
    }
    else
	dt = src;

    /* need to call carry first? (just to make sure?) */
    /*
       fprintf (stdout,"DEBUG: INCR %.12lf %.12lf = %.12lf\n", 
       _debug_decimal(dt), _debug_decimal(incr), 
       _debug_decimal(dt)+_debug_decimal(incr)); 
     */

    /* no sign change, just add */
    if ((dt->positive && incr->positive) ||
	(dt->mode == DATETIME_RELATIVE && !dt->positive && !incr->positive)) {

	for (i = incr->to; i >= incr->from; i--) {
	    _datetime_add_field(dt, incr, i);
	}
    }

    else if (!incr->positive || dt->mode == DATETIME_RELATIVE) {

	for (i = incr->to; i >= incr->from; i--) {
	    _datetime_subtract_field(dt, incr, i);
	}
    }

    /* now only two special cases of bc ABSOLUTE left */

    else if (!incr->positive) {	/* incr is negative, dt is positive */

	for (i = incr->to; i > DATETIME_YEAR; i--) {
	    _datetime_subtract_field(dt, incr, i);
	}
	_datetime_add_field(dt, incr, DATETIME_YEAR);
    }
    else {			/* incr is positive, dt is negative */

	for (i = incr->to; i > DATETIME_YEAR; i--) {
	    _datetime_add_field(dt, incr, i);
	}
	_datetime_subtract_field(dt, incr, DATETIME_YEAR);
    }
    /*
       fprintf (stdout,"DEBUG: INCR RESULT = %.12lf\n", _debug_decimal(dt)); 
     */
    if (src->mode == DATETIME_RELATIVE) {
	datetime_change_from_to(dt, src->from, src->to, -1);

	/* copy dt back into src to return */
	datetime_copy(src, dt);
    }

    return 0;
}


/*****************************************************************/
/*
   When calling, the field must be 
   in the range of src, but this is not enforced here.

   The only thing used from the "incr" DateTime is the value of
   the field being subtracted and the "from" & "to" 

   by the time we get here, if src is RELATIVE, src->from should
   already be minimized to allow borrowing from "lower" fields

 */

static int _datetime_subtract_field(DateTime * src, DateTime * incr,
				    int field)
{

    if (src->mode == DATETIME_RELATIVE) {
	DateTime srcinc, tinc;
	int borrow = 0;

	datetime_copy(&tinc, src);
	datetime_copy(&srcinc, incr);
	switch (field) {
	case DATETIME_SECOND:
	    /* no "-1" here - remember seconds is floating point */
	    /* might result in over borrowing, so have to check */
	    if (src->second < incr->second) {
		if ((int)(incr->second - src->second) == (incr->second - src->second)) {	/* diff is integer */
		    borrow = 1 + (incr->second - src->second - 1) / 60;
		}
		else
		    borrow = 1 + (incr->second - src->second) / 60;
		src->second += borrow * 60;
	    }
	    src->second -= incr->second;
	    if (borrow) {
		srcinc.minute = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_MINUTE);
	    }
	    break;

	case DATETIME_MINUTE:
	    if (src->minute < incr->minute) {
		borrow = 1 + (incr->minute - src->minute - 1) / 60;
		src->minute += borrow * 60;
	    }
	    src->minute -= incr->minute;
	    if (borrow) {
		srcinc.hour = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_HOUR);
	    }
	    break;

	case DATETIME_HOUR:
	    if (src->hour < incr->hour) {
		borrow = 1 + (incr->hour - src->hour - 1) / 24;
		src->hour += borrow * 24;
	    }
	    src->hour -= incr->hour;
	    if (borrow) {
		srcinc.day = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_DAY);
	    }
	    break;

	case DATETIME_DAY:
	    if (src->day < incr->day) {	/* SIGN CHANGE */
		src->day = incr->day - src->day;
		datetime_invert_sign(src);
		tinc.day = 0;
		src->hour = 0;
		src->minute = 0;
		src->second = 0.0;
		datetime_increment(src, &tinc);	/* no sign change */
	    }
	    else
		src->day -= incr->day;
	    break;

	case DATETIME_MONTH:
	    if (src->month < incr->month) {
		borrow = 1 + (incr->month - src->month - 1) / 12;
		src->month += borrow * 12;
	    }
	    src->month -= incr->month;
	    if (borrow) {
		srcinc.year = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_YEAR);
	    }
	    break;

	case DATETIME_YEAR:
	    if (src->year < incr->year) {	/* SIGN CHANGE */
		src->year = incr->year - src->year;
		datetime_invert_sign(src);
		tinc.year = 0;
		src->month = 0;
		datetime_increment(src, &tinc);	/* no sign change */
	    }
	    else
		src->year -= incr->year;
	    break;
	}
    }

    else if (src->mode == DATETIME_ABSOLUTE) {
	DateTime srcinc, tinc, cpsrc;
	int i, newdays, borrow = 0;


	datetime_copy(&srcinc, incr);	/* makes srcinc valid incr */
	switch (field) {
	case DATETIME_SECOND:
	    if (src->second < incr->second) {
		borrow = 1 + (incr->second - src->second - 1) / 60;
		src->second += borrow * 60;
	    }
	    src->second -= incr->second;
	    if (borrow) {
		srcinc.minute = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_MINUTE);
	    }
	    break;

	case DATETIME_MINUTE:
	    if (src->minute < incr->minute) {
		borrow = 1 + (incr->minute - src->minute - 1) / 60;
		src->minute += borrow * 60;
	    }
	    src->minute -= incr->minute;
	    if (borrow) {
		srcinc.hour = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_HOUR);
	    }
	    break;

	case DATETIME_HOUR:
	    if (src->hour < incr->hour) {
		borrow = 1 + (incr->hour - src->hour - 1) / 24;
		src->hour += borrow * 24;
	    }
	    src->hour -= incr->hour;
	    if (borrow) {
		srcinc.day = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_DAY);
	    }
	    break;

	case DATETIME_DAY:

	    if (src->day <= incr->day) {
		datetime_copy(&cpsrc, src);
		datetime_change_from_to(&cpsrc, DATETIME_YEAR,
					DATETIME_MONTH, -1);
		datetime_set_increment_type(&cpsrc, &tinc);
		tinc.month = 1;
		newdays = src->day;
		while (newdays <= incr->day) {
		    _datetime_subtract_field(&cpsrc, &tinc, DATETIME_MONTH);
		    newdays +=
			datetime_days_in_month(cpsrc.year, cpsrc.month,
					       cpsrc.positive);
		    borrow++;
		}
		src->day = newdays;
	    }
	    src->day -= incr->day;
	    if (borrow) {
		/*
		   src->year = cpsrc.year;
		   src->month = cpsrc.month;
		   src->positive = cpsrc.positive;
		 */
		/* check here & below - srcinc may be a day-second interval - mess anything up? */
		srcinc.month = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_MONTH);
	    }
	    break;

	case DATETIME_MONTH:
	    if (src->month <= incr->month) {
		borrow = 1 + (incr->month - src->month) / 12;
		src->month += borrow * 12;
	    }
	    src->month -= incr->month;
	    if (borrow) {
		srcinc.year = borrow;
		_datetime_subtract_field(src, &srcinc, DATETIME_YEAR);
	    }
	    break;

	case DATETIME_YEAR:
	    if (src->year <= incr->year) {	/* SIGN CHANGE */
		datetime_set_increment_type(src, &tinc);
		tinc.positive = src->positive;
		if (datetime_in_interval_year_month(tinc.to)) {
		    tinc.month = src->month - 1;	/* convert to REL */
		    src->year = incr->year - src->year + 1;
		    /* +1 to skip 0 */
		    datetime_invert_sign(src);
		    tinc.year = 0;
		    src->month = 1;
		    datetime_increment(src, &tinc);	/* no sign change */
		}
		else {		/* have to convert to days */
		    tinc.day = src->day - 1;	/* convert to REL */
		    for (i = src->month - 1; i > 0; i--) {
			tinc.day +=
			    datetime_days_in_month(src->year, i,
						   src->positive);
		    }
		    tinc.hour = src->hour;
		    tinc.minute = src->minute;
		    tinc.second = src->second;
		    src->year = incr->year - src->year + 1;
		    /* +1 to skip 0 */
		    datetime_invert_sign(src);
		    src->month = 1;
		    src->day = 1;
		    src->hour = src->minute = 0;
		    src->second = 0;
		    datetime_increment(src, &tinc);	/* no sign change */
		}
	    }
	    else
		src->year -= incr->year;
	    break;
	}
    }

    return 0;
}

/*****************************************************************/

/* When absolute is zero, all fields carry toward the future */
/* When absolute is one, sign of datetime is ignored */
static int _datetime_carry(DateTime * dt, int absolute)
{
    int i, carry;

    /* normalize day-sec (same for ABSOLUTE & RELATIVE) */
    for (i = dt->to; i > dt->from && i > DATETIME_DAY; i--) {
	switch (i) {
	case DATETIME_SECOND:
	    if (dt->second >= 60.) {
		carry = dt->second / 60.;
		dt->minute += carry;
		dt->second -= carry * 60;
	    }
	    break;
	case DATETIME_MINUTE:
	    if (dt->minute >= 60) {
		carry = dt->minute / 60;
		dt->hour += carry;
		dt->minute -= carry * 60;
	    }
	    break;
	case DATETIME_HOUR:
	    if (dt->hour >= 24) {
		carry = dt->hour / 24;
		dt->day += carry;
		dt->hour -= carry * 24;
	    }
	    break;
	}
    }

    /* give year a SIGN, temporarily */
    if (!absolute && !dt->positive && dt->mode == DATETIME_ABSOLUTE) {
	dt->year = -dt->year;
    }

    if (dt->from == DATETIME_YEAR && dt->to >= DATETIME_MONTH) {

	/* normalize yr-mo */
	if (dt->mode == DATETIME_ABSOLUTE) {
	    if (dt->month > 12) {	/* month will never be zero */
		carry = (dt->month - 1) / 12;	/* no carry until 13 */
		dt->year += carry;
		if (dt->year == 0)
		    dt->year = 1;
		dt->month -= carry * 12;
		/* 
		   if(dt->month == 0) dt->month = 1; 
		   shouldn't happen */
	    }
	}
	else {
	    if (dt->month >= 12) {
		carry = dt->month / 12;
		dt->year += carry;
		dt->month -= carry * 12;
	    }
	}

    }

    /* normalize yr-day */
    if (dt->mode == DATETIME_ABSOLUTE && dt->to > DATETIME_MONTH) {

	while (dt->day >
	       datetime_days_in_month(dt->year, dt->month, dt->positive)) {
	    dt->day -=
		datetime_days_in_month(dt->year, dt->month, dt->positive);
	    if (dt->month == 12) {	/* carry to year */
		dt->year++;
		if (dt->year == 0)
		    dt->year = 1;
		dt->month = 1;
	    }
	    else		/* no carry to year */
		dt->month++;

	}			/* end while */
    }				/* end if */

    /* undo giving year a SIGN, temporarily */
    if (!absolute && dt->mode == DATETIME_ABSOLUTE) {
	if (dt->year < 0) {
	    dt->year = -dt->year;
	    dt->positive = 0;
	}
	else
	    dt->positive = 1;
    }

    return 0;
}

static int _datetime_add_field(DateTime * src, DateTime * incr, int field)
{
    switch (field) {
    case DATETIME_SECOND:
	src->second += incr->second;
	break;
    case DATETIME_MINUTE:
	src->minute += incr->minute;
	break;
    case DATETIME_HOUR:
	src->hour += incr->hour;
	break;
    case DATETIME_DAY:
	src->day += incr->day;
	break;
    case DATETIME_MONTH:
	src->month += incr->month;
	break;
    case DATETIME_YEAR:
	src->year += incr->year;
	break;
    }
    if (src->mode == DATETIME_RELATIVE)
	_datetime_carry(src, 1);	/* do carries using absolute values */
    else
	_datetime_carry(src, 0);	/* do carries toward future */

    return 0;
}
