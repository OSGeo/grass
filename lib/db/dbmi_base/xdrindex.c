/*!
  \file lib/db/dbmi_base/xdrindex.c
  
  \brief DBMI Library (base) - external data representation (index)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>
#include "macros.h"


/*!
  \brief Send index

  \param index

  \return
*/
int db__send_index(dbIndex * index)
{
    int i;

    DB_SEND_STRING(&index->indexName);
    DB_SEND_STRING(&index->tableName);
    DB_SEND_CHAR(index->unique);

    DB_SEND_INT(index->numColumns);

    for (i = 0; i < index->numColumns; i++) {
	DB_SEND_STRING(&index->columnNames[i]);
    }

    return DB_OK;
}

/*!
  \brief Send index array

  \param list
  \param count

  \return
*/
int db__send_index_array(dbIndex * list, int count)
{
    int i;

    DB_SEND_INT(count);
    for (i = 0; i < count; i++) {
	DB_SEND_INDEX(&list[i]);
    }
    return DB_OK;
}

/*!
  \brief Receive index

  \param index

  \return
*/
int db__recv_index(dbIndex * index)
{
    int i, ncols;

    db_init_index(index);
    DB_RECV_STRING(&index->indexName);
    DB_RECV_STRING(&index->tableName);
    DB_RECV_CHAR(&index->unique);

    DB_RECV_INT(&ncols);

    if (db_alloc_index_columns(index, ncols) != DB_OK)
	return db_get_error_code();

    for (i = 0; i < ncols; i++) {
	DB_RECV_STRING(&index->columnNames[i]);
    }

    return DB_OK;
}

/*!
  \brief Receive index array

  \param list
  \param count
*/
int db__recv_index_array(dbIndex ** list, int *count)
{
    int i;

    DB_RECV_INT(count);

    *list = db_alloc_index_array(*count);
    if (*list == NULL)
	return db_get_error_code();

    for (i = 0; i < *count; i++) {
	DB_RECV_INDEX(&((*list)[i]));
    }

    return DB_OK;
}
