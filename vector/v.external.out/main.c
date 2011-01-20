
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

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "ogr_api.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _options options;
    struct _flags   flags;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("output"));
    G_add_keyword(_("external"));

    module->description =
	_("Defines vector output format utilizing OGR library.");

    OGRRegisterAll();

    parse_args(argc, argv, &options, &flags);

    if (flags.f->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    if (flags.r->answer) {
	G_remove("", "OGR");
	exit(EXIT_SUCCESS);
    }

    if (options.format->answer)
	check_format(options.format->answer);

    if (options.dsn->answer)
	make_link(options.dsn->answer,
		  options.format->answer, options.opts->answers);
    
    if (flags.p->answer) {
	print_status();
    }

    exit(EXIT_SUCCESS);
}
