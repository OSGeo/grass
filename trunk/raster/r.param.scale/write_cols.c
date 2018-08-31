
/*****************************************************************************/

/***                                                                       ***/

/***                            write_cols()                               ***/

/***   	         Writes out colour file for morphometric features          ***/

/***               Jo Wood, Project ASSIST, 21st February 1995             ***/

/***                                                                       ***/

/*****************************************************************************/

#include <grass/raster.h>
#include "param.h"


void write_cols(void)
{
    struct Colors colours;
    CELL val1, val2;
    Rast_init_colors(&colours);

    val1 = FLAT;
    val2 = PIT;
    Rast_add_c_color_rule(&val1, 180, 180, 180,	/* White      */
			  &val2, 0, 0, 0, &colours);	/* Black      */
    val1 = CHANNEL;
    val2 = PASS;
    Rast_add_c_color_rule(&val1, 0, 0, 255,	/* Blue       */
			  &val2, 0, 255, 0, &colours);	/* Green      */
    val1 = RIDGE;
    val2 = PEAK;
    Rast_add_c_color_rule(&val1, 255, 255, 0,	/* Yellow     */
			  &val2, 255, 0, 0, &colours);	/* Red        */

    Rast_write_colors(rast_out_name, G_mapset(), &colours);

    Rast_free_colors(&colours);

}
