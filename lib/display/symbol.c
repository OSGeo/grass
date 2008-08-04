
/****************************************************************************
 *
 * MODULE:       display
 * AUTHOR(S):    Hamish Bowman <hamish_nospam yahoo.com> (original contributor)
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
#include <grass/raster.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

/*!
 * \brief draw a symbol at pixel coordinates
 *
 * Draws a symbol (one of $GISBASE/etc/symbols/) to the active display.
 * The starting x0,y0 coordinate corresponds to the center of the icon.
 * The symbol must be pre-processed with S_stroke() before being sent
 * to this function.
 *
 * example:
 *   #include <grass/display.h>
 *   #include <grass/symbol.h>
 *   SYMBOL *Symb;
 *   Symb = S_read( symbol_name );
 *   S_stroke( Symb, size, rotation, tolerance );
 *   D_symbol( Symb, x0, y0, line_color, fill_color );
 *
 *  \param Symb The symbol name (e.g. basic/circle)
 *  \param x0   The starting x display coordinate (pixel)
 *  \param y0   The starting y display coordinate (pixel)
 *  \param line_color  Outline color
 *  \param fill_color  Fill color
 *  \return void
 */

void D_symbol(const SYMBOL * Symb, int x0, int y0,
	      const RGBA_Color * line_color, const RGBA_Color * fill_color)
{
    int i, j, k;
    const SYMBPART *part;
    const SYMBCHAIN *chain;
    int xp, yp;
    int *x, *y;


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
		    R_RGB_color(fill_color->r, fill_color->g, fill_color->b);
		else
		    R_RGB_color(part->fcolor.r, part->fcolor.g,
				part->fcolor.b);

		for (j = 0; j < part->count; j++) {	/* for each component polygon */
		    chain = part->chain[j];

		    x = G_malloc(sizeof(int) * chain->scount);
		    y = G_malloc(sizeof(int) * chain->scount);

		    for (k = 0; k < chain->scount; k++) {
			x[k] = x0 + chain->sx[k];
			y[k] = y0 - chain->sy[k];
		    }
		    R_polygon_abs(x, y, chain->scount);

		    G_free(x);
		    G_free(y);
		}

	    }
	    /* again, to draw the lines */
	    if ((part->color.color == S_COL_DEFAULT &&
		 line_color->a != RGBA_COLOR_NONE) ||
		part->color.color == S_COL_DEFINED) {
		if (part->color.color == S_COL_DEFAULT) {
		    R_RGB_color(line_color->r, line_color->g, line_color->b);
		}
		else
		    R_RGB_color(part->color.r, part->color.g, part->color.b);

		for (j = 0; j < part->count; j++) {
		    chain = part->chain[j];

		    for (k = 0; k < chain->scount; k++) {
			xp = x0 + chain->sx[k];
			yp = y0 - chain->sy[k];
			if (k == 0)
			    R_move_abs(xp, yp);
			else
			    R_cont_abs(xp, yp);
		    }
		}
	    }
	    break;

	case S_STRING:
	    if (part->color.color == S_COL_NONE)
		break;
	    else if (part->color.color == S_COL_DEFAULT &&
		     line_color->a != RGBA_COLOR_NONE)
		R_RGB_color(line_color->r, line_color->g, line_color->b);
	    else
		R_RGB_color(part->color.r, part->color.g, part->color.b);

	    chain = part->chain[0];

	    for (j = 0; j < chain->scount; j++) {
		xp = x0 + chain->sx[j];
		yp = y0 - chain->sy[j];
		if (j == 0)
		    R_move_abs(xp, yp);
		else
		    R_cont_abs(xp, yp);
	    }
	    break;

	}			/* switch */
    }				/* for loop */
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
void D_symbol2(const SYMBOL * Symb, int x0, int y0,
	       const RGBA_Color * primary_color,
	       const RGBA_Color * secondary_color)
{
    /* TODO: merge duplicate D_symbol() code into common lib fns */
    int i, j, k;
    const SYMBPART *part;
    const SYMBCHAIN *chain;
    int xp, yp;
    int *x, *y;


    G_debug(2, "D_symbol(): %d parts", Symb->count);

    for (i = 0; i < Symb->count; i++) {
	part = Symb->part[i];

	switch (part->type) {

	case S_POLYGON:
	    /* draw background fills */
	    if ((part->fcolor.color == S_COL_DEFAULT &&
		 primary_color->a != RGBA_COLOR_NONE) ||
		part->fcolor.color == S_COL_DEFINED) {
		if (part->fcolor.color == S_COL_DEFAULT)
		    R_RGB_color(primary_color->r, primary_color->g,
				primary_color->b);
		else
		    R_RGB_color(part->fcolor.r, part->fcolor.g,
				part->fcolor.b);

		for (j = 0; j < part->count; j++) {	/* for each component polygon */
		    chain = part->chain[j];

		    x = G_malloc(sizeof(int) * chain->scount);
		    y = G_malloc(sizeof(int) * chain->scount);

		    for (k = 0; k < chain->scount; k++) {
			x[k] = x0 + chain->sx[k];
			y[k] = y0 - chain->sy[k];
		    }
		    R_polygon_abs(x, y, chain->scount);

		    G_free(x);
		    G_free(y);
		}

	    }
	    /* again, to draw the lines */
	    if ((part->color.color == S_COL_DEFAULT &&
		 secondary_color->a != RGBA_COLOR_NONE) ||
		part->color.color == S_COL_DEFINED) {
		if (part->color.color == S_COL_DEFAULT) {
		    R_RGB_color(secondary_color->r, secondary_color->g,
				secondary_color->b);
		}
		else
		    R_RGB_color(part->color.r, part->color.g, part->color.b);

		for (j = 0; j < part->count; j++) {
		    chain = part->chain[j];

		    for (k = 0; k < chain->scount; k++) {
			xp = x0 + chain->sx[k];
			yp = y0 - chain->sy[k];
			if (k == 0)
			    R_move_abs(xp, yp);
			else
			    R_cont_abs(xp, yp);
		    }
		}
	    }
	    break;

	case S_STRING:
	    if (part->color.color == S_COL_NONE)
		break;
	    else if (part->color.color == S_COL_DEFAULT &&
		     primary_color->a != RGBA_COLOR_NONE)
		R_RGB_color(primary_color->r, primary_color->g,
			    primary_color->b);
	    else
		R_RGB_color(part->color.r, part->color.g, part->color.b);

	    chain = part->chain[0];

	    for (j = 0; j < chain->scount; j++) {
		xp = x0 + chain->sx[j];
		yp = y0 - chain->sy[j];
		if (j == 0)
		    R_move_abs(xp, yp);
		else
		    R_cont_abs(xp, yp);
	    }
	    break;

	}			/* switch */
    }				/* for loop */
}
