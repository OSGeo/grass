/****************************************************************************
 *
 * MODULE:       test.gparson.lib
 *
 *
 * PURPOSE:      Unit tests for the gparson library
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with
 *               GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gparson.h>
#include "test_gparson_lib.h"
#include "grass/gparson.h"

/* ************************************************************************* */
/* ************************************************************************* */

/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module;
    int returnstat = 0;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("gparson"));
    G_add_keyword(_("unit test"));
    module->description = _("Performs unit tests "
                            "for the gparson library");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /*Run the unit tests */
    returnstat += unit_test_parson_wrapper();

    if (returnstat != 0)
        G_warning("Errors detected while testing the gparson lib");
    else
        G_message("\n-- gparson lib tests finished successfully --");

    return (returnstat);
}
