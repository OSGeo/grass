/*!
  \file lib/db/dbmi_base/index.c
  
  \brief DBMI Library (base) - index management
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \brief Initialize dbIndex

  \param index pointer to dbIndex to be initialized
*/
void db_init_index(dbIndex * index)
{
    db_init_string(&index->indexName);
    db_init_string(&index->tableName);
    index->numColumns = 0;
    index->columnNames = NULL;
    index->unique = 0;
}

/*!
  \brief Free allocated dbIndex

  \param index pointer to dbIndex to be freed
*/
void db_free_index(dbIndex * index)
{
    db_free_string(&index->indexName);
    db_free_string(&index->tableName);
    if (index->numColumns > 0)
	db_free_string_array(index->columnNames, index->numColumns);
    db_init_index(index);
}

/*!
  \brief Allocate index columns

  \param index pointer to dbIndex
  \param ncols number of columns to be allocated

  \return DB_OK
*/
int db_alloc_index_columns(dbIndex * index, int ncols)
{
    index->columnNames = db_alloc_string_array(ncols);
    if (index->columnNames == NULL)
	return db_get_error_code();
    index->numColumns = ncols;

    return DB_OK;
}

/*!
  \brief Allocate index array

  \param count number of items

  \return pointer to allocated dbIndex array
*/
dbIndex *db_alloc_index_array(int count)
{
    dbIndex *list;
    int i;

    list = (dbIndex *) db_calloc(count, sizeof(dbIndex));
    if (list) {
	for (i = 0; i < count; i++)
	    db_init_index(&list[i]);
    }
    return list;
}

/*!
  \brief Free index array

  \param list dbIndex array
  \param count number of items in the array
*/
void db_free_index_array(dbIndex * list, int count)
{
    int i;

    if (list) {
	for (i = 0; i < count; i++)
	    db_free_index(&list[i]);
	db_free(list);
    }
}

/*!
  \brief Set index name

  \param index pointer to dbIndex
  \param name name to be set

  \return DB_OK on success
  \return DB_FAILED on error
*/
int db_set_index_name(dbIndex * index, const char *name)
{
    return db_set_string(&index->indexName, name);
}

/*!
  \brief Get index name

  \param index pointer to dbIndex
  
  \return string buffer with name
*/
const char *db_get_index_name(dbIndex * index)
{
    return db_get_string(&index->indexName);
}

/*!
  \brief Set table name

  \param index pointer to dbIndex
  \param name name to be set

  \return DB_OK on success
  \return DB_FAILED on error
 */
int db_set_index_table_name(dbIndex * index, const char *name)
{
    return db_set_string(&index->tableName, name);
}

/*!
  \brief Get table name

  \param index pointer to dbIndex
  
  \return string buffer with name
*/
const char *db_get_index_table_name(dbIndex * index)
{
    return db_get_string(&index->tableName);
}

/*!
  \brief Get number of columns

  \param index pointer to dbIndex

  \return number of columns
*/
int db_get_index_number_of_columns(dbIndex * index)
{
    return index->numColumns;
}

/*!
  \brief Set column name

  \param index pointer to dbIndex
  \param column_num column number
  \param name name to be set

  \return DB_OK on success
  \return DB_FAILED on error
*/
int db_set_index_column_name(dbIndex * index, int column_num, const char *name)
{
    if (column_num < 0 || column_num >= index->numColumns) {
	db_error(_("db_set_index_column_name(): invalid column number"));
	return db_get_error_code();
    }
    return db_set_string(&index->columnNames[column_num], name);
}

/*!
  \brief Get column number

  \param index pointer to dbIndex
  \param column_num column number

  \return string buffer with name
 */
const char *db_get_index_column_name(dbIndex * index, int column_num)
{
    if (column_num < 0 || column_num >= index->numColumns) {
	db_error(_("db_get_index_column_name(): invalid column number"));
	return ((const char *)NULL);
    }
    return db_get_string(&index->columnNames[column_num]);
}

/*!
  \brief Set index type to unique

  \todo return type void?

  \param index pointer to dbIndex

  \return 0
*/
int db_set_index_type_unique(dbIndex * index)
{
    index->unique = 1;

    return 0;
}

/*!
  \brief Set index type to non-unique
  
  \todo return type void?

  \param index pointer to dbIndex

  \return 0
*/
int db_set_index_type_non_unique(dbIndex * index)
{
    index->unique = 0;

    return 0;
}

/*!
  \brief Test if type is unique

  \param index pointer to dbIndex

  \return non-zero if True
  \return zero if False
 */
int db_test_index_type_unique(dbIndex * index)
{
    return index->unique != 0;
}

/*!
  \brief Report index 

  \param fd file where to print index info
  \param index pointer to dbIndex
*/
void db_print_index(FILE * fd, dbIndex * index)
{
    int i, nCols;

    fprintf(fd, "Name: %s\n", db_get_index_name(index));
    if (db_test_index_type_unique(index))
	fprintf(fd, "Unique: true\n");
    else
	fprintf(fd, "Unique: false\n");
    fprintf(fd, "Table: %s\n", db_get_index_table_name(index));
    nCols = db_get_index_number_of_columns(index);
    fprintf(fd, "Number of columns: %d\nColumns:\n", nCols);

    for (i = 0; i < nCols; i++) {
	fprintf(fd, "  %s\n", db_get_index_column_name(index, i));
    }
}
