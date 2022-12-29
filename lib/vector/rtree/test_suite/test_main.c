/****************************************************************************
 *
 * MODULE:       test.rtree.lib
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert <at> gmx <dot> de
 * 		05 Sep 2007 Berlin
 *
 * PURPOSE:      Unit test for the vector rtree implementation
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
    struct Option *unit;
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
    param.unit->options = "basic";
    param.unit->description = _("Choose the unit tests to run");
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
            = _("Unit tests for the vector rtree library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /*unit tests */
    i = 0;
    if (param.unit->answers) {
        while (param.unit->answers[i]) {
            if (strcmp(param.unit->answers[i], "basic") == 0)
                returnstat += unit_test_basics();
            i++;
        }
    }


    
    if (returnstat != 0)
        G_warning("Errors detected while testing the vector rtree lib");
    else
        G_message("\n-- vector rtree lib tests finished successfully --");

    return (returnstat);
}
