/*!
  \file lib/db/dbmi_base/table.c
  
  \brief DBMI Library (base) - table management
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
  \brief Allocate a table with a specific number of columns
  
  \param ncols number of columns which should be allocated
  
  \return allocated dbTable
  \return NULL in case of an error
 */
dbTable *db_alloc_table(int ncols)
{
    dbTable *table;
    int i;

    table = (dbTable *) db_malloc(sizeof(dbTable));
    if (table == NULL)
	return (table = NULL);

    db_init_table(table);

    table->columns = (dbColumn *) db_calloc(sizeof(dbColumn), ncols);
    if (table->columns == NULL) {
	db_free(table);
	return (table = NULL);
    }
    table->numColumns = ncols;
    for (i = 0; i < ncols; i++)
	db_init_column(&table->columns[i]);

    return table;
}

/*!
   \brief Initialize the table to zero
   
   \param table pointer to dbTable
*/
void db_init_table(dbTable * table)
{
    db_zero((void *)table, sizeof(dbTable));
    db_init_string(&table->tableName);
    db_init_string(&table->description);
}

/*!
  \brief Free the table
  
  \param table pointer to dbTable
*/
void db_free_table(dbTable * table)
{
    int i;

    db_free_string(&table->tableName);
    db_free_string(&table->description);
    for (i = 0; i < table->numColumns; i++)
	db_free_column(&table->columns[i]);
    if (table->columns)
	db_free(table->columns);
    db_free(table);
}

/*!
   \brief Set the name of the table

   \param table pointer to dbTable
   \param name The name of the table

   \return DB_OK on success
*/
int db_set_table_name(dbTable * table, const char *name)
{
    return db_set_string(&table->tableName, name);
}

/*!
  \brief Get the name of the table

  \param table pointer to dbTable

  \return name of the table
*/
const char *db_get_table_name(dbTable * table)
{
    return db_get_string(&table->tableName);
}

/*!
  \brief Set the description of the table
  
  \param table pointer to dbTable
  \param name description of the table
  
  \return DB_OK
 */
int db_set_table_description(dbTable * table, const char *description)
{
    return db_set_string(&table->description, description);
}

/*!
  \brief Get the description of the table

  \param table pointer to dbTable
  
  \return description of the table
*/
const char *db_get_table_description(dbTable * table)
{
    return db_get_string(&table->description);
}

/*!
  \brief Return the number of columns of the table
  
  \param table pointer to dbTable
  
  \return number of columns
*/
int db_get_table_number_of_columns(dbTable * table)
{
    return table->numColumns;
}

static void set_all_column_privs(dbTable * table, void (*set_column_priv) ())
{
    int col, ncols;
    dbColumn *column;

    ncols = db_get_table_number_of_columns(table);
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	set_column_priv(column);
    }
}

static int get_all_column_privs(dbTable * table, int (*get_column_priv) ())
{
    int priv, col, ncols;
    dbColumn *column;

    ncols = db_get_table_number_of_columns(table);
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	priv = get_column_priv(column);
	if (priv != DB_GRANTED)
	    return priv;
    }
    return DB_GRANTED;
}

/*!
  \brief Grant selection privileges for all columns
  
  \param table pointer to dbTable
*/
void db_set_table_select_priv_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_select_priv_granted);
}

/*!
  \brief Set selection privileges not granted for all columns
  
  \param table pointer to dbTable
*/
void db_set_table_select_priv_not_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_select_priv_not_granted);
}

/*!
  \brief Get table select privileges

  \param table pointer to dbTable
 
  \return privilages
*/
int db_get_table_select_priv(dbTable * table)
{
    return get_all_column_privs(table, db_get_column_select_priv);
}

/*!
  \brief Grant update privileges for all columns

  \param table pointer to dbTable
*/
void db_set_table_update_priv_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_update_priv_granted);
}

/*!
  \brief Set update privileges not granted for all columns
  
  \param table pointer to dbTable
*/
void db_set_table_update_priv_not_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_update_priv_not_granted);
}

/*!
  \brief Get table update privileges

  \param table pointer to dbTable

  \return privilages
*/
int db_get_table_update_priv(dbTable * table)
{
    return get_all_column_privs(table, db_get_column_update_priv);
}

/*!
  \brief Grant insert privileges for table
  
  \param table pointer to dbTable
*/
void db_set_table_insert_priv_granted(dbTable * table)
{
    table->priv_insert = DB_GRANTED;
}

/*!
   \brief Set insert privileges not granted for table
   
   \param table pointer to dbTable
 */
void db_set_table_insert_priv_not_granted(dbTable * table)
{
    table->priv_insert = DB_NOT_GRANTED;
}

/*!
  \brief Get table insert privileges

  \param table pointer to dbTable

  \return prilileges
*/
int db_get_table_insert_priv(dbTable * table)
{
    return table->priv_insert;
}

/*!
  \brief Grant delete privileges for table
  
  \param table pointer to dbTable
 */
void db_set_table_delete_priv_granted(dbTable * table)
{
    table->priv_delete = DB_GRANTED;
}

/*!
  \brief Set delete privileges not granted for table
  
  \param table pointer to dbTable
*/
void db_set_table_delete_priv_not_granted(dbTable * table)
{
    table->priv_delete = DB_NOT_GRANTED;
}

/*!
  \brief Get table delete privileges

  \param table pointer to dbTable

  \return privileges
*/
int db_get_table_delete_priv(dbTable * table)
{
    return table->priv_delete;
}

/*!
  \brief Returns column structure for given table and column number

  \param table pointer to dbTable
  \param idx     column index (starting with '0')

  \return pointer to dbColumn
  \return NULL if not found
*/
dbColumn *db_get_table_column(dbTable * table, int idx)
{
    if (idx < 0 || idx >= table->numColumns)
	return ((dbColumn *) NULL);
    return &table->columns[idx];
}

/*!
  \brief Returns column structure for given table and column name

  \param table pointer to dbTable
  \param name the name of the column

  \return pointer to dbColumn
  \return NULL if not found
*/
dbColumn *db_get_table_column_by_name(dbTable * table, const char* name)
{
    dbColumn *c = NULL;
    int i, columns = table->numColumns;

    for(i = 0; i < columns; i++ ) {
        c = db_get_table_column(table, i);

        if(c == NULL)
            return c;

        if(strcmp(name, db_get_string(&c->columnName)) == 0)
            break;

        c = NULL;
    }

    return c;
}

/*!
  \brief Set a specific column for given table and column number
  
  \param table Pointer to dbTable
  \param idx Column index (starting with '0').  The index must be in range.
  \param column Pointer to a dbColumn to insert.
  A copy of the column stored, so the original column can be deleted.
  
  \return DB_OK on success
  \return DB_FAILURE on error
*/
int db_set_table_column(dbTable * table, int idx, dbColumn *column)
{
    if (idx < 0 || idx >= table->numColumns)
	return DB_FAILED;
    db_copy_column(&table->columns[idx], column);
    return DB_OK;
}

/*!
  \brief Append a specific column to given table
  
  \param table Pointer to dbTable
  \param column Pointer to a dbColumn to append.
  A copy of the column is stored, so the original column can be deleted.
  
  \return DB_OK on success
  \return DB_FAILURE on error
*/
int db_append_table_column(dbTable * table, dbColumn *column)
{
    table->columns = (dbColumn*)db_realloc((void*)table->columns, sizeof(dbColumn)*(table->numColumns + 1));
    if(table->columns == NULL)
        return DB_FAILED;
    db_copy_column(&table->columns[table->numColumns], column);
    table->numColumns++;
    return DB_OK;
}

/*!
  \brief Make a new exact copy of an existing table
  
  New memory is allocated for the clone, the columns-content will be copied too.
  
  \param src Pointer to dbTable
  
  \return A new alloacted clone of the given table on success 
  \return NULL on error
*/
dbTable *db_clone_table(dbTable *src)
{
    int i, n = db_get_table_number_of_columns(src);
    dbTable *new = db_alloc_table(n);
    if(new == NULL)
        return (new = NULL);

    db_copy_string(&new->description, &src->description);
    db_copy_string(&new->tableName, &src->tableName);

    /* Deep copy the columns */
    for(i = 0; i < n; i++)
    {
        db_copy_column(&new->columns[i], &src->columns[i]);
    }

    new->numColumns = n;
    new->priv_delete = src->priv_delete;
    new->priv_insert = src->priv_insert;

    return new;
}

/*!
   \brief Create SQL CREATE sring from table definition
 
   \param table pointer to dbTable
   \param sql dbString to store the SQL CREATE string

   \return DB_OK on success
   \return DB_FAILED on error
*/
int db_table_to_sql(dbTable * table, dbString * sql)
{
    int col, ncols;
    dbColumn *column;
    const char *colname;
    int sqltype, ctype;
    char buf[500];

    db_set_string(sql, "create table ");
    db_append_string(sql, db_get_table_name(table));
    db_append_string(sql, " ( ");

    ncols = db_get_table_number_of_columns(table);

    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	colname = db_get_column_name(column);
	sqltype = db_get_column_sqltype(column);

	ctype = db_sqltype_to_Ctype(sqltype);
	G_debug(3, "%s (%s)", colname, db_sqltype_name(sqltype));

	if (col > 0)
	    db_append_string(sql, ", ");
	db_append_string(sql, colname);
	db_append_string(sql, " ");
	/* Note: I found on Web:
	 *  These are the ANSI data types: BIT, CHARACTER, DATE, DECIMAL, DOUBLE PRECISION, FLOAT, 
	 *  INTEGER, INTERVAL, NUMERIC, REAL, SMALLINT, TIMESTAMP, TIME, VARBIT, VARCHAR, CHAR
	 *  ...
	 *  Thus, the only data types you can use with the assurance that they will 
	 *  work everywhere are as follows:
	 *  DOUBLE PRECISION, FLOAT, INTEGER, NUMERIC, REAL, SMALLINT, VARCHAR, CHAR */
	switch (sqltype) {
	case DB_SQL_TYPE_CHARACTER:
	    sprintf(buf, "varchar(%d)", db_get_column_length(column));
	    db_append_string(sql, buf);
	    break;
	case DB_SQL_TYPE_TEXT:
	    G_warning("Type TEXT converted to 'VARCHAR(250)'");
	    db_append_string(sql, "varchar(250)");
	    break;
	case DB_SQL_TYPE_SMALLINT:
	case DB_SQL_TYPE_INTEGER:
	    db_append_string(sql, "integer");
	    break;
	case DB_SQL_TYPE_REAL:
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	case DB_SQL_TYPE_DECIMAL:
	case DB_SQL_TYPE_NUMERIC:
	case DB_SQL_TYPE_INTERVAL:
	    db_append_string(sql, "double precision");
	    break;
	case DB_SQL_TYPE_DATE:
	    db_append_string(sql, "date");
	    break;
	case DB_SQL_TYPE_TIME:
	    db_append_string(sql, "time");
	    break;
	case DB_SQL_TYPE_TIMESTAMP:
	    db_append_string(sql, "datetime");
	    break;
	default:
	    G_warning("Unknown column type (%s)", colname);
	    return DB_FAILED;
	}
    }
    db_append_string(sql, " )");
    G_debug(3, "sql statement: %s", db_get_string(sql));

    return DB_OK;
}
