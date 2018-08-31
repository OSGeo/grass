/*!
  \file db/driver/postgres/parse.c
  
  \brief DBMI - Low Level PostgreSQL database driver - parse connection string
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"
#include <grass/glocale.h>

/*
  \brief Parse connection string in form:
  1) 'database_name'
  2) 'host=xx,port=xx,dbname=xx'
  
  \returns DB_OK on success
  \return DB_FAILED on failure
*/
int parse_conn(const char *str, PGCONN * pgconn)
{
    int i;
    char **tokens, delm[2];

    /* reset */
    G_zero(pgconn, sizeof(PGCONN));

    G_debug(3, "parse_conn: '%s'", str);

    if (strchr(str, '=') == NULL) {	/* db name only */
	pgconn->dbname = G_store(str);
    }
    else {
	delm[0] = ',';
	delm[1] = '\0';
	tokens = G_tokenize(str, delm);
	i = 0;
	while (tokens[i]) {
	    G_chop(tokens[i]);
	    G_debug(3, "token %d : %s", i, tokens[i]);
	    if (strncmp(tokens[i], "host", 4) == 0)
		pgconn->host = G_store(tokens[i] + 5);
	    else if (strncmp(tokens[i], "port", 4) == 0)
		pgconn->port = G_store(tokens[i] + 5);
	    else if (strncmp(tokens[i], "options", 7) == 0)
		pgconn->options = G_store(tokens[i] + 8);
	    else if (strncmp(tokens[i], "tty", 3) == 0)
		pgconn->tty = G_store(tokens[i] + 4);
	    else if (strncmp(tokens[i], "dbname", 6) == 0)
		pgconn->dbname = G_store(tokens[i] + 7);
	    else if (strncmp(tokens[i], "user", 4) == 0)
		G_warning(_("'user' in database definition is not supported, use db.login"));
	    /* pgconn->user = G_store ( tokens[i] + 5 ); */
	    else if (strncmp(tokens[i], "password", 8) == 0)
		/* pgconn->password = G_store ( tokens[i] + 9 ); */
		G_warning(_("'password' in database definition is not supported, use db.login"));
	    else if (strncmp(tokens[i], "schema", 6) == 0)
		pgconn->schema = G_store(tokens[i] + 7);
	    else {
		db_d_append_error("%s %s",
				  _("Unknown option in database definition "
				    "for PostgreSQL: "),
				  tokens[i]);
		return DB_FAILED;
	    }
	    i++;
	}
	G_free_tokens(tokens);
    }

    return DB_OK;
}
