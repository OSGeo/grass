#include <grass/dbmi.h>
#include "macros.h"


int db__send_table_definition(dbTable * table)
{
    int i;

    DB_SEND_INT(table->numColumns);

    for (i = 0; i < table->numColumns; i++) {
	DB_SEND_COLUMN_DEFINITION(&table->columns[i]);
    }
    DB_SEND_STRING(&table->tableName);
    DB_SEND_STRING(&table->description);

    DB_SEND_INT(table->priv_insert);
    DB_SEND_INT(table->priv_delete);

    return DB_OK;
}

int db__recv_table_definition(dbTable ** table)
{
    int i, ncols;
    dbTable *t;

    DB_RECV_INT(&ncols);

    *table = t = db_alloc_table(ncols);
    if (t == NULL)
	return db_get_error_code();

    for (i = 0; i < t->numColumns; i++) {
	DB_RECV_COLUMN_DEFINITION(&t->columns[i]);
    }
    DB_RECV_STRING(&t->tableName);
    DB_RECV_STRING(&t->description);

    DB_RECV_INT(&t->priv_insert);
    DB_RECV_INT(&t->priv_delete);

    return DB_OK;
}

int db__send_table_data(dbTable * table)
{
    int i, ncols;

    ncols = table->numColumns;
    DB_SEND_INT(ncols);
    for (i = 0; i < ncols; i++) {
	DB_SEND_COLUMN_VALUE(db_get_table_column(table, i));
    }

    return DB_OK;
}

int db__recv_table_data(dbTable * table)
{
    int i, ncols;

    ncols = table->numColumns;
    DB_RECV_INT(&i);

    if (i != ncols) {
	db_error("fetch: table has wrong number of columns");
	return DB_FAILED;
    }
    for (i = 0; i < ncols; i++) {
	DB_RECV_COLUMN_VALUE(db_get_table_column(table, i));
    }

    return DB_OK;
}
