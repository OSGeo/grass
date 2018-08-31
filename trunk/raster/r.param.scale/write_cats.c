
/*****************************************************************************/

/***                                                                       ***/

/***                            write_cats()                               ***/

/***   	        Writes out category file for morphometric features	   ***/

/***               Jo Wood, Project ASSIST, 7th February 1995              ***/

/***                                                                       ***/

/*****************************************************************************/
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "param.h"


void write_cats(void)
{
    struct Categories cats;
    CELL cat;

    /*------------------------------------------------------------------------*/
    /*                            INITIALISE                                  */

    /*------------------------------------------------------------------------*/
    Rast_init_cats("", &cats);
    Rast_set_cats_title("Surface Features", &cats);

    /*------------------------------------------------------------------------*/
    /*                      FILL OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    cat = FLAT;
    Rast_set_c_cat(&cat, &cat, " Planar", &cats);
    cat = PIT;
    Rast_set_c_cat(&cat, &cat, " Pit", &cats);
    cat = PEAK;
    Rast_set_c_cat(&cat, &cat, " Peak", &cats);
    cat = RIDGE;
    Rast_set_c_cat(&cat, &cat, " Ridge", &cats);
    cat = CHANNEL;
    Rast_set_c_cat(&cat, &cat, " Channel", &cats);
    cat = PASS;
    Rast_set_c_cat(&cat, &cat, " Pass (saddle)", &cats);

    /*------------------------------------------------------------------------*/
    /*                     WRITE OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    Rast_write_cats(rast_out_name, &cats);

    Rast_free_cats(&cats);

    return;
}
