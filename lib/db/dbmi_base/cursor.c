/*!
  \file lib/db/dbmi_base/cursor.c
  
  \brief DBMI Library (base) - cursors management
  
  (C) 1999-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
*/

#include <stdlib.h>
#include <grass/dbmi.h>

/*!
  \brief Initialize dbCursor
  
  \param cursor pointer to dbCursor to be initialized
*/
void db_init_cursor(dbCursor *cursor)
{
    G_zero(cursor, sizeof(dbCursor));
    cursor->token = -1;
}

/*!
   \brief Allocate table for cursor

   \param cursor pointer to dbCursor
   \param ncols  number of column in table

   \return DB_OK on success
   \return error code on error
 */
int db_alloc_cursor_table(dbCursor *cursor, int ncols)
{
    cursor->table = db_alloc_table(ncols);
    if (cursor->table == NULL)
	return db_get_error_code();
    return DB_OK;
}

/*!
   \brief Free allocated dbCursor

   \param cursor pointer to dbCursor
*/
void db_free_cursor(dbCursor *cursor)
{
    if (cursor->table)
	db_free_table(cursor->table);
    if (cursor->column_flags)
	db_free_cursor_column_flags(cursor);
    db_init_cursor(cursor);
}

/*!
   \brief Get table allocated by cursor

   \param cursor pointer to dbCursor
   
   \return pointer to dbTable
*/
dbTable *db_get_cursor_table(dbCursor *cursor)
{
    return cursor->table;
}

/*!
  \brief Set table for given cursor

  \param cursor pointer to dbCursor
  \param table  pointer to dbTable
*/
void db_set_cursor_table(dbCursor *cursor, dbTable *table)
{
    cursor->table = table;
}

/*!
  \brief Get cursor token

  \param cursor pointer to dbCursor

  \return pointer to dbToken
*/
dbToken db_get_cursor_token(dbCursor *cursor)
{
    return cursor->token;
}

/*!
  \brief Set cursor token

  \param cursor pointer to dbCursor
  \param token  pointer to dbToken
*/
void db_set_cursor_token(dbCursor *cursor, dbToken token)
{
    cursor->token = token;
}

/*!
   \brief Set cursor to be read-only (select)

   \param cursor pointer to dbCursor
*/
void db_set_cursor_type_readonly(dbCursor *cursor)
{
    cursor->type = DB_READONLY;
}

/*!
   \brief Set cursor to be writable (update)

   \param cursor pointer to dbCursor
*/
void db_set_cursor_type_update(dbCursor *cursor)
{
    cursor->type = DB_UPDATE;
}

/*!
   \brief Set cursor to be writable (insert)

   \param cursor pointer to dbCursor
*/
void db_set_cursor_type_insert(dbCursor *cursor)
{
    cursor->type = DB_INSERT;
}

/*!
  \brief Check cursor type

  \param cursor pointer to dbCursor

  \return 1 for known cursor type
  \return 0 for unknown cursor type
*/
int db_test_cursor_type_fetch(dbCursor *cursor)
{
    return (cursor->type == DB_READONLY ||
	    cursor->type == DB_UPDATE ||
	    cursor->type == DB_INSERT);
}

/*!
  \brief Check if cursor type is 'update'

  \param cursor pointer to dbCursor

  \return 1 if cursor type is 'update'
  \return 0 otherwise
*/
int db_test_cursor_type_update(dbCursor *cursor)
{
    return (cursor->type == DB_UPDATE);
}

/*!
  \brief Check if cursor type is 'insert'

  \param cursor pointer to dbCursor

  \return 1 if cursor type is 'insert'
  \return 0 otherwise
*/
int db_test_cursor_type_insert(dbCursor *cursor)
{
    return (cursor->type == DB_INSERT);
}

/*!
  \brief Set cursor mode

  Modes:
   - DB_SCROLL
   - DB_INSENSITIVE

  \param cursor pointer to dbCursor
  \param mode cursor mode
 */
void db_set_cursor_mode(dbCursor *cursor, int mode)
{
    cursor->mode = mode;
}

/*!
  \brief Set 'scroll' cursor mode

  \param cursor pointer to dbCursor
*/
void db_set_cursor_mode_scroll(dbCursor *cursor)
{
    cursor->mode |= DB_SCROLL;
}

/*!
  \brief Unset 'scroll' cursor mode

  \param cursor pointer to dbCursor
*/
void db_unset_cursor_mode_scroll(dbCursor *cursor)
{
    cursor->mode &= ~DB_SCROLL;
}

/*!
  \brief Unset cursor mode

  \param cursor pointer to dbCursor
*/
void db_unset_cursor_mode(dbCursor *cursor)
{
    cursor->mode = 0;
}

/*!
  \brief Set 'intensive' cursor mode

  \param cursor pointer to dbCursor
*/
void db_set_cursor_mode_insensitive(dbCursor *cursor)
{
    cursor->mode |= DB_INSENSITIVE;
}

/*!
  \brief Unset 'intensive' cursor mode

  \param cursor pointer to dbCursor
*/
void db_unset_cursor_mode_insensitive(dbCursor *cursor)
{
    cursor->mode &= ~DB_INSENSITIVE;
}

/*!
  \brief Check if cursor mode is 'scroll'

  \param cursor pointer to dbCursor

  \return 1 if true
  \return 0 if false
*/
int db_test_cursor_mode_scroll(dbCursor *cursor)
{
    return (cursor->mode & DB_SCROLL);
}

/*!
  \brief Check if cursor mode is 'intensive'

  \param cursor pointer to dbCursor

  \return 1 if true
  \return 0 if false
*/
int db_test_cursor_mode_insensitive(dbCursor *cursor)
{
    return (cursor->mode & DB_INSENSITIVE);
}

/*!
  \brief Allocate columns' flags for cursor

  \param cursor pointer to dbCursor

  \return DB_OK on success
  \return error code on failure
*/
int db_alloc_cursor_column_flags(dbCursor *cursor)
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
  \brief Free columns' flags of cursor

  \param cursor pointer to dbCursor
*/
void db_free_cursor_column_flags(dbCursor *cursor)
{
    if (cursor->column_flags)
	db_free(cursor->column_flags);
    cursor->column_flags = NULL;
}

/*!
  \brief Set Column flag to 'update'

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')
*/
void db_set_cursor_column_for_update(dbCursor *cursor, int col)
{
    db_set_cursor_column_flag(cursor, col);
}

/*!
  \brief Unset 'update' column flag

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')
*/
void db_unset_cursor_column_for_update(dbCursor *cursor, int col)
{
    db_unset_cursor_column_flag(cursor, col);
}

/*!
  \brief Check if column flag is 'update'

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')

  \return 1 if true
  \return 0 if false
*/
int db_test_cursor_column_for_update(dbCursor *cursor, int col)
{
    return db_test_cursor_column_flag(cursor, col);
}

/*!
  \brief Check if columns' flag is 'update'

  \param cursor pointer to dbCursor

  \return 1 if true
  \return 0 if false
*/
int db_test_cursor_any_column_for_update(dbCursor *cursor)
{
    return db_test_cursor_any_column_flag(cursor);
}

/*!
  \brief Set column's flag

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')
 */
void db_set_cursor_column_flag(dbCursor *cursor, int col)
{
    if (cursor->column_flags)
	cursor->column_flags[col] = 1;
}

/*!
  \brief Unset column's flag

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')
 */
void db_unset_cursor_column_flag(dbCursor *cursor, int col)
{
    if (cursor->column_flags)
	cursor->column_flags[col] = 0;
}

/*!
  \brief Checks column's flag

  \param cursor pointer to dbCursor
  \param col    column index (starting with '0')

  \return 1 if flag is defined
  \return 0 otherwise
*/
int db_test_cursor_column_flag(dbCursor *cursor, int col)
{
    return cursor->column_flags && cursor->column_flags[col] ? 1 : 0;
}

/*!
  \brief Get number of columns

  \param cursor pointer to dbCursor
*/
int db_get_cursor_number_of_columns(dbCursor *cursor)
{
    dbTable *table;

    table = db_get_cursor_table(cursor);
    if (table)
	return db_get_table_number_of_columns(table);
    return 0;
}

/*!
  \brief Checks columns' flag

  Is any cursor column flag set?

  \param cursor pointer to dbCursor

  \return 1 if true
  \return 0 if false
*/
int db_test_cursor_any_column_flag(dbCursor *cursor)
{
    int ncols, col;

    ncols = db_get_cursor_number_of_columns(cursor);
    for (col = 0; col < ncols; col++)
	if (db_test_cursor_column_flag(cursor, col))
	    return 1;
    return 0;
}
