#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
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
	free(table);
	return (table = NULL);
    }
    table->numColumns = ncols;
    for (i = 0; i < ncols; i++)
	db_init_column(&table->columns[i]);

    return table;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_init_table(dbTable * table)
{
    db_zero((void *)table, sizeof(dbTable));
    db_init_string(&table->tableName);
    db_init_string(&table->description);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_free_table(dbTable * table)
{
    int i;

    db_free_string(&table->tableName);
    for (i = 0; i < table->numColumns; i++)
	db_free_column(&table->columns[i]);
    if (table->columns)
	free(table->columns);
    free(table);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_table_name(dbTable * table, const char *name)
{
    return db_set_string(&table->tableName, name);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_table_name(dbTable * table)
{
    return db_get_string(&table->tableName);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_table_description(dbTable * table, const char *description)
{
    return db_set_string(&table->description, description);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_table_description(dbTable * table)
{
    return db_get_string(&table->description);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_table_number_of_columns(dbTable * table)
{
    return table->numColumns;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
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

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
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
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_select_priv_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_select_priv_granted);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_select_priv_not_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_select_priv_not_granted);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_table_select_priv(dbTable * table)
{
    return get_all_column_privs(table, db_get_column_select_priv);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_update_priv_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_update_priv_granted);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_update_priv_not_granted(dbTable * table)
{
    set_all_column_privs(table, db_set_column_update_priv_not_granted);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_table_update_priv(dbTable * table)
{
    return get_all_column_privs(table, db_get_column_update_priv);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_insert_priv_granted(dbTable * table)
{
    table->priv_insert = DB_GRANTED;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_insert_priv_not_granted(dbTable * table)
{
    table->priv_insert = DB_NOT_GRANTED;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_table_insert_priv(dbTable * table)
{
    return table->priv_insert;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_delete_priv_granted(dbTable * table)
{
    table->priv_delete = DB_GRANTED;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_set_table_delete_priv_not_granted(dbTable * table)
{
    table->priv_delete = DB_NOT_GRANTED;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_get_table_delete_priv(dbTable * table)
{
    return table->priv_delete;
}

/*!
   \brief Create SQL CREATE sring from table definition
   \return 
   \param 
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
