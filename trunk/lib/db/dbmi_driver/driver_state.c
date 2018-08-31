/*!
 * \file db/dbmi_driver/driver_state.c
 * 
 * \brief DBMI Library (driver) - drivers state
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include "dbstubs.h"


static dbDriverState state;

/*!
  \brief Initialize driver state
*/
void db__init_driver_state(void)
{
    db_zero((void *)&state, sizeof(state));
}

/*!
  \brief Get driver state

  \return pointer to dbDriverState
*/
dbDriverState *db__get_driver_state(void)
{
    return &state;
}

/*!
  \brief Test database connection

  \return 1 opened
  \return 0 closed
*/
int db__test_database_open(void)
{
    return state.open ? 1 : 0;
}

/*!
  \brief Mark database as opened

  \param dbname database name
  \param dbschema database schema name
*/
void db__mark_database_open(const char *dbname, const char *dbschema)
{
    state.dbname = db_store(dbname);
    state.dbschema = db_store(dbschema);
    state.open = 1;
}

/*!
  \brief Mark database as closed
*/
void db__mark_database_closed(void)
{
    db_free(state.dbname);
    db_free(state.dbschema);
    state.open = 0;
}

/*!
  \brief Add cursor do driver state

  \param cursor db cursor to be added
*/
void db__add_cursor_to_driver_state(dbCursor * cursor)
{
    dbCursor **list;
    int i;

    /* find an empty slot in the cursor list */
    list = state.cursor_list;
    for (i = 0; i < state.ncursors; i++)
	if (list[i] == NULL)
	    break;

    /* if not found, extend list */
    if (i >= state.ncursors) {
	list =
	    (dbCursor **) db_realloc((void *)list,
				     (i + 1) * sizeof(dbCursor *));
	if (list == NULL)
	    return;
	state.cursor_list = list;
	state.ncursors = i + 1;
    }

    /* add it in */
    list[i] = cursor;
}

/*!
  \brief Drop cursor from driver state

  \param cursor db cursor to be dropped
*/
void db__drop_cursor_from_driver_state(dbCursor * cursor)
{
    int i;

    for (i = 0; i < state.ncursors; i++)
	if (state.cursor_list[i] == cursor)
	    state.cursor_list[i] = NULL;
}

/*!
  \brief Close all cursors
*/
void db__close_all_cursors(void)
{
    int i;

    for (i = 0; i < state.ncursors; i++)
	if (state.cursor_list[i])
	    db_driver_close_cursor(state.cursor_list[i]);

    if (state.cursor_list)
	db_free(state.cursor_list);

    state.ncursors = 0;
    state.cursor_list = NULL;
}
