
/****************************************************************
 *
 * MODULE:       v.external.out
 * 
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Make GRASS write vector maps utilizing the OGR library.
 *               (Partly based on r.external.out code)
 *               
 * COPYRIGHT:    (C) 2010-2013 by Martin Landa and the GRASS Development Team
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
    G_add_keyword("OGR");
    G_add_keyword("PostGIS");
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
        if (G_remove("", "OGR") == 1) {
            G_verbose_message(_("Switched from OGR to native format"));
        }
        else {
            if (G_remove("", "PG") == 1)
                G_verbose_message(_("Switched from PostGIS to native format"));
        }
        exit(EXIT_SUCCESS);
    }

    format = NULL;
    if (options.format->answer) {
	format = G_store(options.format->answer);
	check_format(format);
    }
    
    if (options.dsn->answer) {
	make_link(options.dsn->answer, format,
		  options.opts->answer, options.opts->answers);
    }
    
    if (flags.p->answer || flags.g->answer) {
	print_status(flags.g->answer ? TRUE : FALSE);
    }

    exit(EXIT_SUCCESS);
}
