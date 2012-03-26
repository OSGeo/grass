
/****************************************************************
 *
 * MODULE:       v.external.out
 * 
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com> (based on r.external.out code)
 *               
 * PURPOSE:      Make GRASS write vector maps utilizing the OGR library.
 *               
 * COPYRIGHT:    (C) 2010 by Martin Landa and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _options options;
    struct _flags   flags;

    char * format;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword(_("external"));
    G_add_keyword(_("OGR"));
    G_add_keyword(_("PostGIS"));
    module->description = _("Defines vector output format.");

#ifdef HAVE_OGR
    OGRRegisterAll();
#endif
    parse_args(argc, argv, &options, &flags);

    if (flags.f->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    if (flags.r->answer) {
	if (G_remove("", "OGR") == 0)
	    G_remove("", "PG");
	exit(EXIT_SUCCESS);
    }

    format = NULL;
    if (options.format->answer) {
	format = G_store(options.format->answer);
	check_format(format);
    }
    
    if (options.dsn->answer) {
	char *dsn;
	
	/* be friendly, ignored 'PG:' prefix for PostGIS format */
	if (strcmp(format, "PostGIS") == 0 &&
	    G_strncasecmp(options.dsn->answer, "PG:", 3) == 0) {
	    int i, length;
	    
	    length = strlen(options.dsn->answer);
	    dsn = (char *) G_malloc(length - 3);
	    for (i = 3; i < length; i++)
		dsn[i-3] = options.dsn->answer[i];
	    dsn[length-3] = '\0';
	}
	else {
	    dsn = G_store(options.dsn->answer);
	}
    
	make_link(dsn, format,
		  options.opts->answer, options.opts->answers);
    }
    
    if (flags.p->answer || flags.g->answer) {
	print_status(flags.g->answer ? 1 : 0);
    }

    exit(EXIT_SUCCESS);
}
