#include <stdlib.h>
#include <grass/dbmi.h>
#include "dbstubs.h"


static dbDriverState state;


void db__init_driver_state(void)
{
    db_zero((void *)&state, sizeof(state));
}


dbDriverState *db__get_driver_state(void)
{
    return &state;
}


int db__test_database_open(void)
{
    return state.open ? 1 : 0;
}


void db__mark_database_open(const char *dbname, const char *dbschema)
{
    state.dbname = db_store(dbname);
    state.dbschema = db_store(dbschema);
    state.open = 1;
}


void db__mark_database_closed(void)
{
    free(state.dbname);
    free(state.dbschema);
    state.open = 0;
}


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


void db__drop_cursor_from_driver_state(dbCursor * cursor)
{
    int i;

    for (i = 0; i < state.ncursors; i++)
	if (state.cursor_list[i] == cursor)
	    state.cursor_list[i] = NULL;
}


void db__close_all_cursors(void)
{
    int i;

    for (i = 0; i < state.ncursors; i++)
	if (state.cursor_list[i])
	    db_driver_close_cursor(state.cursor_list[i]);

    if (state.cursor_list)
	free(state.cursor_list);

    state.ncursors = 0;
    state.cursor_list = NULL;
}
