/*!
  \file lib/db/dbmi_base/sqlCtype.c
  
  \brief DBMI Library (base) - SQL data type to C data type

  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief Get C data type based on given SQL data type

  \param sqltype SQL data type

  \return related C data type
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
