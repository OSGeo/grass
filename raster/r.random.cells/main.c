
/****************************************************************************
 *
 * MODULE:       r.random.cells
 * AUTHOR(S):    Charles Ehlschlaeger; National Center for Geographic Information
 *               and Analysis, University of California, Santa Barbara (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      generates a random sets of cells that are at least
 *               some distance apart
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#undef TRACE
#undef DEBUG

#define MAIN
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "ransurf.h"
#include "local_proto.h"
#undef MAIN

int main(int argc, char *argv[])
{
    struct GModule *module;

    FUNCTION(main);

    G_gisinit(argv[0]);
    /* Set description */
    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Generates random cell values with spatial dependence.");

    Init(argc, argv);
    Indep();
    exit(EXIT_SUCCESS);
}
