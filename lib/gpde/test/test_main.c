
/****************************************************************************
*
* MODULE:       test.gpde.lib
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert <at> gmx <dot> de
* 		27 11 2006 Berlin
*
* PURPOSE:      Unit and integration tests for the gpde library
*
* COPYRIGHT:    (C) 2006 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/N_pde.h>
#include <grass/gmath.h>
#include "test_gpde_lib.h"


/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    struct Option *unit, *integration;
    struct Flag *full, *testunit, *testint;
} paramType;

paramType param;		/*Parameters */

/*- prototypes --------------------------------------------------------------*/
static void set_params(void);		/*Fill the paramType structure */

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params(void)
{
    param.unit = G_define_option();
    param.unit->key = "unit";
    param.unit->type = TYPE_STRING;
    param.unit->required = NO;
    param.unit->options = "array,assemble,geom,gradient,les,tools";
    param.unit->description = "Choose the unit tests to run";

    param.integration = G_define_option();
    param.integration->key = "integration";
    param.integration->type = TYPE_STRING;
    param.integration->required = NO;
    param.integration->options = "gwflow,heatflow,transport";
    param.integration->description = "Choose the integration tests to run";


    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = "Run all unit tests";

    param.testint = G_define_flag();
    param.testint->key = 'i';
    param.testint->description = "Run all integration tests";

    param.full = G_define_flag();
    param.full->key = 'a';
    param.full->description = "Run all unit and integration tests";

}

/* ************************************************************************* */
/* Main function, open the RASTER3D map and create the raster maps ************** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module;
    int returnstat = 0, i;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword("test");
    G_add_keyword("gpde");
    module->description =
	_("Performs unit and integration tests for gpde library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /*Run the unit tests */
    if (param.testunit->answer || param.full->answer) {
	returnstat += unit_test_arrays();
	returnstat += unit_test_assemble();
	returnstat += unit_test_gradient();
	returnstat += unit_test_geom_data();
	returnstat += unit_test_les_creation();
	returnstat += unit_test_tools();

    }

    /*Run the integration tests */
    if (param.testint->answer || param.full->answer) {
	returnstat += integration_test_gwflow();
	returnstat += integration_test_solute_transport();
    }

    /*Run single tests */
    if (!param.full->answer) {
	/*unit tests */
	if (!param.testunit->answer) {
	    i = 0;
	    if (param.unit->answers)
		while (param.unit->answers[i]) {
		    if (strcmp(param.unit->answers[i], "array") == 0)
			returnstat += unit_test_arrays();

		    if (strcmp(param.unit->answers[i], "assemble") == 0)
			returnstat += unit_test_assemble();

		    if (strcmp(param.unit->answers[i], "gradient") == 0)
			returnstat += unit_test_gradient();

		    if (strcmp(param.unit->answers[i], "geom") == 0)
			returnstat += unit_test_geom_data();

		    if (strcmp(param.unit->answers[i], "les") == 0)
			returnstat += unit_test_les_creation();

		    if (strcmp(param.unit->answers[i], "tools") == 0)
			returnstat += unit_test_tools();

		    i++;
		}
	}
	/*integration tests */
	if (!param.testint->answer) {
	    i = 0;
	    if (param.integration->answers)
		while (param.integration->answers[i]) {
		    if (strcmp(param.integration->answers[i], "gwflow") == 0)
			returnstat += integration_test_gwflow();

		    if (strcmp(param.integration->answers[i], "heatflow") == 0);	/*nothing to do for now */

		    if (strcmp(param.integration->answers[i], "transport") == 0)
		        returnstat += integration_test_solute_transport();

		    i++;
		}

	}
    }

    if(returnstat != 0)
    	G_warning("Errors detected while testing the gpde lib");
    else
    	G_message("\n-- gpde lib tests finished successfully --");

    return (returnstat);
}
