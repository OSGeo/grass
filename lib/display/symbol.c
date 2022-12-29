
/****************************************************************************
 *
 * MODULE:       display
 * AUTHOR(S):    Hamish Bowman <hamish_b yahoo.com> (original contributor)
 *                      (adapted from Radim Blazek's d.vect code)
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      draw a symbol at pixel coordinates
 * COPYRIGHT:    (C) 2005-2007 by M. Hamish Bowman, and
 *                              the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

static void symbol(const SYMBOL *Symb, double x0, double y0,
		   const RGBA_Color *fill_color,
		   const RGBA_Color *line_color,
		   const RGBA_Color *string_color)
{
    int i, j, k;
    const SYMBPART *part;
    const SYMBCHAIN *chain;
    double xp, yp;
    double *x, *y;
    double sx = D_get_d_to_u_xconv();
    double sy = D_get_d_to_u_yconv();

    G_debug(2, "D_symbol(): %d parts", Symb->count);

    for (i = 0; i < Symb->count; i++) {
	part = Symb->part[i];

	switch (part->type) {

	case S_POLYGON:
	    /* draw background fills */
	    if ((part->fcolor.color == S_COL_DEFAULT &&
		 fill_color->a != RGBA_COLOR_NONE) ||
		part->fcolor.color == S_COL_DEFINED) {
		if (part->fcolor.color == S_COL_DEFAULT)
		    D_RGB_color(fill_color->r, fill_color->g, fill_color->b);
		else
		    D_RGB_color(part->fcolor.r, part->fcolor.g,
				part->fcolor.b);

		for (j = 0; j < part->count; j++) {	/* for each component polygon */
		    chain = part->chain[j];

		    x = G_malloc(sizeof(double) * chain->scount);
		    y = G_malloc(sizeof(double) * chain->scount);

		    for (k = 0; k < chain->scount; k++) {
			x[k] = x0 + sx * chain->sx[k];
			y[k] = y0 - sy * chain->sy[k];
		    }
		    D_polygon_abs(x, y, chain->scount);

		    G_free(x);
		    G_free(y);
		}

	    }
	    /* again, to draw the lines */
	    if ((part->color.color == S_COL_DEFAULT &&
		 line_color->a != RGBA_COLOR_NONE) ||
		part->color.color == S_COL_DEFINED) {
		if (part->color.color == S_COL_DEFAULT)
		    D_RGB_color(line_color->r, line_color->g, line_color->b);
		else
		    D_RGB_color(part->color.r, part->color.g, part->color.b);

		for (j = 0; j < part->count; j++) {
		    chain = part->chain[j];

		    D_begin();
		    for (k = 0; k < chain->scount; k++) {
			xp = x0 + sx * chain->sx[k];
			yp = y0 - sy * chain->sy[k];
			if (k == 0)
			    D_move_abs(xp, yp);
			else
			    D_cont_abs(xp, yp);
		    }
		    D_end();
		    D_stroke();
		}
	    }
	    break;

	case S_STRING:
	    if (part->color.color == S_COL_NONE)
		break;
	    else if (part->color.color == S_COL_DEFAULT &&
		     string_color->a != RGBA_COLOR_NONE)
		D_RGB_color(string_color->r, string_color->g, string_color->b);
	    else
		D_RGB_color(part->color.r, part->color.g, part->color.b);

	    chain = part->chain[0];

	    D_begin();
	    for (j = 0; j < chain->scount; j++) {
		xp = x0 + sx * chain->sx[j];
		yp = y0 - sy * chain->sy[j];
		if (j == 0)
		    D_move_abs(xp, yp);
		else
		    D_cont_abs(xp, yp);
	    }
	    D_end();
	    D_stroke();
	    break;

	}			/* switch */
    }				/* for loop */
}

/*!
 * \brief draw a symbol at pixel coordinates
 *
 * Draws a symbol (one of $GISBASE/etc/symbols/) to the active display.
 * The starting x0,y0 coordinate corresponds to the center of the icon.
 * The symbol must be pre-processed with S_stroke() before being sent
 * to this function.
 *
 * \par Example
 * \code
 *   #include <grass/display.h>
 *   #include <grass/symbol.h>
 *   ...
 *   SYMBOL *Symb;
 *   Symb = S_read( symbol_name );
 *   S_stroke( Symb, size, rotation, tolerance );
 *   D_symbol( Symb, x0, y0, line_color, fill_color );
 * \endcode
 *
 *  \param Symb The symbol name (e.g. basic/circle)
 *  \param x0   The starting x display coordinate (pixel)
 *  \param y0   The starting y display coordinate (pixel)
 *  \param line_color  Outline color
 *  \param fill_color  Fill color
 *  \return void
 */

void D_symbol(const SYMBOL *Symb, double x0, double y0,
	      const RGBA_Color *line_color,
	      const RGBA_Color *fill_color)
{
    symbol(Symb, x0, y0, fill_color, line_color, line_color);
}


/*!
 * \brief draw a symbol at pixel coordinates (alternate)
 *
 * Draws a symbol (one of $GISBASE/etc/symbols/) to the active display.
 * The same as D_symbol(), but it uses a primary and secondary color
 * instead of line and fill color. The primary color is used to draw
 * stroke lines (STRINGs) and as the fill color for polygons. The secondary
 * color is used for polygon outlines.
 *
 *  \param Symb The symbol name (e.g. basic/circle)
 *  \param x0   The starting x display coordinate (pixel)
 *  \param y0   The starting y display coordinate (pixel)
 *  \param primary_color  Primary draw color
 *  \param secondary_color  Secondary draw color
 *  \return void
 */
void D_symbol2(const SYMBOL *Symb, double x0, double y0,
	       const RGBA_Color *primary_color,
	       const RGBA_Color *secondary_color)
{
    symbol(Symb, x0, y0, primary_color, secondary_color, primary_color);
}
