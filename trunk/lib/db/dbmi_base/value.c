/*!
  \file lib/db/dbmi_base/value.c
  
  \brief DBMI Library (base) - value management
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdlib.h>
#include <grass/dbmi.h>

/*!
  \brief Check of value is null

  \param value pointer to dbValue

  \return non-zero is null
  \return zero is not null
*/
int db_test_value_isnull(dbValue * value)
{
    return (value->isNull != 0);
}

/*!
  \brief Get integer value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_int(dbValue * value)
{
    return (value->i);
}

/*!
  \brief Get double precision value

  \param value pointer to dbValue

  \return value
*/
double db_get_value_double(dbValue * value)
{
    return (value->d);
}

/*!
  \brief Get value as double
  
  For given value and C type of value returns double representation.

  \param value pointer to dbValue
  \param ctype C data type

  \return value
*/
double db_get_value_as_double(dbValue * value, int ctype)
{
    double val;

    switch (ctype) {
    case (DB_C_TYPE_INT):
	val = (double)db_get_value_int(value);
	break;
    case (DB_C_TYPE_STRING):
	val = atof(db_get_value_string(value));
	break;
    case (DB_C_TYPE_DOUBLE):
	val = db_get_value_double(value);
	break;
    default:
	val = 0;
    }
    return val;
}

/*!
  \brief Get string value

  \param value pointer to dbValue

  \return value
*/
const char *db_get_value_string(dbValue * value)
{
    return (db_get_string(&value->s));
}

/*!
  \brief Get year value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_year(dbValue * value)
{
    return (value->t.year);
}

/*!
  \brief Get month value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_month(dbValue * value)
{
    return (value->t.month);
}

/*!
  \brief Get day value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_day(dbValue * value)
{
    return (value->t.day);
}

/*!
  \brief Get hour value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_hour(dbValue * value)
{
    return (value->t.hour);
}

/*!
  \brief Get minute value

  \param value pointer to dbValue

  \return value
*/
int db_get_value_minute(dbValue * value)
{
    return (value->t.minute);
}

/*!
  \brief Get seconds value

  \param value pointer to dbValue

  \return value
*/
double db_get_value_seconds(dbValue * value)
{
    return (value->t.seconds);
}

/*!
  \brief Set value to null

  \param value pointer to dbValue
*/
void db_set_value_null(dbValue * value)
{
    value->isNull = 1;
}

/*!
  \brief Set value to not null

  \param value pointer to dbValue
*/
void db_set_value_not_null(dbValue * value)
{
    value->isNull = 0;
}

/*!
  \brief Set integer value

  \param value pointer to dbValue
  \param i integer value
*/
void db_set_value_int(dbValue * value, int i)
{
    value->i = i;
    db_set_value_not_null(value);
}

/*!
  \brief Set double precision value

  \param value pointer to dbValue
  \param d double value
*/
void db_set_value_double(dbValue * value, double d)
{
    value->d = d;
    db_set_value_not_null(value);
}

/*!
  \brief Set string value

  \param value pointer to dbValue
  \param s string value
*/
int db_set_value_string(dbValue * value, const char *s)
{
    db_set_value_not_null(value);
    return db_set_string(&value->s, s);
}

/*!
  \brief Set year value

  \param value pointer to dbValue
  \param year year value
*/
void db_set_value_year(dbValue * value, int year)
{
    value->t.year = year;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Set month value

  \param value pointer to dbValue
  \param month month value
*/
void db_set_value_month(dbValue * value, int month)
{
    value->t.month = month;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Set day value

  \param value pointer to dbValue
  \param day day value
*/
void db_set_value_day(dbValue * value, int day)
{
    value->t.day = day;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Set hour value

  \param value pointer to dbValue
  \param hour hour value
*/
void db_set_value_hour(dbValue * value, int hour)
{
    value->t.hour = hour;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Set minute value

  \param value pointer to dbValue
  \param minute minute value
*/
void db_set_value_minute(dbValue * value, int minute)
{
    value->t.minute = minute;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Set seconds value

  \param value pointer to dbValue
  \param seconds seconds value
*/
void db_set_value_seconds(dbValue * value, double seconds)
{
    value->t.seconds = seconds;
    db_set_value_datetime_not_current(value);
}

/*!
  \brief Check if datatime is current

  \param value pointer to dbValue

  \return non-zero for true
  \return zero for false
*/
int db_test_value_datetime_current(dbValue * value)
{
    return (value->t.current != 0);
}

/*!
  \brief Set datetime to current

  \param value pointer to dbValue
*/
void db_set_value_datetime_current(dbValue * value)
{
    value->t.current = 1;
    db_set_value_not_null(value);
}

/*!
  \brief Set value to non-current

  \param value pointer to dbValue
*/
void db_set_value_datetime_not_current(dbValue * value)
{
    value->t.current = 0;
    db_set_value_not_null(value);
}

/*!
  \brief Copy value

  Copy value from src to destination
  
  \param dst destination dbValue
  \param src source dbValue
*/
void db_copy_value(dbValue * dst, dbValue * src)
{
    dst->isNull = src->isNull;
    dst->i = src->i;
    dst->d = src->d;
    if (src->s.nalloc > 0)
	db_copy_string(&(dst->s), &(src->s));
    dst->t.current = src->t.current;
    dst->t.year = src->t.year;
    dst->t.month = src->t.month;
    dst->t.day = src->t.day;
    dst->t.hour = src->t.hour;
    dst->t.minute = src->t.minute;
    dst->t.seconds = src->t.seconds;
}

/*!
  \brief Initialize dbCatValArray

  \param arr pointer to dbCatValArray to be initialized
*/
void db_CatValArray_init(dbCatValArray * arr)
{
    arr->n_values = 0;
    arr->alloc = 0;
    arr->value = NULL;
}

/*!
  \brief Free allocated dbCatValArray

  \param arr pointer to dbCatValArray
*/
void db_CatValArray_free(dbCatValArray * arr)
{
    if (arr->ctype == DB_C_TYPE_STRING || arr->ctype == DB_C_TYPE_DATETIME) {
	int i;

	for (i = 0; i < arr->n_values; i++) {
	    if (arr->ctype == DB_C_TYPE_STRING && arr->value[i].val.s) {
		db_free_string(arr->value[i].val.s);
	    }
	    if (arr->ctype == DB_C_TYPE_DATETIME && arr->value[i].val.t) {
		db_free(arr->value[i].val.t);
	    }
	}
    }

    G_free(arr->value);
}

/*!
  \brief Allocate dbCatValArray

  \todo return type void?

  \param arr pointer to dbCatValArray
  \param n number of items

  \return DB_OK
*/
int db_CatValArray_alloc(dbCatValArray * arr, int n)
{
    arr->value = (dbCatVal *) G_calloc(n, sizeof(dbCatVal));

    arr->alloc = n;

    return DB_OK;
}

/*!
  \brief Realloc dbCatValArray

  \todo return code void?

  \param arr pointer to dbCatValArray
  \param n number of items

  \return DB_OK
*/
int db_CatValArray_realloc(dbCatValArray * arr, int n)
{
    arr->value = (dbCatVal *) G_realloc(arr->value, n * sizeof(dbCatVal));

    arr->alloc = n;

    return DB_OK;
}
