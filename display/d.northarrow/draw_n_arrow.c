/*
 * draw_n_arrow() places a north arrow somewhere in the display frame
 */

#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "options.h"

int draw_n_arrow(double east, double north, double fontsize,
		 char *n_arrow_num, double line_width)
{
    double x_pos, y_pos;
    double t, b, l, r;
    double tt, tb, tl, tr; /* text box*/

    SYMBOL *Symb;
    RGBA_Color *line_color, *fill_color;
    int R, G, B;
    double x0, y0;
    char icon[64];
    double symbol_size;


    /* Establish text size */
    if (fontsize > 0)
	D_text_size(fontsize, fontsize);

    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    x_pos = east * (r - l) / 100.;
    y_pos = (100. - north) * (b - t) / 100.;

    if (line_width > 0)
	D_line_width(line_width);

    if (fontsize > 0) {
    	/* draw the "N" */
    	D_get_text_box("N", &tt, &tb, &tl, &tr);
    	D_use_color(fg_color);

    	/* positions manually tuned */
    	switch (n_arrow_num[0]) {
    	case '1':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 45);
    	    D_text("N");
    	    break;
    	case '3':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 60);
    	    D_text("N");
    	    break;
    	case '4':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 45);
    	    D_text("N");
    	    break;
    	case '7':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 70);
    	    D_text("N");
    	    break;
    	case '9':
	case 'f':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 55);
    	    D_text("N");
	    break;
    	case 'c':
    	    D_pos_abs(x_pos - (tr + tl) / 2, y_pos - 48.5);
    	    D_text("N");
    	    break;
    	case '2':
    	case '5':
    	case '6':
    	case '8':
    	    break;
    	default:
    	    G_fatal_error(_("Could not parse symbol"));
    	}
    }

    /* display the north arrow symbol */
    line_color = G_malloc(sizeof(RGBA_Color));
    fill_color = G_malloc(sizeof(RGBA_Color));

    if (D_color_number_to_RGB(fg_color, &R, &G, &B) == 0)
    	/* fall back to black on failure */
    	G_str_to_color(DEFAULT_FG_COLOR, &R, &G, &B);
    line_color->r = (unsigned char)R;
    line_color->g = (unsigned char)G;
    line_color->b = (unsigned char)B;
    line_color->a = RGBA_COLOR_OPAQUE;

    if (D_color_number_to_RGB(bg_color, &R, &G, &B) == 0)
    	/* fall back to black on failure */
    	G_str_to_color(DEFAULT_FG_COLOR, &R, &G, &B);
    fill_color->r = (unsigned char)R;
    fill_color->g = (unsigned char)G;
    fill_color->b = (unsigned char)B;
    fill_color->a = RGBA_COLOR_OPAQUE;

    if (n_arrow_num[0] == '2' || n_arrow_num[0] == '9')
    	fill_color->a = RGBA_COLOR_TRANSPARENT;

    /* sizes manually tuned */
    switch (n_arrow_num[0]) {
    case '1':
    	symbol_size = 35.;
    	break;
    case '2':
    	symbol_size = 19.;
    	break;
    case '3':
    	symbol_size = 20.;
    	break;
    case '4':
    	symbol_size = 15.;
    	break;
    case '5':
    case '6':
    	symbol_size = 14.;
    	break;
    case '7':
    	symbol_size = 23.;
    	break;
    case '8':
    case '9':
    	symbol_size = 17.;
    	break;
    case 'c':
    	symbol_size = 80.;
    	break;
    case 'f':
    	symbol_size = 100.;
    	break;
    default:
    	G_fatal_error(_("Could not parse symbol"));
    }

    x0 = D_d_to_u_col(x_pos);
    y0 = D_d_to_u_row(y_pos);

    if (n_arrow_num[0] == 'c')
	strcpy(icon, "n_arrows/compass");
    else if (n_arrow_num[0] == 'f')
	strcpy(icon, "n_arrows/fancy_compass");
    else {
	strcpy(icon, "n_arrows/n_arrow");
	strncat(icon, n_arrow_num, 32);
    }

    Symb = S_read(icon);

    if(!Symb)
    	G_fatal_error(_("Could not read symbol \"%s\""), icon);

    S_stroke(Symb, symbol_size, 0.0, 0);
    D_symbol(Symb, x0, y0, line_color, fill_color);


    if (line_width > 0)
	D_line_width(0);

    G_free(Symb);
    G_free(line_color);
    G_free(fill_color);

    return 0;
}
