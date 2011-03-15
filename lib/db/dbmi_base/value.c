#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_test_value_isnull(dbValue * value)
{
    return (value->isNull != 0);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_int(dbValue * value)
{
    return (value->i);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
double db_get_value_double(dbValue * value)
{
    return (value->d);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
/* for given value and C type of value returns double representation */
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
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_value_string(dbValue * value)
{
    return (db_get_string(&value->s));
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_year(dbValue * value)
{
    return (value->t.year);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_month(dbValue * value)
{
    return (value->t.month);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_day(dbValue * value)
{
    return (value->t.day);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_hour(dbValue * value)
{
    return (value->t.hour);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_value_minute(dbValue * value)
{
    return (value->t.minute);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
double db_get_value_seconds(dbValue * value)
{
    return (value->t.seconds);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_null(dbValue * value)
{
    value->isNull = 1;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_not_null(dbValue * value)
{
    value->isNull = 0;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_int(dbValue * value, int i)
{
    value->i = i;
    db_set_value_not_null(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_double(dbValue * value, double d)
{
    value->d = d;
    db_set_value_not_null(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_value_string(dbValue * value, const char *s)
{
    db_set_value_not_null(value);
    return db_set_string(&value->s, s);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_year(dbValue * value, int year)
{
    value->t.year = year;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_month(dbValue * value, int month)
{
    value->t.month = month;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_day(dbValue * value, int day)
{
    value->t.day = day;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_hour(dbValue * value, int hour)
{
    value->t.hour = hour;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_minute(dbValue * value, int minute)
{
    value->t.minute = minute;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_seconds(dbValue * value, double seconds)
{
    value->t.seconds = seconds;
    db_set_value_datetime_not_current(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_test_value_datetime_current(dbValue * value)
{
    return (value->t.current != 0);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_datetime_current(dbValue * value)
{
    value->t.current = 1;
    db_set_value_not_null(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_value_datetime_not_current(dbValue * value)
{
    value->t.current = 0;
    db_set_value_not_null(value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
/* copy value from src to destination */
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
   \fn 
   \brief 
   \return 
   \param 
 */
void db_CatValArray_init(dbCatValArray * arr)
{
    arr->n_values = 0;
    arr->alloc = 0;
    arr->value = NULL;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
 */
int db_CatValArray_alloc(dbCatValArray * arr, int n)
{
    arr->value = (dbCatVal *) G_calloc(n, sizeof(dbCatVal));

    arr->alloc = n;

    return DB_OK;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_CatValArray_realloc(dbCatValArray * arr, int n)
{
    arr->value = (dbCatVal *) G_realloc(arr->value, n * sizeof(dbCatVal));

    arr->alloc = n;

    return DB_OK;
}
