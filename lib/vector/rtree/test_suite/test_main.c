/****************************************************************************
 *
 * MODULE:       test.gpde.lib
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert <at> gmx <dot> de
 * 		05 Sep 2007 Berlin
 *
 * PURPOSE:      Unit and integration tests for the gmath library
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
#include "test_rtree_lib.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *unit, *integration, *solverbenchmark, *blasbenchmark, *rows;
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
    param.unit->required = YES;
    param.unit->options = "basics";
    param.unit->description = _("Choose the unit tests to run");

    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = _("Run all unit tests");

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
            = _("Performs benchmarks, unit and integration tests for the gmath library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /*unit tests */
    i = 0;
    if (param.unit->answers) {
        while (param.unit->answers[i]) {
            if (strcmp(param.unit->answers[i], "basics") == 0)
                returnstat += unit_test_basics();
            i++;
        }
    }


    
    if (returnstat != 0)
        G_warning("Errors detected while testing the gmath lib");
    else
        G_message("\n-- gmath lib tests finished successfully --");

    return (returnstat);
}
