
/*****************************************************************************
*
* MODULE:       Grass g3d Library
* AUTHOR(S):    Soeren Gebbert, Braunschweig (GER) Jun 2011
* 		        soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:	Unit and Integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "test_g3d_lib.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *unit, *integration, *depths, *rows, *cols;
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
    param.unit->options = "coord,putget,large";
    param.unit->description = _("Choose the unit tests to run");

    param.integration = G_define_option();
    param.integration->key = "integration";
    param.integration->type = TYPE_STRING;
    param.integration->required = NO;
    param.integration->options = "";
    param.integration->description = _("Choose the integration tests to run");

    param.depths = G_define_option();
    param.depths->key = "depths";
    param.depths->type = TYPE_INTEGER;
    param.depths->required = NO;
    param.depths->answer = "20";
    param.depths->description = _("The number of depths to be used for the large file put value test");

    param.rows = G_define_option();
    param.rows->key = "rows";
    param.rows->type = TYPE_INTEGER;
    param.rows->required = NO;
    param.rows->answer = "5400";
    param.rows->description = _("The number of rows to be used for the large file put value test");

    param.cols = G_define_option();
    param.cols->key = "cols";
    param.cols->type = TYPE_INTEGER;
    param.cols->required = NO;
    param.cols->answer = "10800";
    param.cols->description = _("The number of columns to be used for the large file put value test");

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
    int depths, rows, cols;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description
            = _("Performs unit and integration tests for the g3d library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
 
    depths = atoi(param.depths->answer);
    rows   = atoi(param.rows->answer);
    cols   = atoi(param.cols->answer);
   
    /* Initiate the defaults for testing */
    Rast3d_init_defaults();

    /*Run the unit tests */
    if (param.testunit->answer || param.full->answer) {
        returnstat += unit_test_coordinate_transform();
        returnstat += unit_test_put_get_value();
        returnstat += unit_test_put_get_value_large_file(depths, rows, cols);
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
                    if (strcmp(param.unit->answers[i], "coord") == 0)
                        returnstat += unit_test_coordinate_transform();
                    if (strcmp(param.unit->answers[i], "putget") == 0)
                        returnstat += unit_test_put_get_value();
                    if (strcmp(param.unit->answers[i], "large") == 0)
                        returnstat += unit_test_put_get_value_large_file(depths, rows, cols);
                    
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
        G_warning("Errors detected while testing the g3d lib");
    else
        G_message("\n-- g3d lib tests finished successfully --");

    return (returnstat);
}
