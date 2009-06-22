
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

    /*------------------------------------------------------------------------*/
    /*                            INITIALISE                                  */

    /*------------------------------------------------------------------------*/
    Rast_init_cats((CELL) 0, "", &cats);
    Rast_set_raster_cats_title("Surface Features", &cats);

    /*------------------------------------------------------------------------*/
    /*                      FILL OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    Rast_set_cat(FLAT, " Planar", &cats);
    Rast_set_cat(PIT, " Pit", &cats);
    Rast_set_cat(PEAK, " Peak", &cats);
    Rast_set_cat(RIDGE, " Ridge", &cats);
    Rast_set_cat(CHANNEL, " Channel", &cats);
    Rast_set_cat(PASS, " Pass (saddle)", &cats);

    /*------------------------------------------------------------------------*/
    /*                     WRITE OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    if (Rast_write_cats(rast_out_name, &cats) <= 0)
	G_warning(_("Cannot write category file for raster map <%s>"),
		  rast_out_name);

    Rast_free_cats(&cats);

    return;
}
