#include <grass/dbmi.h>
#include "macros.h"


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

int db__send_index_array(dbIndex * list, int count)
{
    int i;

    DB_SEND_INT(count);
    for (i = 0; i < count; i++) {
	DB_SEND_INDEX(&list[i]);
    }
    return DB_OK;
}

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
