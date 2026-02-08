/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <grass/datetime.h>

static int _datetime_add_field(DateTime *, DateTime *, int);
static int _datetime_subtract_field(DateTime *, DateTime *, int);

/* NEW helpers to reduce cognitive complexity */
static int subtract_relative_field(DateTime *src, DateTime *incr, int field);
static int subtract_absolute_field(DateTime *src, DateTime *incr, int field);

static int subtract_rel_second(DateTime *src, DateTime *incr);
static int subtract_rel_minute(DateTime *src, DateTime *incr);
static int subtract_rel_hour(DateTime *src, DateTime *incr);
static int subtract_rel_day(DateTime *src, DateTime *incr);
static int subtract_rel_month(DateTime *src, DateTime *incr);
static int subtract_rel_year(DateTime *src, DateTime *incr);

static int subtract_abs_second(DateTime *src, DateTime *incr);
static int subtract_abs_minute(DateTime *src, DateTime *incr);
static int subtract_abs_hour(DateTime *src, DateTime *incr);
static int subtract_abs_day(DateTime *src, DateTime *incr);
static int subtract_abs_month(DateTime *src, DateTime *incr);
static int subtract_abs_year(DateTime *src, DateTime *incr);
/*****************************************************************/
#if 0  /* unused */
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
 * 'incr' must be a valid increment for 'src'. See
 <b>datetime_is_valid_increment()</b>,
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
int datetime_increment(DateTime *src, DateTime *incr)
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
        relfrom = datetime_in_interval_day_second(src->from) ? DATETIME_DAY
                                                             : DATETIME_YEAR;
        datetime_change_from_to(&cpdt, relfrom, src->to, -1); /* min. from */
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

    else if (!incr->positive) { /* incr is negative, dt is positive */

        for (i = incr->to; i > DATETIME_YEAR; i--) {
            _datetime_subtract_field(dt, incr, i);
        }
        _datetime_add_field(dt, incr, DATETIME_YEAR);
    }
    else { /* incr is positive, dt is negative */

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
static int _datetime_subtract_field(DateTime *src, DateTime *incr, int field)
{
    if (src->mode == DATETIME_RELATIVE)
        return subtract_relative_field(src, incr, field);

    if (src->mode == DATETIME_ABSOLUTE)
        return subtract_absolute_field(src, incr, field);

    return 0;
}

/*****************************************************************/

/* When absolute is zero, all fields carry toward the future */
/* When absolute is one, sign of datetime is ignored */
/* NEW helpers for _datetime_carry() */
static void carry_day_second(DateTime *dt);
static void maybe_temp_sign_year(DateTime *dt, int absolute);
static void normalize_year_month(DateTime *dt);
static void normalize_year_day(DateTime *dt);
static void undo_temp_sign_year(DateTime *dt, int absolute);

static int _datetime_carry(DateTime *dt, int absolute)
{
    carry_day_second(dt);

    maybe_temp_sign_year(dt, absolute);

    if (dt->from == DATETIME_YEAR && dt->to >= DATETIME_MONTH)
        normalize_year_month(dt);

    if (dt->mode == DATETIME_ABSOLUTE && dt->to > DATETIME_MONTH)
        normalize_year_day(dt);

    undo_temp_sign_year(dt, absolute);

    return 0;
}
static void carry_day_second(DateTime *dt)
{
    int i, carry;

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
}

static void maybe_temp_sign_year(DateTime *dt, int absolute)
{
    if (!absolute && !dt->positive && dt->mode == DATETIME_ABSOLUTE)
        dt->year = -dt->year;
}

static void normalize_year_month(DateTime *dt)
{
    int carry;

    if (dt->mode == DATETIME_ABSOLUTE) {
        if (dt->month > 12) {
            carry = (dt->month - 1) / 12;
            dt->year += carry;
            if (dt->year == 0)
                dt->year = 1;
            dt->month -= carry * 12;
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

static void normalize_year_day(DateTime *dt)
{
    while (dt->day >
           datetime_days_in_month(dt->year, dt->month, dt->positive)) {
        dt->day -= datetime_days_in_month(dt->year, dt->month, dt->positive);

        if (dt->month == 12) {
            dt->year++;
            if (dt->year == 0)
                dt->year = 1;
            dt->month = 1;
        }
        else {
            dt->month++;
        }
    }
}

static void undo_temp_sign_year(DateTime *dt, int absolute)
{
    if (!absolute && dt->mode == DATETIME_ABSOLUTE) {
        if (dt->year < 0) {
            dt->year = -dt->year;
            dt->positive = 0;
        }
        else {
            dt->positive = 1;
        }
    }
}


static int _datetime_add_field(DateTime *src, DateTime *incr, int field)
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
        _datetime_carry(src, 1); /* do carries using absolute values */
    else
        _datetime_carry(src, 0); /* do carries toward future */

    return 0;
}

/* ---------------- NEW: RELATIVE dispatcher ---------------- */
static int subtract_relative_field(DateTime *src, DateTime *incr, int field)
{
    switch (field) {
    case DATETIME_SECOND:
        return subtract_rel_second(src, incr);
    case DATETIME_MINUTE:
        return subtract_rel_minute(src, incr);
    case DATETIME_HOUR:
        return subtract_rel_hour(src, incr);
    case DATETIME_DAY:
        return subtract_rel_day(src, incr);
    case DATETIME_MONTH:
        return subtract_rel_month(src, incr);
    case DATETIME_YEAR:
        return subtract_rel_year(src, incr);
    default:
        return 0;
    }
}

static int subtract_rel_second(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->second < incr->second) {
        double diff = incr->second - src->second;

        /* keep original integer-diff special case */
        if ((int)diff == diff)
            borrow = 1 + (diff - 1) / 60;
        else
            borrow = 1 + diff / 60;

        src->second += borrow * 60;
    }

    src->second -= incr->second;

    if (borrow) {
        srcinc.minute = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_MINUTE);
    }
    return 0;
}

static int subtract_rel_minute(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->minute < incr->minute) {
        borrow = 1 + (incr->minute - src->minute - 1) / 60;
        src->minute += borrow * 60;
    }
    src->minute -= incr->minute;

    if (borrow) {
        srcinc.hour = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_HOUR);
    }
    return 0;
}

static int subtract_rel_hour(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->hour < incr->hour) {
        borrow = 1 + (incr->hour - src->hour - 1) / 24;
        src->hour += borrow * 24;
    }
    src->hour -= incr->hour;

    if (borrow) {
        srcinc.day = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_DAY);
    }
    return 0;
}

static int subtract_rel_day(DateTime *src, DateTime *incr)
{
    DateTime tinc;

    /* keep original sign-change behavior exactly */
    if (src->day < incr->day) { /* SIGN CHANGE */
        datetime_copy(&tinc, src);
        src->day = incr->day - src->day;
        datetime_invert_sign(src);
        tinc.day = 0;
        src->hour = 0;
        src->minute = 0;
        src->second = 0.0;
        datetime_increment(src, &tinc); /* no sign change */
        return 0;
    }

    src->day -= incr->day;
    return 0;
}

static int subtract_rel_month(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->month < incr->month) {
        borrow = 1 + (incr->month - src->month - 1) / 12;
        src->month += borrow * 12;
    }
    src->month -= incr->month;

    if (borrow) {
        srcinc.year = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_YEAR);
    }
    return 0;
}

static int subtract_rel_year(DateTime *src, DateTime *incr)
{
    DateTime tinc;

    /* keep original sign-change behavior exactly */
    if (src->year < incr->year) { /* SIGN CHANGE */
        datetime_copy(&tinc, src);
        src->year = incr->year - src->year;
        datetime_invert_sign(src);
        tinc.year = 0;
        src->month = 0;
        datetime_increment(src, &tinc); /* no sign change */
        return 0;
    }

    src->year -= incr->year;
    return 0;
}

/* ---------------- NEW: ABSOLUTE dispatcher ---------------- */
static int subtract_absolute_field(DateTime *src, DateTime *incr, int field)
{
    switch (field) {
    case DATETIME_SECOND:
        return subtract_abs_second(src, incr);
    case DATETIME_MINUTE:
        return subtract_abs_minute(src, incr);
    case DATETIME_HOUR:
        return subtract_abs_hour(src, incr);
    case DATETIME_DAY:
        return subtract_abs_day(src, incr);
    case DATETIME_MONTH:
        return subtract_abs_month(src, incr);
    case DATETIME_YEAR:
        return subtract_abs_year(src, incr);
    default:
        return 0;
    }
}

static int subtract_abs_second(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr); /* makes srcinc valid incr */

    if (src->second < incr->second) {
        borrow = 1 + (incr->second - src->second - 1) / 60;
        src->second += borrow * 60;
    }
    src->second -= incr->second;

    if (borrow) {
        srcinc.minute = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_MINUTE);
    }
    return 0;
}

static int subtract_abs_minute(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->minute < incr->minute) {
        borrow = 1 + (incr->minute - src->minute - 1) / 60;
        src->minute += borrow * 60;
    }
    src->minute -= incr->minute;

    if (borrow) {
        srcinc.hour = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_HOUR);
    }
    return 0;
}

static int subtract_abs_hour(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->hour < incr->hour) {
        borrow = 1 + (incr->hour - src->hour - 1) / 24;
        src->hour += borrow * 24;
    }
    src->hour -= incr->hour;

    if (borrow) {
        srcinc.day = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_DAY);
    }
    return 0;
}

static int subtract_abs_day(DateTime *src, DateTime *incr)
{
    DateTime srcinc, tinc, cpsrc;
    int newdays, borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->day <= incr->day) {
        datetime_copy(&cpsrc, src);
        datetime_change_from_to(&cpsrc, DATETIME_YEAR, DATETIME_MONTH, -1);
        datetime_set_increment_type(&cpsrc, &tinc);
        tinc.month = 1;

        newdays = src->day;
        while (newdays <= incr->day) {
            _datetime_subtract_field(&cpsrc, &tinc, DATETIME_MONTH);
            newdays += datetime_days_in_month(cpsrc.year, cpsrc.month,
                                              cpsrc.positive);
            borrow++;
        }
        src->day = newdays;
    }

    src->day -= incr->day;

    if (borrow) {
        srcinc.month = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_MONTH);
    }
    return 0;
}

static int subtract_abs_month(DateTime *src, DateTime *incr)
{
    DateTime srcinc;
    int borrow = 0;

    datetime_copy(&srcinc, incr);

    if (src->month <= incr->month) {
        borrow = 1 + (incr->month - src->month) / 12;
        src->month += borrow * 12;
    }
    src->month -= incr->month;

    if (borrow) {
        srcinc.year = borrow;
        _datetime_subtract_field(src, &srcinc, DATETIME_YEAR);
    }
    return 0;
}

static int subtract_abs_year(DateTime *src, DateTime *incr)
{
    DateTime tinc;
    int i;

    if (src->year <= incr->year) { /* SIGN CHANGE */
        datetime_set_increment_type(src, &tinc);
        tinc.positive = src->positive;

        if (datetime_in_interval_year_month(tinc.to)) {
            tinc.month = src->month - 1; /* convert to REL */
            src->year = incr->year - src->year + 1; /* +1 to skip 0 */
            datetime_invert_sign(src);
            tinc.year = 0;
            src->month = 1;
            datetime_increment(src, &tinc); /* no sign change */
        }
        else { /* have to convert to days */
            tinc.day = src->day - 1; /* convert to REL */
            for (i = src->month - 1; i > 0; i--) {
                tinc.day += datetime_days_in_month(src->year, i, src->positive);
            }
            tinc.hour = src->hour;
            tinc.minute = src->minute;
            tinc.second = src->second;

            src->year = incr->year - src->year + 1; /* +1 to skip 0 */
            datetime_invert_sign(src);
            src->month = 1;
            src->day = 1;
            src->hour = src->minute = 0;
            src->second = 0;
            datetime_increment(src, &tinc); /* no sign change */
        }
        return 0;
    }

    src->year -= incr->year;
    return 0;
}

