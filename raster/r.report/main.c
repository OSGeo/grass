/****************************************************************************
 *
 * MODULE:       r.report
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Roberto Flor <flor itc.it>, Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define GLOBAL
#include <stdlib.h>
#include "global.h"

int 
main (int argc, char *argv[])
{
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster, statistics");
    module->description =
	_("Reports statistics for raster map layers.");

    parse_command_line (argc, argv);

    G_get_window (&window);

    get_stats();

    report();

    exit(EXIT_SUCCESS);
}
