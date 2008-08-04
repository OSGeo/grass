#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_interval_range(int sqltype, int *from, int *to)
{
    switch (sqltype) {
    case DB_SQL_TYPE_DATE:
	*from = DB_YEAR;
	*to = DB_DAY;
	return;
    case DB_SQL_TYPE_TIME:
	*from = DB_HOUR;
	*to = DB_FRACTION;
	return;
    }

    if (sqltype & DB_YEAR)
	*from = DB_YEAR;
    else if (sqltype & DB_MONTH)
	*from = DB_MONTH;
    else if (sqltype & DB_DAY)
	*from = DB_DAY;
    else if (sqltype & DB_HOUR)
	*from = DB_HOUR;
    else if (sqltype & DB_MINUTE)
	*from = DB_MINUTE;
    else if (sqltype & DB_SECOND)
	*from = DB_SECOND;
    else if (sqltype & DB_FRACTION)
	*from = DB_FRACTION;
    else
	*from = 0;

    if (sqltype & DB_FRACTION)
	*to = DB_FRACTION;
    else if (sqltype & DB_SECOND)
	*to = DB_SECOND;
    else if (sqltype & DB_MINUTE)
	*to = DB_MINUTE;
    else if (sqltype & DB_HOUR)
	*to = DB_HOUR;
    else if (sqltype & DB_DAY)
	*to = DB_DAY;
    else if (sqltype & DB_MONTH)
	*to = DB_MONTH;
    else if (sqltype & DB_YEAR)
	*to = DB_YEAR;
    else
	*to = 0;
}
