#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn void db_init_cursor (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_init_cursor(dbCursor * cursor)
{
    cursor->driver = NULL;
    cursor->token = -1;
    cursor->type = 0;
    cursor->mode = 0;
    cursor->table = NULL;
    cursor->column_flags = NULL;
}

/*!
   \fn int db_alloc_cursor_table (dbCursor *cursor, int ncols)
   \brief 
   \return 
   \param 
 */
int db_alloc_cursor_table(dbCursor * cursor, int ncols)
{
    cursor->table = db_alloc_table(ncols);
    if (cursor->table == NULL)
	return db_get_error_code();
    return DB_OK;
}

/*!
   \fn void db_free_cursor (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_free_cursor(dbCursor * cursor)
{
    if (cursor->table)
	db_free_table(cursor->table);
    if (cursor->column_flags)
	db_free_cursor_column_flags(cursor);
    db_init_cursor(cursor);
}

/*!
   \fn dbTable *db_get_cursor_table (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
dbTable *db_get_cursor_table(dbCursor * cursor)
{
    return cursor->table;
}

/*!
   \fn void db_set_cursor_table (dbCursor *cursor, dbTable *table)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_table(dbCursor * cursor, dbTable * table)
{
    cursor->table = table;
}

/*!
   \fn dbToken db_get_cursor_token (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
dbToken db_get_cursor_token(dbCursor * cursor)
{
    return cursor->token;
}

/*!
   \fn void db_set_cursor_token (dbCursor *cursor, dbToken token)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_token(dbCursor * cursor, dbToken token)
{
    cursor->token = token;
}

/*!
   \fn void db_set_cursor_type_readonly (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_type_readonly(dbCursor * cursor)
{
    cursor->type = DB_READONLY;
}

/*!
   \fn void db_set_cursor_type_update (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_type_update(dbCursor * cursor)
{
    cursor->type = DB_UPDATE;
}

/*!
   \fn void db_set_cursor_type_insert (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_type_insert(dbCursor * cursor)
{
    cursor->type = DB_INSERT;
}

/*!
   \fn int db_test_cursor_type_fetch (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_type_fetch(dbCursor * cursor)
{
    return (cursor->type == DB_READONLY || cursor->type == DB_UPDATE);
}

/*!
   \fn int db_test_cursor_type_update (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_type_update(dbCursor * cursor)
{
    return (cursor->type == DB_UPDATE);
}

/*!
   \fn int db_test_cursor_type_insert (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_type_insert(dbCursor * cursor)
{
    return (cursor->type == DB_INSERT);
}

/*!
   \fn void db_set_cursor_mode (dbCursor *cursor, int mode)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_mode(dbCursor * cursor, int mode)
{
    cursor->mode = mode;
}

/*!
   \fn void db_set_cursor_mode_scroll (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_mode_scroll(dbCursor * cursor)
{
    cursor->mode |= DB_SCROLL;
}

/*!
   \fn void db_unset_cursor_mode_scroll (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_unset_cursor_mode_scroll(dbCursor * cursor)
{
    cursor->mode &= ~DB_SCROLL;
}

/*!
   \fn void db_unset_cursor_mode (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_unset_cursor_mode(dbCursor * cursor)
{
    cursor->mode = 0;
}

/*!
   \fn void db_set_cursor_mode_insensitive (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_mode_insensitive(dbCursor * cursor)
{
    cursor->mode |= DB_INSENSITIVE;
}

/*!
   \fn void db_unset_cursor_mode_insensitive (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_unset_cursor_mode_insensitive(dbCursor * cursor)
{
    cursor->mode &= ~DB_INSENSITIVE;
}

/*!
   \fn int db_test_cursor_mode_scroll (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_mode_scroll(dbCursor * cursor)
{
    return (cursor->mode & DB_SCROLL);
}


/*!
   \fn int db_test_cursor_mode_insensitive (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_mode_insensitive(dbCursor * cursor)
{
    return (cursor->mode & DB_INSENSITIVE);
}

/*!
   \fn int db_alloc_cursor_column_flags (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_alloc_cursor_column_flags(dbCursor * cursor)
{
    int ncols;
    int col;

    ncols = db_get_cursor_number_of_columns(cursor);
    cursor->column_flags = (short *)db_calloc(ncols, sizeof(short));
    if (cursor->column_flags == NULL)
	return db_get_error_code();
    for (col = 0; col < ncols; col++)
	db_unset_cursor_column_flag(cursor, col);
    return DB_OK;
}

/*!
   \fn void db_free_cursor_column_flags (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
void db_free_cursor_column_flags(dbCursor * cursor)
{
    if (cursor->column_flags)
	free(cursor->column_flags);
    cursor->column_flags = NULL;
}

/*!
   \fn void db_set_cursor_column_for_update (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_column_for_update(dbCursor * cursor, int col)
{
    db_set_cursor_column_flag(cursor, col);
}

/*!
   \fn void db_unset_cursor_column_for_update (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
void db_unset_cursor_column_for_update(dbCursor * cursor, int col)
{
    db_unset_cursor_column_flag(cursor, col);
}

/*!
   \fn int db_test_cursor_column_for_update (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_column_for_update(dbCursor * cursor, int col)
{
    return db_test_cursor_column_flag(cursor, col);
}

/*!
   \fn int db_test_cursor_any_column_for_update (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_any_column_for_update(dbCursor * cursor)
{
    return db_test_cursor_any_column_flag(cursor);
}

/*!
   \fn void db_set_cursor_column_flag (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
void db_set_cursor_column_flag(dbCursor * cursor, int col)
{
    if (cursor->column_flags)
	cursor->column_flags[col] = 1;
}

/*!
   \fn void db_unset_cursor_column_flag (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
void db_unset_cursor_column_flag(dbCursor * cursor, int col)
{
    if (cursor->column_flags)
	cursor->column_flags[col] = 0;
}

/*!
   \fn int db_test_cursor_column_flag (dbCursor *cursor, int col)
   \brief 
   \return 
   \param 
 */
int db_test_cursor_column_flag(dbCursor * cursor, int col)
{
    return cursor->column_flags && cursor->column_flags[col] ? 1 : 0;
}

/*!
   \fn int db_get_cursor_number_of_columns (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
int db_get_cursor_number_of_columns(dbCursor * cursor)
{
    dbTable *table;

    table = db_get_cursor_table(cursor);
    if (table)
	return db_get_table_number_of_columns(table);
    return 0;
}

/*!
   \fn int db_test_cursor_any_column_flag (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */
/* is any cursor column flag set? */
int db_test_cursor_any_column_flag(dbCursor * cursor)
{
    int ncols, col;

    ncols = db_get_cursor_number_of_columns(cursor);
    for (col = 0; col < ncols; col++)
	if (db_test_cursor_column_flag(cursor, col))
	    return 1;
    return 0;
}
