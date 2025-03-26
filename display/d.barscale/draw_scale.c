/*
 * draw_scale() places a scalebar somewhere in the display frame
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "options.h"

#define NUMSCALES 16

/* declare variables */
static const struct scale {
    char *symbol;
    double size;
    double limit;
} all_scales[2][NUMSCALES] = {{/* meters */
                               {"", 0., 2.},
                               {"1 m", 1., 7.},
                               {"5 m", 5., 20.},
                               {"10 m", 10., 70.},
                               {"50 m", 50., 200.},
                               {"100 m", 100., 700.},
                               {"500 m", 500., 2000.},
                               {"1 km", 1000., 7000.},
                               {"5 km", 5000., 20000.},
                               {"10 km", 10000., 70000.},
                               {"50 km", 50000., 200000.},
                               {"100 km", 100000., 700000.},
                               {"500 km", 500000., 2000000.},
                               {"1000 km", 1000000., 7000000.},
                               {"5000 km", 5000000., 20000000.},
                               {"10000 km", 10000000., 70000000.}},
                              {
                                  /* feet/miles */
                                  {"", 0.000, 1.},
                                  {"1 ft", 0.305, 2.},
                                  {"5 ft", 1.524, 10.},
                                  {"10 ft", 3.048, 20.},
                                  {"50 ft", 15.240, 100.},
                                  {"100 ft", 30.480, 200.},
                                  {"500 ft", 152.400, 1000.},
                                  {"1000 ft", 304.800, 2000.},
                                  {"1 mi", 1609.344, 10000.},
                                  {"5 mi", 8046.720, 20000.},
                                  {"10 mi", 16093.440, 100000.},
                                  {"50 mi", 80467.200, 200000.},
                                  {"100 mi", 160934.400, 1000000.},
                                  {"500 mi", 804672.000, 2000000.},
                                  {"1000 mi", 1609344.000, 10000000.},
                                  {"5000 mi", 8046720.000, 20000000.},
                              }};

int draw_scale(double east, double north, int length, int seg, int units,
               char *label_cstm, int style, int text_posn, double width_scale,
               double fontsize)
{
    double meters;
    double line_len;
    int i, incr;
    double x_pos, y_pos;
    double t, b, l, r;
    double pt = 0.0, pb = 0.0, pl = 0.0, pr = 0.0; /* background box */
    double tt, tb, tl, tr;                         /* text box */
    double xarr[5], yarr[5];
    double seg_len;
    const struct scale *scales = all_scales[use_feet];
    SYMBOL *Symb;
    RGBA_Color *line_color, *fill_color;
    int R, G, B;
    double x0, y0;
    double symbol_size;
    char *label;
    double size;
    double xspace_bf_N, x_pos_start, xspace_around_line, xsize_N;
    double ysize, ysize_solid, ysize_checker, ysize_ticks;

    /* Establish text size */
    if (fontsize > 0)
        D_text_size(fontsize, fontsize);

    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    x_pos = l + (int)(east * (r - l) / 100.);
    y_pos = t + (int)((100. - north) * (b - t) / 100.);

    D_setup(0); /* back to regular coordinate settings */
    meters = D_get_u_east() - D_get_u_west();
    meters *= G_database_units_to_meters_factor();

    /* find the right scale only if length is not given by user(length=0) */
    if (length == 0) {
        for (incr = 0; incr < NUMSCALES; incr++) {
            if (meters <= scales[incr].limit)
                break;
        }

        /* region is too small to draw anything. ever reached? */
        if (!incr)
            return -1;

        /* beyond the maximum just make the longest scale narrower */
        if (incr >= NUMSCALES)
            incr = NUMSCALES - 1;

        label = scales[incr].symbol;
        size = scales[incr].size;
    }
    /* length given by user */
    else {
        label = G_malloc(GNAME_MAX);
        size = length / G_meters_to_units_factor(units);
        sprintf(label, "%d %s", length, label_cstm);
    }

    line_len =
        D_get_u_to_d_xconv() * size / G_database_units_to_meters_factor();

    seg_len = line_len / seg;
    /* work around round off */
    line_len = seg_len * seg;

    D_setup_unity(0);

    /* setup size variables */
    xspace_bf_N = 5 * width_scale;
    xspace_around_line = 10 * width_scale;
    xsize_N = 10 * width_scale;
    x_pos_start =
        x_pos + (north_arrow ? (xspace_bf_N + xsize_N + xspace_around_line)
                             : xspace_around_line);
    ysize = 30 * width_scale;
    ysize_solid = 8 * width_scale;
    ysize_checker = 6 * width_scale;
    ysize_ticks = 20 * width_scale; /* the length of the edge tick */

    if (do_background) {
        /* Blank out area with background color */
        D_get_text_box(label, &tt, &tb, &tl, &tr);
        pl = x_pos + 0;
        pr = x_pos + line_len + 2 * xspace_around_line +
             (north_arrow ? xspace_bf_N + xsize_N : 0);
        if (text_posn == TEXT_OVER) {
            pt = y_pos + tb - 5 * width_scale;
            pb = y_pos + ysize;
            if (style == STYLE_TICKS_DOWN)
                pb += 12 * width_scale;
        }
        if (text_posn == TEXT_UNDER) {
            pt = y_pos + 0;
            pb = y_pos + ysize - tb + 5 * width_scale;
            if (style == STYLE_TICKS_UP)
                pt -= 12 * width_scale;
        }
        else if (text_posn == TEXT_RIGHT) {
            pr = pr + tr + xspace_around_line;
            pt = y_pos + 0;
            pb = y_pos + ysize;
            if (style == STYLE_TICKS_UP) {
                pt -= 12 * width_scale;
                pb -= 6 * width_scale;
            }
            if (style == STYLE_TICKS_DOWN) {
                pt += 4 * width_scale;
                pb += 12 * width_scale;
            }
        }
        else if (text_posn == TEXT_LEFT) {
            pl = x_pos - tr - 13 * width_scale;
            pt = y_pos + 0;
            pb = y_pos + ysize;
            if (style == STYLE_TICKS_UP) {
                pt -= 12 * width_scale;
                pb -= 4 * width_scale;
            }
            if (style == STYLE_TICKS_DOWN) {
                pt += 3 * width_scale;
                pb += 11 * width_scale;
            }
        }

        if (fontsize < 0) { /* no text */
            pl = x_pos + 0;
            pr = x_pos + line_len + 2 * xspace_around_line +
                 (north_arrow ? xspace_bf_N + xsize_N : 0);
            switch (style) {
            case STYLE_CLASSIC_BAR:
            case STYLE_THIN_WITH_ENDS:
            case STYLE_PART_CHECKER:
            case STYLE_FULL_CHECKER:
            case STYLE_MIXED_CHECKER:
            case STYLE_TAIL_CHECKER:
            case STYLE_SOLID_BAR:
            case STYLE_HOLLOW_BAR:
            case STYLE_TICKS_BOTH:
            case STYLE_ARROW_ENDS:
                pt = y_pos + 0;
                pb = y_pos + ysize;
                break;
            case STYLE_TICKS_UP:
                pt = y_pos - 12 * width_scale;
                pb = y_pos + 25 * width_scale;
                break;
            case STYLE_TICKS_DOWN:
                pt = y_pos + 3 * width_scale;
                pb = y_pos + 40 * width_scale;
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

    /* Draw the small N with an arrow through it on the left side for the
     * classic barscale styles */
    D_use_color(fg_color);
    if (north_arrow) {
        D_begin();
        D_move_abs(x_pos + xspace_bf_N, y_pos + 2 * ysize / 3);
        D_cont_rel(0, -10 * width_scale);
        D_cont_rel(10 * width_scale, 10 * width_scale);
        D_cont_rel(0, -10 * width_scale);
        D_move_rel(-5 * width_scale, 14 * width_scale);
        D_cont_rel(0, -17 * width_scale);
        D_cont_rel(-2.5 * width_scale, -0);
        D_cont_rel(2.5 * width_scale, -4 * width_scale);
        D_cont_rel(2.5 * width_scale, 4 * width_scale);
        D_cont_rel(-2.5 * width_scale, -0);
        D_close();
        D_end();
        D_stroke();
    }

    /* The end points of the center-line are (x_pos + 25, y_pos + 15)
       and (x_pos + 25 + line_len, y_pos + 15) */
    if (style == STYLE_CLASSIC_BAR) {
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + ysize_checker / 2);
        /* actual width is line_len-1+1=line_len */
        D_cont_rel(line_len - 1, 0);
        D_cont_rel(0, -ysize_checker);
        D_cont_rel(-line_len + 1, 0);
        D_cont_rel(0, ysize_checker);
        D_end();
        D_close();
        D_stroke();

        for (i = 1; i <= seg; i += 2) {
            /* width is seg_len and height is 5 */
            D_box_rel(seg_len, -ysize_checker);
            D_pos_rel(seg_len * 2, 0);
        }
    }
    else if (style == STYLE_THIN_WITH_ENDS) {
        /* draw simple line scale */
        D_begin();
        D_move_abs(x_pos_start, y_pos + (ysize - ysize_ticks) / 2);
        D_cont_abs(x_pos_start, y_pos + ysize - (ysize - ysize_ticks) / 2);
        D_move_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_move_abs(x_pos_start + line_len, y_pos + (ysize - ysize_ticks) / 2);
        D_cont_abs(x_pos_start + line_len,
                   y_pos + ysize - (ysize - ysize_ticks) / 2);
        D_close();
        D_end(); /* no-op? */
    }
    else if (style == STYLE_SOLID_BAR) {
        /* draw simple solid-bar scale */
        xarr[0] = 0;
        yarr[0] = ysize_solid;
        xarr[1] = line_len;
        yarr[1] = 0;
        xarr[2] = 0;
        yarr[2] = -ysize_solid;
        xarr[3] = -line_len;
        yarr[3] = 0;
        xarr[4] = 0;
        yarr[4] = ysize_solid;

        D_move_abs(x_pos_start, y_pos + ysize / 2 - ysize_solid / 2);
        D_polygon_rel(xarr, yarr, 5);
    }
    else if (style == STYLE_HOLLOW_BAR) {
        /* draw hollow-bar scale */
        D_use_color(fg_color);
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 - ysize_solid / 2);
        D_cont_rel(0, ysize_solid);
        D_cont_rel(line_len, 0);
        D_cont_rel(0, -ysize_solid);
        D_cont_rel(-line_len, 0);
        D_cont_rel(0, ysize_solid);
        D_close();
        D_end(); /* no-op? */
    }
    else if (style == STYLE_FULL_CHECKER) {
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + ysize_checker);
        /* actual width is line_len-1+1=line_len and height is 7+1=8 */
        D_cont_rel(line_len, 0);
        D_cont_rel(0, -2 * ysize_checker);
        D_cont_rel(-line_len, 0);
        D_cont_rel(0, +2 * ysize_checker);
        D_close();
        D_end(); /* no-op? */
        D_stroke();

        D_pos_rel(0, -ysize_checker);
        for (i = 1; i <= seg; i++) {
            xarr[0] = 0;
            yarr[0] = 0;
            xarr[1] = seg_len;
            yarr[1] = 0;
            xarr[2] = 0;
            yarr[2] = (i % 2 ? -ysize_checker : ysize_checker);
            xarr[3] = -seg_len;
            yarr[3] = 0;
            xarr[4] = 0;
            yarr[4] = (i % 2 ? ysize_checker : -ysize_checker);
            /* width is seg_len and height is 6 */
            D_polygon_rel(xarr, yarr, 5);
            D_pos_rel(seg_len, 0);
        }
    }
    else if (style == STYLE_PART_CHECKER) {
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + ysize_checker);
        /* actual width is line_len-1+1=line_len and height is 7+1=8 */
        D_cont_rel(line_len, 0);
        D_cont_rel(0, -2 * ysize_checker);
        D_cont_rel(-line_len, 0);
        D_cont_rel(0, +2 * ysize_checker);
        D_close();
        D_end(); /* no-op? */
        D_stroke();

        D_pos_rel(0, -ysize_checker);
        for (i = 1; i <= seg; i++) {
            if (i <= (seg == 5 ? 2 : 4)) {
                xarr[0] = 0;
                yarr[0] = 0;
                xarr[1] = seg_len / 2.;
                yarr[1] = 0;
                xarr[2] = 0;
                yarr[2] = -ysize_checker;
                xarr[3] = -seg_len / 2.;
                yarr[3] = 0;
                xarr[4] = 0;
                yarr[4] = ysize_checker;
                D_polygon_rel(xarr, yarr, 5);
                D_pos_rel(seg_len / 2., 0);

                xarr[0] = 0;
                yarr[0] = 0;
                xarr[1] = seg_len / 2.;
                yarr[1] = 0;
                xarr[2] = 0;
                yarr[2] = ysize_checker;
                xarr[3] = -seg_len / 2.;
                yarr[3] = 0;
                xarr[4] = 0;
                yarr[4] = -ysize_checker;
                D_polygon_rel(xarr, yarr, 5);
                D_pos_rel(seg_len / 2., 0);
            }
            else {
                xarr[0] = 0;
                yarr[0] = 0;
                xarr[1] = seg_len;
                yarr[1] = 0;
                xarr[2] = 0;
                yarr[2] = (i % 2 ? -ysize_checker : ysize_checker);
                xarr[3] = -seg_len;
                yarr[3] = 0;
                xarr[4] = 0;
                yarr[4] = (i % 2 ? ysize_checker : -ysize_checker);
                /* width is seg_len and height is 6 */
                D_polygon_rel(xarr, yarr, 5);
                D_pos_rel(seg_len, 0);
            }
        }
    }
    else if (style == STYLE_MIXED_CHECKER) {
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + ysize_checker);
        /* actual width is line_len-1+1=line_len and height is 7+1=8 */
        D_cont_rel(line_len, 0);
        D_cont_rel(0, -2 * ysize_checker);
        D_cont_rel(-line_len, 0);
        D_cont_rel(0, +2 * ysize_checker);

        /* horizontal line across the middle to separate white from white */
        D_move_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_rel(line_len, 0);
        D_end(); /* no-op? */
        D_close();
        D_stroke();

        D_move_abs(x_pos_start, y_pos + ysize / 2);

        for (i = 1; i <= seg; i++) {
            if (i <= (seg == 5 ? 2 : 6)) {
                if (i % 2 == 0) {
                    xarr[0] = 0;
                    yarr[0] = 0;
                    xarr[1] = seg_len;
                    yarr[1] = 0;
                    xarr[2] = 0;
                    yarr[2] = -ysize_checker;
                    xarr[3] = -seg_len;
                    yarr[3] = 0;
                    xarr[4] = 0;
                    yarr[4] = +ysize_checker;
                    D_polygon_rel(xarr, yarr, 5);
                }

                xarr[0] = 0;
                yarr[0] = 0;
                xarr[1] = seg_len / 2.;
                yarr[1] = 0;
                xarr[2] = 0;
                yarr[2] = +ysize_checker;
                xarr[3] = -seg_len / 2.;
                yarr[3] = 0;
                xarr[4] = 0;
                yarr[4] = -ysize_checker;
                D_pos_rel(seg_len / 2., 0);
                D_polygon_rel(xarr, yarr, 5);
                D_pos_rel(seg_len / 2., 0);
            }
            else {
                xarr[0] = 0;
                yarr[0] = 0;
                xarr[1] = seg_len;
                yarr[1] = 0;
                xarr[2] = 0;
                yarr[2] = +ysize_checker;
                xarr[3] = -seg_len;
                yarr[3] = 0;
                xarr[4] = 0;
                yarr[4] = (i % 2 ? -ysize_checker : ysize_checker);
                /* width is seg_len and height is 6 */
                D_polygon_rel(xarr, yarr, 5);
                D_pos_rel(seg_len, -ysize_checker);
            }
        }
    }
    else if (style == STYLE_TAIL_CHECKER) {
        /* first draw outside box */
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + ysize_checker);
        D_cont_rel(line_len, 0);
        D_cont_rel(0, -2 * ysize_checker);
        D_cont_rel(-line_len, 0);
        D_cont_rel(0, +2 * ysize_checker);
        D_close();
        D_end(); /* no-op? */
        D_stroke();

        D_pos_rel(0, -ysize_checker);
        for (i = 1; i <= (seg == 5 ? 3 : 5); i++) {
            /* width is seg_len and height is 6 */
            xarr[0] = 0;
            yarr[0] = 0;
            xarr[1] = seg_len;
            yarr[1] = 0;
            xarr[2] = 0;
            yarr[2] = (i % 2 ? -ysize_checker : ysize_checker);
            xarr[3] = -seg_len;
            yarr[3] = 0;
            xarr[4] = 0;
            yarr[4] = (i % 2 ? ysize_checker : -ysize_checker);
            D_polygon_rel(xarr, yarr, 5);
            D_pos_rel(seg_len, 0);
        }
        /* draw a vertical cross line */
        D_begin();
        D_move_rel(0, ysize_checker);
        D_cont_rel(0, -2 * ysize_checker);
        D_close();
        D_end(); /* no-op? */
        D_stroke();

        D_pos_rel(0, ysize_checker);
        xarr[0] = 0;
        yarr[0] = 0;
        xarr[1] = line_len / 2.;
        if (seg == 5)
            xarr[1] -= seg_len / 2.;
        yarr[1] = 0;
        xarr[2] = 0;
        yarr[2] = ysize_checker;
        xarr[3] = -line_len / 2.;
        if (seg == 5)
            xarr[3] += seg_len / 2.;
        yarr[3] = 0;
        xarr[4] = 0;
        yarr[4] = -ysize_checker;
        D_polygon_rel(xarr, yarr, 5);
        D_pos_rel(seg_len, 0);
    }
    else if (style == STYLE_TICKS_BOTH) {
        /* draw simple line scale with corssing ticks */
        D_begin();
        D_move_abs(x_pos_start, y_pos + (ysize - ysize_ticks) / 2);
        D_cont_abs(x_pos_start, y_pos + ysize - (ysize - ysize_ticks) / 2);
        D_move_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_move_abs(x_pos_start + line_len, y_pos + (ysize - ysize_ticks) / 2);
        D_cont_abs(x_pos_start + line_len,
                   y_pos + ysize - (ysize - ysize_ticks) / 2);

        D_move_abs(x_pos_start, y_pos + ysize / 2);
        D_move_rel(0, ysize_ticks / 4);
        for (i = 0; i <= seg - 2; i++) {
            D_move_rel(seg_len, 0);
            D_cont_rel(0, -ysize_ticks /
                              2); /* 5 above, on px on line, and 5 below */
            D_move_rel(0, +ysize_ticks / 2);
        }
        D_end(); /* no-op? */
    }
    else if (style == STYLE_TICKS_UP) {
        /* draw simple line scale with up facing ticks */
        D_begin();
        D_move_abs(x_pos_start, y_pos - 2 * width_scale);
        D_cont_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_move_abs(x_pos_start + line_len, y_pos - 2 * width_scale);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);

        D_move_abs(x_pos_start, y_pos + ysize / 2);
        for (i = 0; i <= seg - 2; i++) {
            D_move_rel(seg_len, 0);
            D_cont_rel(0, -ysize_ticks /
                              2); /* 5 above, on px on line, and 5 below */
            D_move_rel(0, +ysize_ticks / 2);
        }
        D_end(); /* no-op? */
        D_close();
    }
    else if (style == STYLE_TICKS_DOWN) {
        /* draw simple line scale with down facing ticks */
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2 + 17 * width_scale);
        D_cont_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_move_abs(x_pos_start + line_len,
                   y_pos + ysize / 2 + 17 * width_scale);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);

        D_move_abs(x_pos_start, y_pos + ysize / 2);
        for (i = 0; i <= seg - 2; i++) {
            D_move_rel(seg_len, 0);
            D_cont_rel(0, +ysize_ticks /
                              2); /* 5 above, on px on line, and 5 below */
            D_move_rel(0, -ysize_ticks / 2);
        }
        D_end(); /* no-op? */
        D_close();
    }
    else if (style == STYLE_ARROW_ENDS) {
        /* draw line scale with |<--dimension arrows-->| on the ends */
        D_begin();
        D_cont_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_end();

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

        symbol_size = 12 * width_scale;

        x0 = D_d_to_u_col(x_pos_start);
        y0 = D_d_to_u_row(y_pos + ysize / 2);
        Symb = S_read("extra/dim_arrow");
        if (!Symb)
            G_fatal_error(_("Could not read symbol \"%s\""), "extra/dim_arrow");
        S_stroke(Symb, symbol_size, 0.0, 0);
        D_symbol(Symb, x0, y0, line_color, fill_color);
        G_free(Symb);

        x0 = D_d_to_u_col(line_len + x_pos_start);
        y0 = D_d_to_u_row(y_pos + ysize / 2);
        Symb = S_read("extra/dim_arrow");
        S_stroke(Symb, symbol_size, 180., 0);
        D_symbol(Symb, x0, y0, line_color, fill_color);
        G_free(Symb);

        G_free(line_color);
        G_free(fill_color);

        /* draw simple line between the two ends */
        D_begin();
        D_move_abs(x_pos_start, y_pos + ysize / 2);
        D_cont_abs(x_pos_start + line_len, y_pos + ysize / 2);
        D_end(); /* no-op? */
    }
    D_stroke();

    if (fontsize < 0) {
        if (length != 0) {
            G_free(label);
        }
        return 0;
    }

    /* draw the distance + units text */

    D_get_text_box(label, &tt, &tb, &tl, &tr);

    if (text_posn == TEXT_OVER) {
        D_pos_abs(x_pos + line_len / 2 + xspace_around_line +
                      (north_arrow ? xspace_bf_N + xsize_N : 0) - (tr - tl) / 2,
                  y_pos);
        D_text(label);
    }
    else if (text_posn == TEXT_UNDER) {
        D_pos_abs(x_pos + line_len / 2 + xspace_around_line +
                      (north_arrow ? xspace_bf_N + xsize_N : 0) - (tr - tl) / 2,
                  y_pos + 40 * width_scale);
        D_text(label);
    }
    else if (text_posn == TEXT_RIGHT) {
        if (style == STYLE_TICKS_UP)
            y_pos -= 8 * width_scale;
        else if (style == STYLE_TICKS_DOWN)
            y_pos += 9 * width_scale;

        D_pos_abs(x_pos + line_len + 2 * xspace_around_line +
                      (north_arrow ? xspace_bf_N + xsize_N : 0),
                  y_pos + ysize / 2 + (tt - tb) / 2);
        D_text(label);
    }
    else if (text_posn == TEXT_LEFT) {
        if (style == STYLE_TICKS_UP)
            y_pos -= 8 * width_scale;
        else if (style == STYLE_TICKS_DOWN)
            y_pos += 9 * width_scale;

        if (style == STYLE_CLASSIC_BAR || style == STYLE_THIN_WITH_ENDS)
            x_pos -= 13 * width_scale;

        D_pos_abs(x_pos + 5 - (tr - tl), y_pos + ysize / 2 + (tt - tb) / 2);
        D_text(label);
    }
    if (length != 0) {
        G_free(label);
    }

    return 0;
}
