/* bar.c
 *
 * function defined:
 *
 * bar(dist_stats,colors)
 *
 * struct stat_list *dist_stats          - linked list of statistics
 * struct Colors *colors                 - map colors
 *
 * PURPOSE: To draw a bar-chart representing the histogram
 * statistics in the linked list dist_stats.
 *
 * NOTES:
 *
 * 1) see dhist.h for a declaration of the structure stat_list.
 * 2) see bar.h for normalized coordinates of the different parts
 *    of the bar-chart, like the origin of the chart, the label
 *    positions, etc.
 *
 *
 * Dave Johnson
 * DBA Systems, Inc.
 * 10560 Arrowhead Drive
 * Fairfax, Virginia 22030
 *
 */

#include <string.h>

#include <grass/raster.h>
#include <grass/display.h>

#include "bar.h"

int bar(struct stat_list *dist_stats, /* list of distribution statistics */
        struct Colors *colors)
{
    struct stat_node *ptr;
    int draw = YES;
    long int bar_height; /* height, in pixels, of a histogram bar */
    CELL bar_color;      /* color/category number of a histogram bar */
    DCELL dmax, range_dmin = 0, range_dmax = 0, dmin, dval;
    long int max_tics; /* maximum tics allowed on an axis */
    long int xoffset;  /* offset for x-axis */
    long int yoffset;  /* offset for y-axis */
    long int stat_start;
    long int stat_finis;
    int text_height;
    int text_width;
    long int i, j;
    long int num_cats = 0;
    long int num_stats = 0;
    long int tic_every; /* spacing, in units of category value, of tics */

    long int tic_unit;
    double t, b, l, r;
    double tt, tb, tl, tr;
    double x_line[3]; /* for border of histogram */
    double y_line[3];
    double x_box[5]; /* for histogram bar coordinates */
    double y_box[5];
    double height, width;
    double xscale; /* scaling factors */
    double yscale;
    char xlabel[1024];
    char ylabel[1024];
    char txt[1024];
    char tic_name[80];

    /* get coordinates of current screen window */
    D_get_src(&t, &b, &l, &r);

    /* create axis lines, to be drawn later */
    height = b - t;
    width = r - l;
    x_line[0] = x_line[1] = l + (ORIGIN_X * width);
    x_line[2] = l + (XAXIS_END * width);
    y_line[0] = b - (YAXIS_END * height);
    y_line[1] = y_line[2] = b - (ORIGIN_Y * height);

    /* figure scaling factors and offsets */
    num_cats = dist_stats->maxcat - dist_stats->mincat + 1;

    if (nodata) {
        num_cats++;
        dist_stats->mincat--;
    }
    xscale = ((x_line[2] - x_line[1]) / ((double)num_cats));
    yscale = ((y_line[1] - y_line[0])) / dist_stats->maxstat;
    if (num_cats >= x_line[2] - x_line[1])
        xoffset = (long int)x_line[1];
    else
        xoffset =
            (long int)x_line[0] + 0.5 * xscale; /* boxes need extra space */
    yoffset = (double)(y_line[1]);

    /* figure tic_every and tic_units for the x-axis of the bar-chart.
     * tic_every tells how often to place a tic-number.  tic_unit tells
     * the unit to use in expressing tic-numbers.
     */
    if (xscale < XTIC_DIST) {
        max_tics = (x_line[2] - x_line[1]) / XTIC_DIST;
        if (nodata)
            max_tics--;
        i = 0;
        if (is_fp) {
            Rast_get_fp_range_min_max(&fp_range, &range_dmin, &range_dmax);
            if (Rast_is_d_null_value(&range_dmin) ||
                Rast_is_d_null_value(&range_dmax))
                G_fatal_error("Floating point data range is empty");

            if ((range_dmax - range_dmin) < 1.0)
                tics[i].every = 5;
            if ((range_dmax - range_dmin) < 110)
                tics[i].every = 20; /* dirrty hack */
            while ((range_dmax - range_dmin) / tics[i].every > max_tics)
                i++;
        }
        else {
            while ((num_cats / tics[i].every) > max_tics)
                i++;
        }
        tic_every = tics[i].every;
        tic_unit = tics[i].unit;
        strcpy(tic_name, tics[i].name);
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

    /* X-AXIS LOOP
     *
     * loop through category range, drawing a pie-slice and a
     * legend bar on each iteration evenly divisible, a tic-mark
     * on those evenly divisible by tic_unit, and a tic_mark
     * number on those evenly divisible by tic_every
     *
     */
    ptr = dist_stats->ptr;
    for (i = dist_stats->mincat; i <= dist_stats->maxcat; i++) {
        if (!ptr)
            break;
        /* figure bar color and height
         *
         * the cat number determines the color, the corresponding stat,
         * determines the bar height.  if a stat cannot be found for the
         * cat, then it doesn't drow anything, before it used to draw the
         * box of size 0 in black. Later when the option to provide the
         * background color will be added , we might still draw a box in
         * this color.
         */
        if (nodata && i == dist_stats->mincat) {
            if (dist_stats->null_stat == 0 && xscale > 1)
                draw = NO;
            else {
                draw = YES;
                Rast_set_c_null_value(&bar_color, 1);
                bar_height = (yoffset - yscale * (double)dist_stats->null_stat);
            }
        }
        else if (ptr->cat == i) { /* AH-HA!! found the stat */
            if (ptr->stat == 0 && xscale > 1)
                draw = NO;
            else {
                draw = YES;
                bar_color = ptr->cat;
                bar_height = (yoffset - yscale * (double)ptr->stat);
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
                    bar_color = ptr->cat;
                    bar_height = (yoffset - yscale * (double)ptr->stat);
                }
                if (ptr->next != NULL)
                    ptr = ptr->next;
            }
            else { /* stat cannot be found */

                if (xscale > 1) {
                    draw = NO;

#ifdef notdef
                    draw = YES;
                    bar_color = D_translate_color("black");
                    bar_height = yoffset; /* zero */
#endif
                }
                else
                    draw = NO;
            }
        }

        /* draw the bar */
        if (draw == YES) {
            if (xscale != 1) {
                /* draw the bar as a box */
                if (!Rast_is_c_null_value(&bar_color) && is_fp) {
                    if (cat_ranges)
                        Rast_get_ith_d_cat(&cats, bar_color, &dmin, &dmax);
                    else {
                        dmin =
                            range_dmin + i * (range_dmax - range_dmin) / nsteps;
                        dmax = range_dmin +
                               (i + 1) * (range_dmax - range_dmin) / nsteps;
                    }
                    if (dmin != dmax) {
                        for (j = 0; j < xscale; j++) {
                            dval = dmin + j * (dmax - dmin) / xscale;
                            D_d_color(dval, colors);
                            x_box[0] = x_box[1] =
                                xoffset + ((i - dist_stats->mincat) * xscale -
                                           0.5 * xscale + j);
                            x_box[2] = x_box[3] =
                                xoffset + ((i - dist_stats->mincat) * xscale -
                                           0.5 * xscale + j + 1);
                            y_box[0] = y_box[3] = yoffset;
                            y_box[1] = y_box[2] = bar_height;
                            D_polygon_abs(x_box, y_box, 4);
                        }
                    }
                    else { /* 1-color bar */

                        D_d_color(dmin, colors);
                        x_box[0] = x_box[1] =
                            xoffset +
                            ((i - dist_stats->mincat) * xscale - 0.5 * xscale);
                        x_box[2] = x_box[3] =
                            xoffset +
                            ((i - dist_stats->mincat) * xscale + 0.5 * xscale);
                        y_box[0] = y_box[3] = yoffset;
                        y_box[1] = y_box[2] = bar_height;
                        D_polygon_abs(x_box, y_box, 4);
                    }
                } /* fp */
                else { /* 1-color bar for int data or null */

                    D_color((CELL)bar_color, colors);
                    x_box[0] = x_box[1] =
                        xoffset +
                        ((i - dist_stats->mincat) * xscale - 0.5 * xscale);
                    x_box[2] = x_box[3] =
                        xoffset +
                        ((i - dist_stats->mincat) * xscale + 0.5 * xscale);
                    y_box[0] = y_box[3] = yoffset;
                    y_box[1] = y_box[2] = bar_height;
                    D_polygon_abs(x_box, y_box, 4);
                }
            }
            else {
                /* draw the bar as a line */
                if (is_fp) {
                    if (cat_ranges)
                        Rast_get_ith_d_cat(&cats, bar_color, &dmin, &dmax);
                    else {
                        dmin =
                            range_dmin + i * (range_dmax - range_dmin) / nsteps;
                        dmax = range_dmin +
                               (i + 1) * (range_dmax - range_dmin) / nsteps;
                    }
                    D_d_color(dmin, colors);
                }
                else
                    D_color((CELL)bar_color, colors);
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

        if (((rem((long int)i, tic_every) == 0L) ||
             ((i == dist_stats->mincat) && nodata)) &&
            !(nodata && i == dist_stats->mincat + 1)) {

            /* draw a numbered tic-mark */
            D_use_color(color);
            D_begin();
            D_move_abs(xoffset + (i - dist_stats->mincat) * xscale -
                           0.5 * xscale,
                       b - ORIGIN_Y * (b - t));
            D_cont_rel(0, BIG_TIC * (b - t));
            D_end();
            D_stroke();

            if (nodata && i == dist_stats->mincat)
                sprintf(txt, "null");
            else if (is_fp) {
                dmin = range_dmin + i * (range_dmax - range_dmin) / nsteps;
                if ((tic_every * (range_dmax - range_dmin) / nsteps) < 1.0)
                    sprintf(txt, "%.2f", dmin / (double)tic_unit);
                else
                    sprintf(txt, "%d", (int)(dmin / (double)tic_unit));
            }
            else
                sprintf(txt, "%d", (int)(i / tic_unit));
            text_height = (b - t) * TEXT_HEIGHT;
            text_width = (r - l) * TEXT_WIDTH;
            D_text_size(text_width, text_height);
            D_get_text_box(txt, &tt, &tb, &tl, &tr);
            while ((tr - tl) > XTIC_DIST) {
                text_width *= 0.75;
                text_height *= 0.75;
                D_text_size(text_width, text_height);
                D_get_text_box(txt, &tt, &tb, &tl, &tr);
            }
            D_pos_abs(xoffset + (i - dist_stats->mincat) * xscale -
                          0.5 * xscale - (tr - tl) / 2,
                      b - XNUMS_Y * (b - t));
            D_text(txt);
        }
        else if (rem(i, tic_unit) == 0.0) {
            /* draw a tic-mark */
            D_use_color(color);
            D_begin();
            D_move_abs(xoffset + (i - dist_stats->mincat) * xscale -
                           0.5 * xscale,
                       b - ORIGIN_Y * (b - t));
            D_cont_rel(0, SMALL_TIC * (b - t));
            D_end();
            D_stroke();
        }
    }

    /* draw the x-axis label */
    if (tic_unit != 1)
        sprintf(xlabel, "X-AXIS: Cell Values %s", tic_name);
    else
        sprintf(xlabel, "X-AXIS: Cell Values");
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH;
    D_text_size(text_width, text_height);
    D_get_text_box(xlabel, &tt, &tb, &tl, &tr);
    D_pos_abs(l + (r - l) / 2 - (tr - tl) / 2, b - LABEL_1 * (b - t));
    D_use_color(color);
    D_text(xlabel);

    /* DRAW Y-AXIS TIC-MARKS AND NUMBERS
     *
     * first, figure tic_every and tic_units for the x-axis of the bar-chart.
     * tic_every tells how often to place a tic-number.  tic_unit tells
     * the unit to use in expressing tic-numbers.
     */

    max_tics = (long)((y_line[1] - y_line[0]) / YTIC_DIST);

    if (dist_stats->maxstat == dist_stats->minstat)
        dist_stats->minstat = 0; /* LOOKS FUNNY TO ME */
    num_stats = dist_stats->maxstat - dist_stats->minstat;
    i = 0;
    while ((num_stats / tics[i].every) > max_tics)
        i++;
    tic_every = tics[i].every;
    tic_unit = tics[i].unit;
    strcpy(tic_name, tics[i].name);

    stat_start = tic_unit * ((long)(dist_stats->minstat / tic_unit));
    stat_finis = tic_unit * ((long)(dist_stats->maxstat / tic_unit));

    /* Y-AXIS LOOP
     *
     */
    for (i = stat_start; i <= stat_finis; i += tic_unit) {
        if (rem(i, tic_every) == (float)0) {
            /* draw a tic-mark */
            D_begin();
            D_move_abs(x_line[0], yoffset - yscale * i);
            D_cont_rel((-(r - l) * BIG_TIC), 0);
            D_end();
            D_stroke();

            /* draw a tic-mark number */
            sprintf(txt, "%d", (int)(i / tic_unit));
            text_height = (b - t) * TEXT_HEIGHT;
            text_width = (r - l) * TEXT_WIDTH;
            D_text_size(text_width, text_height);
            D_get_text_box(txt, &tt, &tb, &tl, &tr);
            while ((tt - tb) > YTIC_DIST) {
                text_width *= 0.75;
                text_height *= 0.75;
                D_text_size(text_width, text_height);
                D_get_text_box(txt, &tt, &tb, &tl, &tr);
            }
            D_pos_abs(l + (r - l) * YNUMS_X - (tr - tl) / 2,
                      yoffset - (yscale * i + 0.5 * (tt - tb)));
            D_text(txt);
        }
        else if (rem(i, tic_unit) == 0.0) {
            /* draw a tic-mark */
            D_begin();
            D_move_abs(x_line[0], yoffset - yscale * i);
            D_cont_rel(-(r - l) * SMALL_TIC, 0);
            D_end();
            D_stroke();
        }
    }

    /* draw the y-axis label */
    if (tic_unit != 1) {
        if (type == COUNT)
            sprintf(ylabel, "Y-AXIS: Number of cells %s", tic_name);
        else
            sprintf(ylabel, "Y-AXIS: Area %s sq. meters", tic_name);
    }
    else {
        if (type == COUNT)
            sprintf(ylabel, "Y-AXIS: Number of cells");
        else
            sprintf(ylabel, "Y-AXIS: Area");
    }

    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH;
    D_text_size(text_width, text_height);
    D_get_text_box(ylabel, &tt, &tb, &tl, &tr);
    D_pos_abs(l + (r - l) / 2 - (tr - tl) / 2, b - LABEL_2 * (b - t));
    D_use_color(color);
    D_text(ylabel);

    /* draw x and y axis lines */
    D_use_color(color);
    D_polyline_abs(x_line, y_line, 3);

    return 0;
}

float rem(long int x, long int y)
{
    long int d = x / y;

    return ((float)(x - y * d));
}
