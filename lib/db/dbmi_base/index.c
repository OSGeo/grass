#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_index_name(dbIndex * index, const char *name)
{
    return db_set_string(&index->indexName, name);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_index_name(dbIndex * index)
{
    return db_get_string(&index->indexName);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_index_table_name(dbIndex * index, const char *name)
{
    return db_set_string(&index->tableName, name);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_index_table_name(dbIndex * index)
{
    return db_get_string(&index->tableName);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_index_number_of_columns(dbIndex * index)
{
    return index->numColumns;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int
db_set_index_column_name(dbIndex * index, int column_num, const char *name)
{
    if (column_num < 0 || column_num >= index->numColumns) {
	db_error("db_set_index_column_name(): invalid column number");
	return db_get_error_code();
    }
    return db_set_string(&index->columnNames[column_num], name);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_index_column_name(dbIndex * index, int column_num)
{
    if (column_num < 0 || column_num >= index->numColumns) {
	db_error("db_get_index_column_name(): invalid column number");
	return ((const char *)NULL);
    }
    return db_get_string(&index->columnNames[column_num]);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_index_type_unique(dbIndex * index)
{
    index->unique = 1;

    return 0;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_index_type_non_unique(dbIndex * index)
{
    index->unique = 0;

    return 0;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_test_index_type_unique(dbIndex * index)
{
    return index->unique != 0;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
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
