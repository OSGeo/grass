/*!
  \file lib/db/dbmi_base/valuefmt.c
  
  \brief DBMI Library (base) - value formatting
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \brief Convert string to value

  \param Cstring string buffer
  \param sqltype SQL data type
  \param[out] value pointer to dbValue

  \return DB_OK on success
  \return DB_FAILED on error
 */
int db_convert_Cstring_to_value(const char *Cstring, int sqltype, dbValue * value)
{
    int i;
    double d;

    switch (db_sqltype_to_Ctype(sqltype)) {
    case DB_C_TYPE_STRING:
	return db_set_value_string(value, Cstring);
    case DB_C_TYPE_INT:
	i = 0;
	sscanf(Cstring, "%d", &i);
	db_set_value_int(value, i);
	break;
    case DB_C_TYPE_DOUBLE:
	d = 0.0;
	sscanf(Cstring, "%lf", &d);
	db_set_value_double(value, d);
	break;
    case DB_C_TYPE_DATETIME:
	return db_convert_Cstring_to_value_datetime(Cstring, sqltype, value);
    default:
	db_error(_("db_convert_Cstring_to_value(): unrecognized sqltype"));
	return DB_FAILED;
    }
    return DB_OK;
}

/*!
  \brief Convert value to string
  
  \param value pointer to dbValue
  \param sqltype SQL data type
  \param[out] string pointer to dbString

  \return DB_OK on success
*/
int db_convert_value_to_string(dbValue * value, int sqltype, dbString * string)
{
    char buf[64];
    const char *bp = buf;

    if (db_test_value_isnull(value)) {
	*buf = 0;
    }
    else {
	switch (db_sqltype_to_Ctype(sqltype)) {
	case DB_C_TYPE_INT:
	    sprintf(buf, "%d", db_get_value_int(value));
	    break;
	case DB_C_TYPE_DOUBLE:
	    sprintf(buf, "%.15g", db_get_value_double(value));
	    G_trim_decimal(buf);
	    break;
	case DB_C_TYPE_STRING:
	    bp = db_get_value_string(value);
	    break;
	case DB_C_TYPE_DATETIME:
	    return db_convert_value_datetime_into_string(value, sqltype,
							 string);
	default:
	    db_error(_("db_convert_value_into_string(): unrecongized sqltype-type"));
	    return DB_FAILED;
	}
    }
    return db_set_string(string, bp);
}
