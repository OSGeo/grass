/*!
  \file lib/db/dbmi_base/datetime.c
  
  \brief DBMI Library (base) - datetime conversions
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdio.h>
#include <string.h>
#include <grass/dbmi.h>

static char ds = '-';
static char ts = ':';

/*!
  \brief Convert datetime value into string

  \param value pointer to dbValue
  \param sqltype SQL data type
  \param[out] string pointer to dbString

  \return DB_OK on success
 */
int db_convert_value_datetime_into_string(dbValue *value, int sqltype,
					  dbString *string)
{
    int to, from;
    int year, month, day, hour, minute;
    double seconds;
    char *xs;
    char buf[64];

    if (db_test_value_datetime_current(value))
	return db_set_string(string, "CURRENT");

    *buf = 0;

    year = db_get_value_year(value);
    month = db_get_value_month(value);
    day = db_get_value_day(value);
    hour = db_get_value_hour(value);
    minute = db_get_value_minute(value);
    seconds = db_get_value_seconds(value);
    if (seconds < 10.0)
	xs = "0";
    else
	xs = "";

    db_interval_range(sqltype, &from, &to);
    switch (from) {
    case DB_YEAR:
	switch (to) {
	case DB_YEAR:
	    sprintf(buf, "%d", year);
	    break;
	case DB_MONTH:
	    sprintf(buf, "%d%c%02d", year, ds, month);
	    break;
	case DB_DAY:
	    sprintf(buf, "%d%c%02d%c%02d", year, ds, month, ds, day);
	    break;
	case DB_HOUR:
	    sprintf(buf, "%d%c%02d%c%02d %02d", year, ds, month, ds, day, hour);
	    break;
	case DB_MINUTE:
	    sprintf(buf, "%d%c%02d%c%02d %02d%c%02d",
		    year, ds, month, ds, day, hour, ts, minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%d%c%02d%c%02d %02d%c%02d%c%s%.10g",
		    year, ds, month, ds, day, hour, ts, minute, ts, xs,
		    seconds);
	    break;
	}
	break;
    case DB_MONTH:
	switch (to) {
	case DB_MONTH:
	    sprintf(buf, "%d", month);
	    break;
	case DB_DAY:
	    sprintf(buf, "%02d%c%02d", month, ds, day);
	    break;
	case DB_HOUR:
	    sprintf(buf, "%02d%c%02d %02d", month, ds, day, hour);
	    break;
	case DB_MINUTE:
	    sprintf(buf, "%02d%c%02d %02d%c%02d", month, ds, day, hour, ts, minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%02d%c%02d %02d%c%02d%c%s%.10g",
		    month, ds, day, hour, ts, minute, ts, xs, seconds);
	    break;
	}
	break;
    case DB_DAY:
	switch (to) {
	case DB_DAY:
	    sprintf(buf, "%02d", day);
	    break;
	case DB_HOUR:
	    sprintf(buf, "%02d %02d", day, hour);
	    break;
	case DB_MINUTE:
	    sprintf(buf, "%02d %02d%c%02d", day, hour, ts, minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%02d %02d%c%02d%c%s%.10g",
		    day, hour, ts, minute, ts, xs, seconds);
	    break;
	}
	break;
    case DB_HOUR:
	switch (to) {
	case DB_HOUR:
	    sprintf(buf, "%02d", hour);
	    break;
	case DB_MINUTE:
	    sprintf(buf, "%02d%c%02d", hour, ts, minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%02d%c%02d%c%s%.10g", hour, ts, minute, ts, xs,
		    seconds);
	    break;
	}
	break;
    case DB_MINUTE:
	switch (to) {
	case DB_MINUTE:
	    sprintf(buf, "%02d", minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%02d%c%s%.10g", minute, ts, xs, seconds);
	    break;
	}
	break;
    case DB_SECOND:
    case DB_FRACTION:
	switch (to) {
	case DB_SECOND:
	case DB_FRACTION:
	    sprintf(buf, "%g", seconds);
	    break;
	}
	break;
    default:
	switch (sqltype) {
	case DB_SQL_TYPE_DATE:
	    sprintf(buf, "%d%c%02d%c%02d", year, ds, month, ds, day);
	    break;
	case DB_SQL_TYPE_TIME:
	    sprintf(buf, "%02d%c%02d%c%s%.10g",
		    hour, ts, minute, ts, xs, seconds);
	    break;
	case DB_SQL_TYPE_TIMESTAMP:
	    sprintf(buf, "%d%c%02d%c%02d %02d%c%02d%c%s%.10g",
		    year, ds, month, ds, day, hour, ts, minute, ts, xs,
		    seconds);
	    break;
	}
    }
    return db_set_string(string, buf);
}

/*!
  \brief Convert datetime string to value

  The format of <em>buf</em> must be as follows
   - buf == "CURRENT" in a case-insignificant fashion value is marked as current
   - sqltype == DB_SQL_TYPE_DATE
    "year*month*day"
   - sqltype == DB_SQL_TYPE_TIME
    "hour*minute*second"
   - sqltype == DB_SQL_TYPE_TIMESTAMP
    "year*month*day hour*minute*second"
   - otherwise the to and from markings in sqltype are used, 
     where "*" represents any non-whitespace character
  
  \param buf input string buffer
  \param sqltype SQL data type
  \param[out] value pointer to dbValue to be set

  \return DB_OK
*/
int db_convert_Cstring_to_value_datetime(const char *buf, int sqltype,
					 dbValue *value)
{
    int from, to;
    int year, month, day, hour, minute;
    double seconds;

    year = month = day = 0;
    hour = minute = 0;
    seconds = 0;

    if (db_nocase_compare(buf, "CURRENT")) {
	db_set_value_datetime_current(value);
	return DB_OK;
    }

    db_interval_range(sqltype, &from, &to);
    switch (from) {
    case DB_YEAR:
	switch (to) {
	case DB_YEAR:
	    sscanf(buf, "%d", &year);
	    break;
	case DB_MONTH:
	    sscanf(buf, "%d%*c%d", &year, &month);
	    break;
	case DB_DAY:
	    sscanf(buf, "%d%*c%d%*c%d", &year, &month, &day);
	    break;
	case DB_HOUR:
	    sscanf(buf, "%d%*c%d%*c%d %d", &year, &month, &day, &hour);
	    break;
	case DB_MINUTE:
	    sscanf(buf, "%d%*c%d%*c%d %d%*c%d",
		   &year, &month, &day, &hour, &minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sscanf(buf, "%d%*c%d%*c%d %d%*c%d%*c%lf",
		   &year, &month, &day, &hour, &minute, &seconds);
	    break;
	}
	break;
    case DB_MONTH:
	switch (to) {
	case DB_MONTH:
	    sscanf(buf, "%d", &month);
	    break;
	case DB_DAY:
	    sscanf(buf, "%d%*c%d", &month, &day);
	    break;
	case DB_HOUR:
	    sscanf(buf, "%d%*c%d %d", &month, &day, &hour);
	    break;
	case DB_MINUTE:
	    sscanf(buf, "%d%*c%d %d%*c%d", &month, &day, &hour, &minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sscanf(buf, "%d%*c%d %d%*c%d%*c%lf",
		   &month, &day, &hour, &minute, &seconds);
	    break;
	}
	break;
    case DB_DAY:
	switch (to) {
	case DB_DAY:
	    sscanf(buf, "%d", &day);
	    break;
	case DB_HOUR:
	    sscanf(buf, "%d %d", &day, &hour);
	    break;
	case DB_MINUTE:
	    sscanf(buf, "%d %d%*c%d", &day, &hour, &minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sscanf(buf, "%d %d%*c%d%*c%lf", &day, &hour, &minute, &seconds);
	    break;
	}
	break;
    case DB_HOUR:
	switch (to) {
	case DB_HOUR:
	    sscanf(buf, "%d", &hour);
	    break;
	case DB_MINUTE:
	    sscanf(buf, "%d%*c%d", &hour, &minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sscanf(buf, "%d%*c%d%*c%lf", &hour, &minute, &seconds);
	    break;
	}
	break;
    case DB_MINUTE:
	switch (to) {
	case DB_MINUTE:
	    sscanf(buf, "%d", &minute);
	    break;
	case DB_SECOND:
	case DB_FRACTION:
	    sscanf(buf, "%d%*c%lf", &minute, &seconds);
	    break;
	}
	break;
    case DB_SECOND:
    case DB_FRACTION:
	sscanf(buf, "%lf", &seconds);
	break;
    default:
	switch (sqltype) {
	case DB_SQL_TYPE_DATE:
	    sscanf(buf, "%d%*c%d%*c%d", &year, &month, &day);
	    break;
	case DB_SQL_TYPE_TIME:
	    sscanf(buf, "%d%*c%d%*c%lf", &hour, &minute, &seconds);
	    break;
	case DB_SQL_TYPE_TIMESTAMP:
	    sscanf(buf, "%d%*c%d%*c%d %d%*c%d%*c%lf",
		   &year, &month, &day, &hour, &minute, &seconds);
	    break;
	}
    }

    db_set_value_year(value, year);
    db_set_value_month(value, month);
    db_set_value_day(value, day);
    db_set_value_hour(value, hour);
    db_set_value_minute(value, minute);
    db_set_value_seconds(value, seconds);

    return DB_OK;
}
