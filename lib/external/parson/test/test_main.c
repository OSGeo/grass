/****************************************************************************
 *
 * MODULE:       test.gjson.lib
 *
 * AUTHOR:       Nishant Bansal (nishant.bansal.282003@gmail.com)
 *
 * PURPOSE:      Unit tests for the gjson library
 *
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with
 *               GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "test_gjson_lib.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Flag *testunit;
} paramType;

paramType param; /*Parameters */

/*- prototypes --------------------------------------------------------------*/
static void set_params(void); /*Fill the paramType structure */

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */

/* ************************************************************************* */
void set_params(void)
{
    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = "Run all unit tests";
}
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
    G_add_keyword(_("general"));
    G_add_keyword(_("json"));
    G_add_keyword(_("unit test"));
    module->description = _("Performs unit tests "
                            "for the gjson library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /*Run the unit tests */
    if (param.testunit->answer) {
        returnstat += unit_test_parson_wrapper();
    }

    if (returnstat != 0)
        G_warning("Errors detected while testing the gjson lib");
    else
        G_message("\n-- gjson lib tests finished successfully --");

    return (returnstat);
}
