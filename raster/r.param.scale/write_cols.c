
/*****************************************************************************/

/***                                                                       ***/

/***                            write_cols()                               ***/

/***   	         Writes out colour file for morphometric features          ***/

/***               Jo Wood, Project ASSIST, 21st February 1995             ***/

/***                                                                       ***/

/*****************************************************************************/

#include "param.h"


void write_cols(void)
{

    /*------------------------------------------------------------------------*/
    /*                            INITIALISE                                  */

    /*------------------------------------------------------------------------*/

    struct Colors colours;

    G_init_colors(&colours);

    /*------------------------------------------------------------------------*/
    /*                       FILL OUT COLORS STRUCTURE                        */

    /*------------------------------------------------------------------------*/

    G_add_color_rule(FLAT, 180, 180, 180,	/* White      */
		     PIT, 0, 0, 0, &colours);	/* Black      */
    G_add_color_rule(CHANNEL, 0, 0, 255,	/* Blue       */
		     PASS, 0, 255, 0, &colours);	/* Green      */
    G_add_color_rule(RIDGE, 255, 255, 0,	/* Yellow     */
		     PEAK, 255, 0, 0, &colours);	/* Red        */


    /*------------------------------------------------------------------------*/
    /*                       WRITE OUT COLORS STRUCTURE                       */

    /*------------------------------------------------------------------------*/

    G_write_colors(rast_out_name, mapset_out, &colours);

    G_free_colors(&colours);

}
