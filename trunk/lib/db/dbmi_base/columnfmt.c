/*!
  \file lib/db/dbmi_base/columnfmt.c
  
  \brief DBMI Library (base) - columns formatting
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief ?
  
  \param Cstring string buffer
  \param column pointer to dbColumn
  
  \return ?
*/
int db_convert_Cstring_to_column_value(const char *Cstring, dbColumn *column)
{
    dbValue *value;
    int sqltype;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_value(column);
    return db_convert_Cstring_to_value(Cstring, sqltype, value);
}

/*!
  \brief ?
  
  \param Cstring string buffer
  \param column pointer to dbColumn

  \return ?
*/
int db_convert_Cstring_to_column_default_value(const char *Cstring, dbColumn *column)
{
    dbValue *value;
    int sqltype;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_default_value(column);
    return db_convert_Cstring_to_value(Cstring, sqltype, value);
}

/*!
  \brief ?

  \param column pointer to dbColumn
  \param string pointer to dbString

  \return ?
*/
int db_convert_column_value_to_string(dbColumn *column, dbString *string)
{
    int sqltype;
    dbValue *value;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_value(column);
    return db_convert_value_to_string(value, sqltype, string);
}

/*!
  \brief ?

  \param column pointer to dbColumn
  \param string pointer to dbString

  \return ?
*/
int db_convert_column_default_value_to_string(dbColumn *column, dbString *string)
{
    int sqltype;
    dbValue *value;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_default_value(column);
    return db_convert_value_to_string(value, sqltype, string);
}
