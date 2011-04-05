/*!
  \file lib/db/dbmi_base/interval.c
  
  \brief DBMI Library (base) - range, interval procedures
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief Define range based on SQL data type

  \param sqltype SQL data type
  \param[out] from 
  \param[out] to
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
