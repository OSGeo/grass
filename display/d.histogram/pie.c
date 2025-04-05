/* pie.c
 *
 * function defined:
 *
 * pie(dist_stats,colors)
 *
 * struct stat_list *dist_stats     - linked list of statistics
 * struct Colors *colors            - map colors
 *
 *
 * PURPOSE: To draw a pie-chart representing the histogram
 * statistics in the linked list dist_stats.
 *
 * NOTES:
 *
 * 1) see dhist.h for a declaration of the structure stat_list.
 * 2) see pie.h for normalized coordinates of the different parts
 *    of the pie-chart, like the origin of the pie, the label
 *    positions, etc.
 * 3) pie slices are given percent labels (eg. 20%, 70%) if they
 *    represent over 15% of the pie.
 *
 *
 * Dave Johnson
 * DBA Systems, Inc.
 * 10560 Arrowhead Drive
 * Fairfax, Virginia 22030
 *
 */

#include <string.h>

#include <grass/display.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "pie.h"

/*#define DEBUG */

#define YES 1
#define NO  0

int pie(struct stat_list *dist_stats, /* list of distribution statistics */
        struct Colors *colors)
{
    struct stat_node *ptr;
    double arc, arc_counter;
    int draw = YES;
    long int bar_height; /* height, in pixels, of a histogram bar */
    long int bar_color;  /* color/category number of a histogram bar */
    long int max_tics;   /* maximum tics allowed on an axis */
    long int xoffset;    /* offset for x-axis */
    long int yoffset;    /* offset for y-axis */
    int text_height;
    int text_width;
    long int i, j;
    long int num_cats;
    long int tic_every; /* spacing, in units of category value, of tics */
    long int tic_unit;
    double t, b, l, r;
    double tt, tb, tl, tr;
    double x_line[5]; /* for border of histogram */
    double y_line[5];
    double x_box[6]; /* for histogram bar coordinates */
    double y_box[6];
    double height, width;
    double xscale; /* scaling factors */

    /* double yscale; */
    char xlabel[1024];
    char txt[1024];
    char tic_name[80];
    DCELL dmin, dmax, range_dmin, range_dmax, dval;
    dmin = 0.0;

    /* get coordinates of current screen window */
    D_get_src(&t, &b, &l, &r);

    /* create legend box border, to be drawn later */
    height = b - t;
    width = r - l;
    x_line[4] = x_line[0] = x_line[1] = l + (BAR_X1 * width);
    y_line[4] = y_line[0] = y_line[3] = b - (BAR_Y1 * height);
    x_line[2] = x_line[3] = l + (BAR_X2 * width);
    bar_height = y_line[1] = y_line[2] = b - (BAR_Y2 * height);

    /* figure scaling factors and offsets */
    num_cats = dist_stats->maxcat - dist_stats->mincat + 1;
    if (nodata) {
        num_cats++;
        dist_stats->mincat--;
    }
    xscale = ((double)(x_line[2] - x_line[1]) / ((double)num_cats));
    /* yscale = ((double)(y_line[0] - y_line[1])) / dist_stats->maxstat; */
    yoffset = (long)(y_line[0]);
    if (num_cats >= x_line[2] - x_line[1])
        xoffset = (double)x_line[1];
    else
        xoffset = (double)x_line[0] + 0.5 * xscale; /* boxes need extra space */

#ifdef DEBUG
    fprintf(stdout, "num_cats=%ld x1=%d x2=%d xscale=%lf\n", num_cats,
            x_line[1], x_line[2], xscale);
#endif

    /* figure tic_every and tic_units for the x-axis of the bar-chart.
     * tic_every tells how often to place a tic-number.  tic_unit tells
     * the unit to use in expressing tic-numbers.
     */
    if (xscale < XTIC_DIST) {
        max_tics = (x_line[2] - x_line[1]) / XTIC_DIST;
        i = 0;
        if (is_fp) {
            Rast_get_fp_range_min_max(&fp_range, &range_dmin, &range_dmax);
            if (Rast_is_d_null_value(&range_dmin) ||
                Rast_is_d_null_value(&range_dmax))
                G_fatal_error("Floating point data range is empty");

            while ((range_dmax - range_dmin) / tics[i].every > max_tics)
                i++;
        }

        while ((num_cats / tics[i].every) > max_tics)
            i++;
        tic_every = tics[i].every;
        tic_unit = tics[i].unit;
        if (G_strlcpy(tic_name, tics[i].name, sizeof(tic_name)) >=
            sizeof(tic_name)) {
            G_fatal_error(_("Tic name <%s> is too long"), tics[i].name);
        }
    }
    else {
        if (is_fp && !cat_ranges) {
            Rast_get_fp_range_min_max(&fp_range, &range_dmin, &range_dmax);
            if (Rast_is_d_null_value(&range_dmin) ||
                Rast_is_d_null_value(&range_dmax))
                G_fatal_error("Floating point data range is empty");
        }
        tic_every = 1;
        tic_unit = 1;
    }

    /* PIE & LEGEND LOOP
     *
     * loop through category range, drawing a pie-slice and a
     * legend bar on each iteration evenly divisible, a tic-mark
     * on those evenly divisible by tic_unit, and a tic_mark
     * number on those evenly divisible by tic_every
     *
     */
    ptr = dist_stats->ptr;
    arc_counter = 0;
    for (i = dist_stats->mincat; i <= dist_stats->maxcat; i++) {
        text_height = height * 0.7 * TEXT_HEIGHT;
        text_width = width * 0.7 * TEXT_WIDTH;
        D_text_size(text_width, text_height);
        draw = NO;
        /* figure color and height of the slice of pie
         *
         * the cat number determines the color, the corresponding stat,
         * determines the bar height.  if a stat cannot be found for the
         * cat, then we don't draw anything, but before in this case we
         * used to draw a black box of size 0. Later on when the option
         * to specify the backgrown colors will be added, we might still
         * draw a box in that color.
         */
        if (nodata && i == dist_stats->mincat)
        /* null */
        {
            if (dist_stats->null_stat == 0 && xscale > 1)
                draw = NO;
            else {
                draw = YES;
                Rast_set_d_null_value(&dval, 1);
                arc = 360.0 * ((double)dist_stats->null_stat /
                               (double)dist_stats->sumstat);
                draw_slice_filled(colors, dval, color, ORIGIN_X, ORIGIN_Y,
                                  RADIUS, arc_counter, arc);
                /*OUTLINE THE SLICE
                   draw_slice_unfilled(colors, color,ORIGIN_X,ORIGIN_Y,
                   RADIUS,arc_counter,arc); */
                arc_counter += arc;
                D_d_color(dval, colors);
            }
        }
        else if (ptr->cat == i) { /* AH-HA!! found the stat */
            if (ptr->stat == 0 && xscale > 1)
                draw = NO;
            else {
                draw = YES;
                if (is_fp) {
                    if (cat_ranges)
                        Rast_get_ith_d_cat(&cats, (CELL)i, &dmin, &dmax);
                    else {
                        dmin =
                            range_dmin + i * (range_dmax - range_dmin) / nsteps;
                        dmax = range_dmin +
                               (i + 1) * (range_dmax - range_dmin) / nsteps;
                    }
                    arc = 360.0 * ptr->stat / dist_stats->sumstat;
                    draw_slice(colors, 1, dmin, dmax, color, ORIGIN_X, ORIGIN_Y,
                               RADIUS, arc_counter, arc);
                    arc_counter += arc;
                    D_d_color(dmin, colors);
                    /*OUTLINE THE SLICE */
                    draw_slice_unfilled(colors, color, ORIGIN_X, ORIGIN_Y,
                                        RADIUS, arc_counter, arc);
                }
                else {
                    bar_color = ptr->cat;
                    arc = 360.0 * ptr->stat / dist_stats->sumstat;
                    draw_slice_filled(colors, (DCELL)bar_color, color, ORIGIN_X,
                                      ORIGIN_Y, RADIUS, arc_counter, arc);
                    D_color((CELL)bar_color, colors);
                    arc_counter += arc;
                }
            }
            if (ptr->next != NULL)
                ptr = ptr->next;
        }
        else { /* we have to look for the stat */

            /* loop until we find it, or pass where it should be */
            while (ptr->cat < i && ptr->next != NULL)
                ptr = ptr->next;
            if (ptr->cat == i) { /* AH-HA!! found the stat */
                if (ptr->stat == 0 && xscale > 1)
                    draw = NO;
                else {
                    draw = YES;
                    if (is_fp) {
                        if (cat_ranges)
                            Rast_get_ith_d_cat(&cats, (CELL)i, &dmin, &dmax);
                        else {
                            dmin = range_dmin +
                                   i * (range_dmax - range_dmin) / nsteps;
                            dmax = range_dmin +
                                   (i + 1) * (range_dmax - range_dmin) / nsteps;
                        }
                        arc = 360.0 * ptr->stat / dist_stats->sumstat;
                        draw_slice(colors, 1, dmin, dmax, color, ORIGIN_X,
                                   ORIGIN_Y, RADIUS, arc_counter, arc);
                        arc_counter += arc;
                        /*OUTLINE THE SLICE */
                        draw_slice_unfilled(colors, color, ORIGIN_X, ORIGIN_Y,
                                            RADIUS, arc_counter, arc);
                        D_d_color(dmin, colors);
                    }
                    else {
                        bar_color = ptr->cat;
                        arc = 360.0 * ptr->stat / dist_stats->sumstat;
                        draw_slice_filled(colors, (DCELL)bar_color, color,
                                          ORIGIN_X, ORIGIN_Y, RADIUS,
                                          arc_counter, arc);
                        D_color((CELL)bar_color, colors);
                        arc_counter += arc;
                    }
                }
            }
            else { /* stat cannot be found */

                if (xscale > 1) {
                    /*
                       draw=YES;
                     */
                    draw = NO;
                    bar_color = D_translate_color("black");
                    D_use_color(bar_color);
                }
                else
                    draw = NO;
            }
        }

        /* draw the bar */
        if (draw == YES) {
            if (xscale != 1) {
                /* draw the bar as a box */

                /* if fp map and not null and range is not empty, draw smooth
                   color range */
                if ((is_fp && !(i == dist_stats->mincat && nodata) &&
                     dmin != dmax)) {
                    for (j = 0; j < xscale; j++) {
                        dval = dmin + j * (dmax - dmin) / xscale;
                        D_d_color(dval, colors);
                        x_box[0] = x_box[1] =
                            xoffset + ((i - dist_stats->mincat) * xscale -
                                       0.5 * xscale + j);
                        x_box[2] = x_box[3] =
                            xoffset + ((i - dist_stats->mincat) * xscale -
                                       0.5 * xscale + j + 1);
                        y_box[0] = y_box[3] = y_box[4] = y_line[0];
                        y_box[1] = y_box[2] = bar_height;
                        D_polygon_abs(x_box, y_box, 4);
                    }
                }
                else { /* draw 1-color bar, color is already set */

                    x_box[0] = x_box[1] =
                        xoffset +
                        ((i - dist_stats->mincat) * xscale - 0.5 * xscale);
                    x_box[2] = x_box[3] =
                        xoffset +
                        ((i - dist_stats->mincat) * xscale + 0.5 * xscale);
                    y_box[0] = y_box[3] = y_box[4] = y_line[0];
                    y_box[1] = y_box[2] = bar_height;
                    D_polygon_abs(x_box, y_box, 4);
                }
            }
            else { /* color is already set for 1-color bar */

                /* draw the bar as a line */
                x_box[0] = x_box[1] =
                    xoffset + (i - dist_stats->mincat) * xscale;
                y_box[0] = yoffset;
                y_box[1] = bar_height;
                D_line_abs(x_box[0], y_box[0], x_box[1], y_box[1]);
            }
        }

        /* draw x-axis tic-marks and numbers */
        /* draw tick for null and for numbers at every tic step
           except when there is null, don't draw tic for mincat+1 */

        if ((rem((long int)i, tic_every) == 0L ||
             ((i == dist_stats->mincat) && nodata)) &&
            !(nodata && i == dist_stats->mincat + 1)) {

            /* draw a numbered tic-mark */
            D_use_color(color);
            D_begin();
            D_move_abs(xoffset + (i - dist_stats->mincat) * xscale -
                           0.5 * xscale,
                       b - BAR_Y1 * height);
            D_cont_rel(0, BIG_TIC * height);
            D_end();
            D_stroke();

            if (nodata && i == dist_stats->mincat)
                sprintf(txt, "null");
            else if (is_fp)
                sprintf(txt, "%d", (int)(dmin / (double)tic_unit));
            else
                sprintf(txt, "%d", (int)(i / tic_unit));
            text_height = height * TEXT_HEIGHT;
            text_width = width * TEXT_WIDTH;
            D_text_size(text_width, text_height);
            D_get_text_box(txt, &tt, &tb, &tl, &tr);
            while ((tr - tl) > XTIC_DIST) {
                text_width *= 0.95;
                text_height *= 0.95;
                D_text_size(text_width, text_height);
                D_get_text_box(txt, &tt, &tb, &tl, &tr);
            }
            D_pos_abs(xoffset + (i - dist_stats->mincat) * xscale -
                          0.5 * xscale - (tr - tl) / 2,
                      b - XNUMS_Y * height);
            D_text(txt);
        }
        else if (rem(i, tic_unit) == 0.0) {
            /* draw a tic-mark */
            D_use_color(color);
            D_begin();
            D_move_abs(xoffset + (i - dist_stats->mincat) * xscale -
                           0.5 * xscale,
                       b - BAR_Y1 * height);
            D_cont_rel(0, SMALL_TIC * height);
            D_end();
            D_stroke();
        }
    }

    /* draw border around pie */
    D_use_color(color);
    draw_slice_unfilled(colors, color, ORIGIN_X, ORIGIN_Y, RADIUS, 0.0, 360.0);

    /* draw border around legend bar */
    D_use_color(color);
    D_polyline_abs(x_line, y_line, 5);

    /* draw the x-axis label */
    if (tic_unit != 1)
        sprintf(xlabel, "Cell Values %s", tic_name);
    else
        sprintf(xlabel, "Cell Values");
    text_height = height * TEXT_HEIGHT;
    text_width = width * TEXT_WIDTH;
    D_text_size(text_width, text_height);
    D_get_text_box(xlabel, &tt, &tb, &tl, &tr);
    D_pos_abs(l + width / 2 - (tr - tl) / 2, b - LABEL * height);
    D_use_color(color);
    D_text(xlabel);

    return 0;
}
