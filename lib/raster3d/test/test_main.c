
/*****************************************************************************
*
* MODULE:       Grass raster3d Library
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
#include "test_raster3d_lib.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *unit, *integration, *depths, *rows, *cols, *tile_size;
    struct Flag *full, *testunit, *testint, *rle, *compression;
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
    param.unit->description = "Choose the unit tests to run";

    param.integration = G_define_option();
    param.integration->key = "integration";
    param.integration->type = TYPE_STRING;
    param.integration->required = NO;
    param.integration->options = "";
    param.integration->description = "Choose the integration tests to run";

    param.depths = G_define_option();
    param.depths->key = "depths";
    param.depths->type = TYPE_INTEGER;
    param.depths->required = NO;
    param.depths->answer = "20";
    param.depths->description = "The number of depths to be used for the large file put/get value test";

    param.rows = G_define_option();
    param.rows->key = "rows";
    param.rows->type = TYPE_INTEGER;
    param.rows->required = NO;
    param.rows->answer = "5400";
    param.rows->description = "The number of rows to be used for the large file put/get value test";

    param.cols = G_define_option();
    param.cols->key = "cols";
    param.cols->type = TYPE_INTEGER;
    param.cols->required = NO;
    param.cols->answer = "10800";
    param.cols->description = "The number of columns to be used for the large file put/get value test";

    param.tile_size = G_define_option();
    param.tile_size->key = "tile_size";
    param.tile_size->type = TYPE_INTEGER;
    param.tile_size->required = NO;
    param.tile_size->answer = "32";
    param.tile_size->description = "The tile size in kilo bytes to be used for the large file put/get value test. Set the tile size to 2048 and the number of row*cols*depths > 130000 to reproduce the tile rle error.";

    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = "Run all unit tests";

    param.testint = G_define_flag();
    param.testint->key = 'i';
    param.testint->description = "Run all integration tests";

    param.full = G_define_flag();
    param.full->key = 'a';
    param.full->description = "Run all unit and integration tests";

    param.compression = G_define_flag();
    param.compression->key = 'l';
    param.compression->description = "Switch zip compression on";
}

/* ************************************************************************* */
/* ************************************************************************* */

/* ************************************************************************* */
int main(int argc, char *argv[]) {
    struct GModule *module;
    int returnstat = 0, i;
    int depths, rows, cols, tile_size;
    int doCompress = RASTER3D_COMPRESSION;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description
            = "Performs unit and integration tests for the raster3d library";

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
 
    depths = atoi(param.depths->answer);
    rows   = atoi(param.rows->answer);
    cols   = atoi(param.cols->answer);
    tile_size = atoi(param.tile_size->answer);

    if(param.compression->answer) {
        doCompress = RASTER3D_COMPRESSION;
    } else {
        doCompress = RASTER3D_NO_COMPRESSION;
    }

    /* Set the compression mode that should be used */
    Rast3d_set_compression_mode(doCompress, RASTER3D_MAX_PRECISION);
   
    /* Initiate the defaults for testing */
    Rast3d_init_defaults();

    /*Run the unit tests */
    if (param.testunit->answer || param.full->answer) {
        returnstat += unit_test_coordinate_transform();
        returnstat += unit_test_put_get_value();
        returnstat += unit_test_put_get_value_large_file(depths, rows, cols, tile_size);
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
                        returnstat += unit_test_put_get_value_large_file(depths, rows, cols, tile_size);
                    
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
        G_warning("Errors detected while testing the raster3d lib");
    else
        G_message("\n-- raster3d lib tests finished successfully --");

    return (returnstat);
}
