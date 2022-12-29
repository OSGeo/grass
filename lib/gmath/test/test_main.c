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
#include <grass/gmath.h>
#include "test_gmath_lib.h"
#include "grass/gmath.h"

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
    param.unit->required = NO;
    param.unit->options = "blas1,blas2,blas3,solver,ccmath,matconv";
    param.unit->description = _("Choose the unit tests to run");

    param.integration = G_define_option();
    param.integration->key = "integration";
    param.integration->type = TYPE_STRING;
    param.integration->required = NO;
    param.integration->options = "";
    param.integration->description = _("Choose the integration tests to run");

    param.rows = G_define_option();
    param.rows->key = "rows";
    param.rows->type = TYPE_INTEGER;
    param.rows->required = NO;
    param.rows->answer = "1000";
    param.rows->description = _("The size of the matrices and vectors for benchmarking");

    param.solverbenchmark = G_define_option();
    param.solverbenchmark->key = "solverbench";
    param.solverbenchmark->type = TYPE_STRING;
    param.solverbenchmark->required = NO;
    param.solverbenchmark->options = "krylov,direct";
    param.solverbenchmark->description = _("Choose solver benchmark");

    param.blasbenchmark = G_define_option();
    param.blasbenchmark->key = "blasbench";
    param.blasbenchmark->type = TYPE_STRING;
    param.blasbenchmark->required = NO;
    param.blasbenchmark->options = "blas2,blas3";
    param.blasbenchmark->description = _("Choose blas benchmark");
    
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
    int rows = 3000;


    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description
            = _("Performs benchmarks, unit and integration tests for the gmath library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);


    if (param.rows->answer)
        sscanf(param.rows->answer, "%i", &rows);

    /*Run the unit tests */
    if (param.testunit->answer || param.full->answer) {
        returnstat += unit_test_blas_level_1();
        returnstat += unit_test_blas_level_2();
        returnstat += unit_test_blas_level_3();
        returnstat += unit_test_solvers();
        returnstat += unit_test_matrix_conversion();
        returnstat += unit_test_ccmath_wrapper();

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
                    if (strcmp(param.unit->answers[i], "blas1") == 0)
                        returnstat += unit_test_blas_level_1();

                    if (strcmp(param.unit->answers[i], "blas2") == 0)
                        returnstat += unit_test_blas_level_2();

                    if (strcmp(param.unit->answers[i], "blas3") == 0)
                        returnstat += unit_test_blas_level_3();

                    if (strcmp(param.unit->answers[i], "solver") == 0)
                        returnstat += unit_test_solvers();

                    if (strcmp(param.unit->answers[i], "ccmath") == 0)
                        returnstat += unit_test_ccmath_wrapper();

                    if (strcmp(param.unit->answers[i], "matconv") == 0)
                        returnstat += unit_test_matrix_conversion();

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

    i = 0;
    if (param.solverbenchmark->answers)
        while (param.solverbenchmark->answers[i]) {
            if (strcmp(param.solverbenchmark->answers[i], "krylov") == 0)
                bench_solvers_krylov(rows);
            if (strcmp(param.solverbenchmark->answers[i], "direct") == 0)
                bench_solvers_direct(rows);
            i++;
        }

    i = 0;
    if (param.blasbenchmark->answers)
        while (param.blasbenchmark->answers[i]) {
            if (strcmp(param.blasbenchmark->answers[i], "blas2") == 0)
                bench_blas_level_2(rows);
            if (strcmp(param.blasbenchmark->answers[i], "blas3") == 0)
                bench_blas_level_3(rows);

            i++;
        }
    
    if (returnstat != 0)
        G_warning("Errors detected while testing the gmath lib");
    else
        G_message("\n-- gmath lib tests finished successfully --");

    return (returnstat);
}
