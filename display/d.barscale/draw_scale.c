/*
 * draw_scale() places a scalebar or a north arrow somewhere in the display frame
 */

#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "options.h"

#define NUMSCALES	16

/* declare variables */
static const struct scale
{
    char *name;
    double size;
    double limit;
    int seg;
} all_scales[2][NUMSCALES] = {
    {
	/* meters */
	{"", 0., 2., 10},
	{"1 meter", 1., 7., 10},
	{"5 meters", 5., 20., 5},
	{"10 meters", 10., 70., 10},
	{"50 meters", 50., 200., 5},
	{"100 meters", 100., 700., 10},
	{"500 meters", 500., 2000., 5},
	{"1 km", 1000., 7000., 10},
	{"5 km", 5000., 20000., 5},
	{"10 km", 10000., 70000., 10},
	{"50 km", 50000., 200000., 5},
	{"100 km", 100000., 700000., 10},
	{"500 km", 500000., 2000000., 5},
	{"1000 km", 1000000., 7000000., 10},
	{"5000 km", 5000000., 20000000., 5},
	{"10000 km", 10000000., 70000000., 10}
    },
    {   /* feet/miles */
	{"", 0.000, 1., 10},
	{"1 foot", 0.305, 2., 10},
	{"5 feet", 1.524, 10., 5},
	{"10 feet", 3.048, 20., 10},
	{"50 feet", 15.240, 100., 5},
	{"100 feet", 30.480, 200., 10},
	{"500 feet", 152.400, 1000., 5},
	{"1000 feet", 304.800, 2000., 10},
	{"1 mile", 1609.344, 10000., 5},
	{"5 miles", 8046.720, 20000., 5},
	{"10 miles", 16093.440, 100000., 10},
	{"50 miles", 80467.200, 200000., 5},
	{"100 miles", 160934.400, 1000000., 10},
	{"500 miles", 804672.000, 2000000., 5},
	{"1000 miles", 1609344.000, 10000000., 10},
	{"5000 miles", 8046720.000, 20000000., 5},
    }
};

int draw_scale(double east, double north, int style, int text_posn,
	       double fontsize, char *n_arrow_num)
{
    double meters;
    double line_len;
    int i, incr;
    double x_pos, y_pos;
    double t, b, l, r;
    double pt, pb, pl, pr; /* background box */
    double tt, tb, tl, tr; /* text box*/
    double xarr[5], yarr[5];
    double seg_len;
    const struct scale *scales = all_scales[use_feet];
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


    if (style == STYLE_NONE) {
	/* draw north arrow only */

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
	    case '2':
	    case '5':
	    case '6':
	    case '8':
		break;
	    default:
		G_fatal_error(_("Could not parse symbol"));
	    }
	}

	/* display the symbol */
	line_color = G_malloc(sizeof(RGBA_Color));
	fill_color = G_malloc(sizeof(RGBA_Color));

	if (D_color_number_to_RGB(fg_color, &R, &G, &B) == 0)
	    /* fall back to black on failure */
	    G_str_to_color(DEFAULT_FG_COLOR, &R, &G, &B);
	line_color->r = (unsigned char)R;
	line_color->g = (unsigned char)G;
	line_color->b = (unsigned char)B;
	line_color->a = RGBA_COLOR_OPAQUE;

	if (D_color_number_to_RGB(fg_color, &R, &G, &B) == 0)
	    /* fall back to black on failure */
	    G_str_to_color(DEFAULT_FG_COLOR, &R, &G, &B);
	fill_color->r = (unsigned char)R;
	fill_color->g = (unsigned char)G;
	fill_color->b = (unsigned char)B;
	fill_color->a = RGBA_COLOR_OPAQUE;

	if (n_arrow_num[0] == '2')
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
	    symbol_size = 17.;
	    break;
	default:
	    G_fatal_error(_("Could not parse symbol"));
	}

	x0 = D_d_to_u_col(x_pos);
	y0 = D_d_to_u_row(y_pos);

	strcpy(icon, "n_arrows/n_arrow");
	strncat(icon, n_arrow_num, 32);
	Symb = S_read(icon);
	S_stroke(Symb, symbol_size, 0.0, 0);
	D_symbol(Symb, x0, y0, line_color, fill_color);

	G_free(line_color);
	G_free(fill_color);

	return 0;
    }


    D_setup(0); /* back to regular coordinate settings */
    meters = D_get_u_east() - D_get_u_west();
    meters *= G_database_units_to_meters_factor();

    /* find the right scale */
    for (incr = 0; incr < NUMSCALES; incr++) {
	if (meters <= scales[incr].limit)
	    break;
    }

    if (!incr)
	return -1; /* use a G_fatal_error() here? */

    line_len = D_get_u_to_d_xconv() * scales[incr].size
	/ G_database_units_to_meters_factor();
    seg_len = line_len / scales[incr].seg;
    /* work around round off */
    line_len = seg_len * scales[incr].seg;

    D_setup_unity(0);

    if (do_background) {
	/* Blank out area with background color */
	D_get_text_box(scales[incr].name, &tt, &tb, &tl, &tr);

	if (text_posn == TEXT_OVER) {
	    pr = x_pos + 35 + line_len;
	    pl = x_pos + 0;
	    pt = y_pos + tb - 5;
	    pb = y_pos + 30;
	    if (style != STYLE_CLASSIC_BAR && style != STYLE_THIN_WITH_ENDS)
		pl += 15;
	    if (style == STYLE_TICKS_DOWN)
		pb += 12;
	}
	if (text_posn == TEXT_UNDER) {
	    pr = x_pos + 35 + line_len;
	    pl = x_pos + 0;
	    pt = y_pos + 0;
	    pb = y_pos + 30 - tb + 5;
	    if (style != STYLE_CLASSIC_BAR && style != STYLE_THIN_WITH_ENDS)
		pl += 15;
	    if (style == STYLE_TICKS_UP)
		pt -= 12;
	}
	else if (text_posn == TEXT_RIGHT){
	    pr = x_pos + 35 + line_len + tr + 5;
	    pl = x_pos + 0;
	    pt = y_pos + 0;
	    pb = y_pos + 30;
	    if (style == STYLE_TICKS_UP) {
		pt -= 12;
		pl += 15;
	    }
	    if (style == STYLE_TICKS_DOWN) {
		pb += 12;
		pl += 15;
	    }
	}
	else if (text_posn == TEXT_LEFT){
	    pr = x_pos + 35 + line_len;
	    pl = x_pos - tr - 13;
	    pt = y_pos + 0;
	    pb = y_pos + 30;
	    if (style == STYLE_TICKS_UP)
		pt -= 12;
	    if (style == STYLE_TICKS_DOWN)
		pb += 11;
	}

	if (fontsize < 0) {  /* no text */
	    switch (style) {
	    case STYLE_CLASSIC_BAR:
	    case STYLE_THIN_WITH_ENDS:
		pr = x_pos + 35 + line_len;
		pl = x_pos + 0;
		pt = y_pos + 0;
		pb = y_pos + 30;
		break;
	    case STYLE_PART_CHECKER:
	    case STYLE_FULL_CHECKER:
	    case STYLE_SOLID_BAR:
	    case STYLE_HOLLOW_BAR:
	    case STYLE_TICKS_BOTH:
	    case STYLE_ARROW_ENDS:
		pr = x_pos + 35 + line_len;
		pl = x_pos + 15;
		pt = y_pos + 0;
		pb = y_pos + 30;
		break;
	    case STYLE_TICKS_UP:
		pr = x_pos + 35 + line_len;
		pl = x_pos + 15;
		pt = y_pos - 12;
		pb = y_pos + 25;
		break;
	    case STYLE_TICKS_DOWN:
		pr = x_pos + 35 + line_len;
		pl = x_pos + 15;
		pt = y_pos + 3;
		pb = y_pos + 40;
		break;
	    default:
		G_fatal_error(_("Programmer error"));
	    }
	}

	/* keep it on the screen */
	if (pt < t)
	    pt = t;
	if (pb > b)
	    pb = b;
	if (pl < l)
	    pl = l;
	if (pr > r)
	    pr = r;

	D_use_color(bg_color);
	D_box_abs(pl, pt, pr, pb);
    }

    /* Draw the small N with an arrow through it on the left side */
    D_use_color(fg_color);
    if (style == STYLE_CLASSIC_BAR || style == STYLE_THIN_WITH_ENDS) {
	D_begin();
	D_move_abs(x_pos + 5, y_pos + 20);
	D_cont_rel(0, -10);
	D_cont_rel(10, 10);
	D_cont_rel(0, -10);
	D_move_rel(-5, 14);
	D_cont_rel(0, -17);
	D_cont_rel(-2.5, -0);
	D_cont_rel(2.5, -4);
	D_cont_rel(2.5, 4);
	D_cont_rel(-2.5, -0);
	D_close();
	D_end();
	D_stroke();
    }

    /* The end points of the center-line are (x_pos + 25, y_pos + 15)
	and (x_pos + 25 + line_len, y_pos + 15) */
    if (style == STYLE_CLASSIC_BAR) {
	D_begin();
	D_move_abs(x_pos + 25, y_pos + 17);
	/* actual width is line_len-1+1=line_len and height is 4+1=5 */
	D_cont_rel(line_len - 1, 0);
	D_cont_rel(0, -4);
	D_cont_rel(-line_len + 1, 0);
	D_cont_rel(0, 4);
	D_end();
	D_stroke();

	D_pos_rel(0, 1);
	for (i = 1; i <= scales[incr].seg; i += 2) {
	    /* width is seg_len and height is 5 */
	    D_box_rel(seg_len, -5);
	    D_pos_rel(seg_len * 2, 0);
	}
    }
    else if (style == STYLE_THIN_WITH_ENDS) {
	/* draw simple line scale */
	D_begin();
	D_move_abs(x_pos + 25, y_pos + 5);
	D_cont_abs(x_pos + 25, y_pos + 25);
	D_move_abs(x_pos + 25, y_pos + 15);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_move_abs(x_pos + 25 + line_len, y_pos + 5);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 25);
	D_close();
	D_end();  /* no-op? */
    }
    else if (style == STYLE_SOLID_BAR) {
	/* draw simple solid-bar scale */
	xarr[0] = 0;
	yarr[0] = +8;
	xarr[1] = line_len;
	yarr[1] = 0;
	xarr[2] = 0;
	yarr[2] = -8;
	xarr[3] = -line_len;
	yarr[3] = 0;
	xarr[4] = 0;
	yarr[4] = +8;

	D_move_abs(x_pos + 25, y_pos + 15 - 4);
	D_polygon_rel(xarr, yarr, 5);
    }
    else if (style == STYLE_HOLLOW_BAR) {
	/* draw hollow-bar scale */
	D_use_color(fg_color);
	D_begin();
	D_move_abs(x_pos + 25, y_pos + 15 - 4);
	D_cont_rel(0, +8);
	D_cont_rel(line_len, 0);
	D_cont_rel(0, -8);
	D_cont_rel(-line_len, 0);
	D_cont_rel(0, +8);
	D_close();
	D_end();  /* no-op? */
    }
    else if (style == STYLE_FULL_CHECKER) {
	D_begin();
	D_move_abs(x_pos + 25, y_pos + 15 + 6);
	/* actual width is line_len-1+1=line_len and height is 7+1=8 */
	D_cont_rel(line_len, 0);
	D_cont_rel(0, -12);
	D_cont_rel(-line_len, 0);
	D_cont_rel(0, +12);
	D_close();
	D_end();  /* no-op? */
	D_stroke();

	D_pos_rel(0, -6);
	for (i = 1; i <= scales[incr].seg; i++) {
	    xarr[0] = 0;		yarr[0] = 0;
	    xarr[1] = seg_len;	yarr[1] = 0;
	    xarr[2] = 0;		yarr[2] = (i % 2 ? -6 : 6);
	    xarr[3] = -seg_len;	yarr[3] = 0;
	    xarr[4] = 0;		yarr[4] = (i % 2 ? 6 : -6);
	    /* width is seg_len and height is 4 */
	    D_polygon_rel(xarr, yarr, 5);
	    D_pos_rel(seg_len, 0);
	}
    }
    else if (style == STYLE_TICKS_BOTH) {
	/* draw simple line scale with corssing ticks */
	D_move_abs(x_pos + 25, y_pos + 5);
	D_cont_abs(x_pos + 25, y_pos + 25);
	D_move_abs(x_pos + 25, y_pos + 15);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_move_abs(x_pos + 25 + line_len, y_pos + 5);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 25);

	D_move_abs(x_pos + 25, y_pos + 15);
	D_move_rel(0, +6);
	for (i = 0; i <= scales[incr].seg - 2; i++) {
	    D_move_rel(seg_len, 0);
	    D_cont_rel(0, -11); /* 5 above, on px on line, and 5 below */
	    D_move_rel(0, +11);
	}
    }
    else if (style == STYLE_TICKS_UP) {
	/* draw simple line scale with up facing ticks */
	D_begin();
	D_move_abs(x_pos + 25, y_pos - 2);
	D_cont_abs(x_pos + 25, y_pos + 15);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_move_abs(x_pos + 25 + line_len, y_pos - 2);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_end();  /* no-op? */
	D_close();

	D_move_abs(x_pos + 25, y_pos + 15);
	for (i = 0; i <= scales[incr].seg - 2; i++) {
	    D_move_rel(seg_len, 0);
	    D_cont_rel(0, -7); /* 5 above, on px on line, and 5 below */
	    D_move_rel(0, +7);
	}
    }
    else if (style == STYLE_TICKS_DOWN) {
	/* draw simple line scale with down facing ticks */
	D_begin();
	D_move_abs(x_pos + 25, y_pos + 15 + 17);
	D_cont_abs(x_pos + 25, y_pos + 15);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_move_abs(x_pos + 25 + line_len, y_pos +15 + 17);
	D_cont_abs(x_pos + 25 + line_len, y_pos + 15);
	D_end();  /* no-op? */
	D_close();

	D_move_abs(x_pos + 25, y_pos + 15);
	for (i = 0; i <= scales[incr].seg - 2; i++) {
	    D_move_rel(seg_len, 0);
	    D_cont_rel(0, +7); /* 5 above, on px on line, and 5 below */
	    D_move_rel(0, -7);
	}
    }
    D_stroke();

    if (fontsize < 0)
	return 0;

    /* draw the distance + units text */
    if (text_posn == TEXT_OVER) {
	D_pos_abs(x_pos + 25 + line_len / 2.
		  - strlen(scales[incr].name) * fontsize * 0.81 / 2,
		  y_pos);
	D_text(scales[incr].name);
    }
    if (text_posn == TEXT_UNDER) {
	D_pos_abs(x_pos + 25 + line_len / 2.
		  - strlen(scales[incr].name) * fontsize * 0.81 / 2,
		  y_pos + 43);
	D_text(scales[incr].name);
    }
    else if (text_posn == TEXT_RIGHT) {
	D_pos_abs(x_pos + 35 + line_len, y_pos + 20);
	D_text(scales[incr].name);
    }
    else if (text_posn == TEXT_LEFT) {
	D_pos_abs(x_pos - 24 -
		  strlen(scales[incr].name) * fontsize/2, y_pos + 20);
	D_text(scales[incr].name);
    }

    return 0;
}
