/****************************************************************************
 *
 * MODULE:       test.dbmi_base.lib
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert <at> googlemail <dot> com
 * 		 2010 Braunschweig, Germany
 *
 * PURPOSE:      Unit and integration tests for the dbmi_base library
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include "test_dbmi_base_lib.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *integration, *unit;
    struct Flag *full, *testunit, *testint;
} paramType;

paramType param; /*Parameters */

/*- prototypes --------------------------------------------------------------*/
static void set_params(void); /*Fill the paramType structure */

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */

/* ************************************************************************* */
void set_params(void) {
    param.unit = G_define_option();
    param.unit->key = "unit";
    param.unit->type = TYPE_STRING;
    param.unit->required = NO;
    param.unit->options = "column,table";
    param.unit->description = _("Choose the unit tests to run");

    param.integration = G_define_option();
    param.integration->key = "integration";
    param.integration->type = TYPE_STRING;
    param.integration->required = NO;
    param.integration->options = "";
    param.integration->description = _("Choose the integration tests to run");
    
    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = _("Run all unit tests");

    param.testint = G_define_flag();
    param.testint->key = 'i';
    param.testint->description = _("Run all integration tests");

    param.full = G_define_flag();
    param.full->key = 'a';
    param.full->description = _("Run all unit and integration tests");

}

/* ************************************************************************* */
/* ************************************************************************* */

/* ************************************************************************* */
int main(int argc, char *argv[]) {
    struct GModule *module;
    int returnstat = 0, i;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description
            = _("Performs unit and integration tests for the dbmi base library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /*Run the unit tests */
    if (param.testunit->answer || param.full->answer) {
        returnstat += unit_test_column();
        returnstat += unit_test_table();
    }

    /*Run the integration tests */
    if (param.testint->answer || param.full->answer) {
        ;
    }

    /*Run single tests */
    if (!param.full->answer) {
        /*unit tests */
        if (!param.testunit->answer) {
            i = 0;
            if (param.unit->answers)
                while (param.unit->answers[i]) {
                    if (strcmp(param.unit->answers[i], "column") == 0)
                        returnstat += unit_test_column();
                    if (strcmp(param.unit->answers[i], "table") == 0)
                        returnstat += unit_test_table();

                    i++;
            }
        }
        /*integration tests */
        if (!param.testint->answer) {
            i = 0;
            if (param.integration->answers)
                while (param.integration->answers[i]) {
                    ;
            }
        }
    }
    
    if (returnstat != 0)
        G_warning("Errors detected while testing the dbmi_base lib");
    else
        G_message("\n-- dbmi_base lib tests finished successfully --");

    return (returnstat);
}
