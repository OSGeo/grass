#include <string.h>
#include <grass/dbmi.h>

/*!
   \brief returns sqltype description
   \return 
   \param 
 */
const char *db_sqltype_name(int sqltype)
{
    static char buf[256];
    int from, to;

    switch (sqltype) {
    case DB_SQL_TYPE_CHARACTER:
	return "CHARACTER";
    case DB_SQL_TYPE_NUMERIC:
	return "NUMERIC";
    case DB_SQL_TYPE_DECIMAL:
	return "DECIMAL";
    case DB_SQL_TYPE_SMALLINT:
	return "SMALLINT";
    case DB_SQL_TYPE_INTEGER:
	return "INTEGER";
    case DB_SQL_TYPE_REAL:
	return "REAL";
    case DB_SQL_TYPE_DOUBLE_PRECISION:
	return "DOUBLE PRECISION";
    case DB_SQL_TYPE_DATE:
	return "DATE";
    case DB_SQL_TYPE_TIME:
	return "TIME";
    case DB_SQL_TYPE_SERIAL:
	return "SERIAL";
    case DB_SQL_TYPE_TEXT:
	return "TEXT";
    }
    switch (sqltype & ~DB_DATETIME_MASK) {
    case DB_SQL_TYPE_TIMESTAMP:
	strcpy(buf, "TIMESTAMP ");
	break;
    case DB_SQL_TYPE_INTERVAL:
	strcpy(buf, "INTERVAL ");
	break;
    default:
	return "UNKNOWN";
    }

    db_interval_range(sqltype, &from, &to);

    switch (from) {
    case DB_YEAR:
	strcat(buf, "YEAR");
	break;
    case DB_MONTH:
	strcat(buf, "MONTH");
	break;
    case DB_DAY:
	strcat(buf, "DAY");
	break;
    case DB_HOUR:
	strcat(buf, "HOUR");
	break;
    case DB_MINUTE:
	strcat(buf, "MINUTE");
	break;
    case DB_SECOND:
	strcat(buf, "SECOND");
	break;
    case DB_FRACTION:
	strcat(buf, "FRACTION");
	break;
    }

    if (from)
	strcat(buf, " to");
    if (to)
	strcat(buf, " ");

    switch (to) {
    case DB_YEAR:
	strcat(buf, "YEAR");
	break;
    case DB_MONTH:
	strcat(buf, "MONTH");
	break;
    case DB_DAY:
	strcat(buf, "DAY");
	break;
    case DB_HOUR:
	strcat(buf, "HOUR");
	break;
    case DB_MINUTE:
	strcat(buf, "MINUTE");
	break;
    case DB_SECOND:
	strcat(buf, "SECOND");
	break;
    case DB_FRACTION:
	strcat(buf, "FRACTION");
	break;
    }

    return buf;
}
