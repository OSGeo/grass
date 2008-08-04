#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_sqltype_to_Ctype(int sqltype)
{
    switch (sqltype) {
    case DB_SQL_TYPE_INTEGER:
	return DB_C_TYPE_INT;
    case DB_SQL_TYPE_SMALLINT:
	return DB_C_TYPE_INT;
    case DB_SQL_TYPE_REAL:
	return DB_C_TYPE_DOUBLE;
    case DB_SQL_TYPE_DOUBLE_PRECISION:
	return DB_C_TYPE_DOUBLE;
    case DB_SQL_TYPE_SERIAL:
	return DB_C_TYPE_INT;
    }

    switch (sqltype & ~DB_DATETIME_MASK) {
    case DB_SQL_TYPE_DATE:
    case DB_SQL_TYPE_TIME:
    case DB_SQL_TYPE_TIMESTAMP:
    case DB_SQL_TYPE_INTERVAL:
	return DB_C_TYPE_DATETIME;
    }

    return DB_C_TYPE_STRING;
}
