/*!
  \file lib/db/dbmi_base/xdrcolumn.c
  
  \brief DBMI Library (base) - external data representation (column)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Send column definition
  
  \param column pointer to dbColumn

  \return DB_OK
*/
int db__send_column_definition(dbColumn * column)
{
    DB_SEND_STRING(&column->columnName);
    DB_SEND_STRING(&column->description);
    DB_SEND_INT(column->sqlDataType);
    DB_SEND_INT(column->hostDataType);
    DB_SEND_INT(column->precision);
    DB_SEND_INT(column->scale);
    DB_SEND_INT(column->dataLen);
    DB_SEND_INT(column->select);
    DB_SEND_INT(column->update);
    DB_SEND_CHAR(column->nullAllowed);
    DB_SEND_CHAR(column->useDefaultValue);
    DB_SEND_CHAR(column->hasDefaultValue);
    if (column->hasDefaultValue) {
	DB_SEND_COLUMN_DEFAULT_VALUE(column);
    }

    return DB_OK;
}

/*!
  \brief Receive column definition

  \param column pointer to dbColumn

  \return DB_OK
*/
int db__recv_column_definition(dbColumn * column)
{
    DB_RECV_STRING(&column->columnName);
    DB_RECV_STRING(&column->description);
    DB_RECV_INT(&column->sqlDataType);
    DB_RECV_INT(&column->hostDataType);
    DB_RECV_INT(&column->precision);
    DB_RECV_INT(&column->scale);
    DB_RECV_INT(&column->dataLen);
    DB_RECV_INT(&column->select);
    DB_RECV_INT(&column->update);
    DB_RECV_CHAR(&column->nullAllowed);
    DB_RECV_CHAR(&column->useDefaultValue);
    DB_RECV_CHAR(&column->hasDefaultValue);
    if (column->hasDefaultValue) {
	DB_RECV_COLUMN_DEFAULT_VALUE(column);
    }

    return DB_OK;
}

/*!
  \brief Send column value

  \param column pointer to dbColumn

  \return
*/
int db__send_column_value(dbColumn * column)
{
    return db__send_value(db_get_column_value(column),
			  db_sqltype_to_Ctype(db_get_column_sqltype(column)));
}

/*!
  \brief Receive column value

  \param column pointer to dbColumn

  \return
*/
int db__recv_column_value(dbColumn * column)
{
    return db__recv_value(db_get_column_value(column),
			  db_sqltype_to_Ctype(db_get_column_sqltype(column)));
}

/*!
  \brief Send column default value

  \param column pointer to dbColumn

  \return
*/
int db__send_column_default_value(dbColumn * column)
{
    return db__send_value(db_get_column_default_value(column),
			  db_sqltype_to_Ctype(db_get_column_sqltype(column)));
}

/*!
  \brief Receive column default value

  \param column pointer to dbColumn

  \return
*/
int db__recv_column_default_value(dbColumn * column)
{
    return db__recv_value(db_get_column_default_value(column),
			  db_sqltype_to_Ctype(db_get_column_sqltype(column)));
}
