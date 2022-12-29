
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/* add column to table */
int add_column(int tab, int type, char *name, int width, int decimals)
{
    int c;

    G_debug(3,
	    "add_column(): tab = %d, type = %d, name = %s, width = %d, decimals = %d",
	    tab, type, name, width, decimals);

    /* truncate column name */
    if (strlen(name) > DBF_COL_NAME - 1) {
	char buf[2000];

	sprintf(buf, "DBMI-DBF driver: column name '%s'", name);
	name[DBF_COL_NAME - 1] = '\0';
	sprintf(buf + strlen(buf), " truncated to '%s'", name);
	G_warning("%s", buf);
    }

    /* Check if the column exists */
    for (c = 0; c < db.tables[tab].ncols; c++) {
	if (G_strcasecmp(db.tables[tab].cols[c].name, name) == 0) {
	    db_d_append_error(_("Column '%s' already exists (duplicate name)"),
			      name);
	    db_d_report_error();
	    return DB_FAILED;
	}
    }

    c = db.tables[tab].ncols;

    if (db.tables[tab].ncols == db.tables[tab].acols) {
	db.tables[tab].acols += 15;
	db.tables[tab].cols =
	    (COLUMN *) G_realloc(db.tables[tab].cols,
				 db.tables[tab].acols * sizeof(TABLE));
    }

    strncpy(db.tables[tab].cols[c].name, name, DBF_COL_NAME - 1);
    db.tables[tab].cols[c].name[DBF_COL_NAME - 1] = '\0';

    db.tables[tab].cols[c].type = type;
    db.tables[tab].cols[c].width = width;
    db.tables[tab].cols[c].decimals = decimals;

    db.tables[tab].ncols++;

    return DB_OK;
}

/* returns column index or -1 */
int find_column(int tab, char *col)
{
    int i;

    for (i = 0; i < db.tables[tab].ncols; i++) {
	if (G_strcasecmp(db.tables[tab].cols[i].name, col) == 0)
	    return (i);
    }
    return (-1);
}

/* drop column from table */
int drop_column(int tab, char *name)
{
    int i, j, c;

    G_debug(3, "drop_column(): tab = %d, name = %s", tab, name);

    /* Check if the column exists */
    c = find_column(tab, name);
    if (c == -1) {
	db_d_append_error(_("Column '%s' does not exist"), name);
	db_d_report_error();
	return DB_FAILED;
    }

    db.tables[tab].ncols--;

    for (i = c; i < db.tables[tab].ncols; i++) {
	strcpy(db.tables[tab].cols[i].name, db.tables[tab].cols[i + 1].name);
	db.tables[tab].cols[i].type = db.tables[tab].cols[i + 1].type;
	db.tables[tab].cols[i].width = db.tables[tab].cols[i + 1].width;
	db.tables[tab].cols[i].decimals = db.tables[tab].cols[i + 1].decimals;
    }

    /* drop column from each row */
    for (i = 0; i < db.tables[tab].nrows; i++) {
	for (j = c; j < db.tables[tab].ncols; j++) {
	    VALUE *dbval_c = &(db.tables[tab].rows[i].values[j]);
	    VALUE *dbval_c1 = &(db.tables[tab].rows[i].values[j + 1]);

	    dbval_c->i = dbval_c1->i;
	    dbval_c->d = dbval_c1->d;

	    if (dbval_c1->c != NULL) {
		save_string(dbval_c, dbval_c1->c);
		G_free((char *)dbval_c1->c);
		dbval_c1->c = NULL;
	    }

	    dbval_c->is_null = dbval_c1->is_null;
	}

	db.tables[tab].rows[i].values =
	    (VALUE *) G_realloc(db.tables[tab].rows[i].values,
				db.tables[tab].ncols * sizeof(VALUE));
    }
    return DB_OK;
}
