
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001,2008 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int replace_variables(char *in, char **datadir, char **database)
{
    *datadir = NULL;
    *database = NULL;

    /* parse/replace variables in input string */
    char tmp[2000];
    char **tokens;
    int no_tokens, n;

    if (!strchr(in, '/')) {	/* no path */
	*datadir = G_store("./");
	*database = G_store(in);
    }
    else {
	tokens = G_tokenize(in, "/");
	no_tokens = G_number_of_tokens(tokens);

	G_debug(3, "no_tokens = %d", no_tokens);

	tmp[0] = '\0';
	for (n = 0; n < no_tokens - 1; n++) {
	    G_chop(tokens[n]);
	    if (n > 0)
		strcat(tmp, "/");

	    G_debug(3, "tokens[%d] = %s", n, tokens[n]);
	    if (tokens[n][0] == '$') {
		G_strchg(tokens[n], '$', ' ');
		G_chop(tokens[n]);
		strcat(tmp, G_getenv_nofatal(tokens[n]));
		G_debug(3, "   -> %s", G_getenv_nofatal(tokens[n]));
	    }
	    else {
		strcat(tmp, tokens[n]);
	    }
	}
	*datadir = G_store(tmp);
	*database = G_store(tokens[n]);

	G_free_tokens(tokens);
    }

    G_debug(2, "datadir = '%s'", *datadir);
    G_debug(2, "database = '%s'", *database);

    return 1;
}

