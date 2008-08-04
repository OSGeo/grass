
/*****************************************************************************/

/***                                                                       ***/

/***                            write_cats()                               ***/

/***   	        Writes out category file for morphometric features	   ***/

/***               Jo Wood, Project ASSIST, 7th February 1995              ***/

/***                                                                       ***/

/*****************************************************************************/
#include <grass/gis.h>
#include <grass/glocale.h>
#include "param.h"


void write_cats(void)
{
    struct Categories cats;

    /*------------------------------------------------------------------------*/
    /*                            INITIALISE                                  */

    /*------------------------------------------------------------------------*/
    G_init_cats((CELL) 0, "", &cats);
    G_set_raster_cats_title("Surface Features", &cats);

    /*------------------------------------------------------------------------*/
    /*                      FILL OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    G_set_cat(FLAT, " Planar", &cats);
    G_set_cat(PIT, " Pit", &cats);
    G_set_cat(PEAK, " Peak", &cats);
    G_set_cat(RIDGE, " Ridge", &cats);
    G_set_cat(CHANNEL, " Channel", &cats);
    G_set_cat(PASS, " Pass (saddle)", &cats);

    /*------------------------------------------------------------------------*/
    /*                     WRITE OUT CATEGORIES STRUCTURE                     */

    /*------------------------------------------------------------------------*/
    if (G_write_cats(rast_out_name, &cats) <= 0)
	G_warning(_("Cannot write category file for raster map <%s>"),
		  rast_out_name);

    G_free_cats(&cats);

    return;
}
