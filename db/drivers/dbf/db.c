
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
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_open_database(dbHandle * handle)
{
    const char *name;
    int len;
    dbConnection connection;
    char buf[1024];
    DIR *dir;
    struct dirent *ent;
    char **tokens;
    int no_tokens, n;

    G_debug(2, "DBF: db__driver_open_database() name = '%s'",
	    db_get_handle_dbname(handle));

    db.name[0] = '\0';
    db.tables = NULL;
    db.atables = 0;
    db.ntables = 0;

    db_get_connection(&connection);
    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0) {
	name = connection.databaseName;
    }

    strcpy(db.name, name);

    /* open database dir and read table ( *.dbf files ) names 
     * to structure */

    /* parse variables in db.name if present */
    if (db.name[0] == '$') {
	tokens = G_tokenize(db.name, "/");
	no_tokens = G_number_of_tokens(tokens);
	db.name[0] = '\0';	/* re-init */

	for (n = 0; n < no_tokens; n++) {
	    G_chop(tokens[n]);
	    G_debug(3, "tokens[%d] = %s", n, tokens[n]);
	    if (tokens[n][0] == '$') {
		G_strchg(tokens[n], '$', ' ');
		G_chop(tokens[n]);
		strcat(db.name, G_getenv_nofatal(tokens[n]));
		G_debug(3, "   -> %s", G_getenv_nofatal(tokens[n]));
	    }
	    else
		strcat(db.name, tokens[n]);

	    strcat(db.name, "/");
	}
	G_free_tokens(tokens);
    }

    G_debug(2, "db.name = %s", db.name);

    errno = 0;
    dir = opendir(db.name);
    if (dir == NULL) {
	if (errno == ENOENT) {
	    int status;

	    status = G_mkdir(db.name);
	    if (status != 0) {	/* mkdir failed */
		db_d_append_error(_("Unable to create DBF database: %s"), name);
		db_d_report_error();
		return DB_FAILED;
	    }
	    else {
		/* now that the dbf/ dir is created, try again */
		dir = opendir(db.name);
		if (dir == NULL) {
		    db_d_append_error(_("Cannot open DBF database directory: %s"),
					name);
		    db_d_report_error();
		    return DB_FAILED;
		}
	    }
	}
	else {			/* some other problem */
	    db_d_append_error(_("Unable to open DBF database: %s"), name);
	    db_d_report_error();
	    return DB_FAILED;
	}
    }

    while ((ent = readdir(dir))) {
	len = strlen(ent->d_name) - 4;
	if ((len > 0) && (G_strcasecmp(ent->d_name + len, ".dbf") == 0)) {
	    strcpy(buf, ent->d_name);
	    buf[len] = '\0';
	    add_table(buf, ent->d_name);
	}
    }

    closedir(dir);
    return DB_OK;
}

int db__driver_close_database()
{
    int i;

    for (i = 0; i < db.ntables; i++) {
	save_table(i);
	free_table(i);
    }
    G_free(db.tables);

    return DB_OK;
}
