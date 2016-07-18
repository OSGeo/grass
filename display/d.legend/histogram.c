/* histogram.c:
 *    Draws a histogram along the left side of a smooth gradient legend
 *    (stats fetching code adapted from d.histogram)
 *
 *    Copyright (C) 2014 by Hamish Bowman, and the GRASS Development Team* 
 *    This program is free software under the GPL (>=v2)
 *    Read the COPYING file that comes with GRASS for details.
 */

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

double histogram(const char *map_name, int x0, int y0, int width,
                 int height, int color, int flip, int horiz, int map_type,
                 int is_fp, struct FPRange render_range, int drawh)
{
    int i, nsteps, ystep;
    long cell_count = 0;
    double max_width, width_mult, dx, max;
    double dy, y0_adjust;       /* only needed for CELL maps */
    struct stat_list dist_stats;
    struct stat_node *ptr;
    struct Range range;
    struct FPRange fprange;
    CELL c_map_min, c_map_max;
    DCELL d_map_min, d_map_max;
    double map_min, map_max, map_range, user_range;
    double crop_min_perc = 0.0, crop_max_perc = 1.0, pad_min_perc = 0.0;

    if (horiz) {
        max_width = height * 1.75;
        nsteps = width - 3;
    }
    else {
        max_width = width * 1.75;
        nsteps = height - 3;
    }

    /* reset return value max */
    max = 0;


    if (render_range.first_time) {
        /* user specified range, can be either larger
           or smaller than actual map's range */

        if (is_fp) {
            Rast_read_fp_range(map_name, "", &fprange);
            Rast_get_fp_range_min_max(&fprange, &d_map_min, &d_map_max);
            map_min = (double)d_map_min;
            map_max = (double)d_map_max;
        }
        else {
            Rast_read_range(map_name, "", &range);
            Rast_get_range_min_max(&range, &c_map_min, &c_map_max);
            map_min = (double)c_map_min;
            map_max = (double)c_map_max;
        }

        map_range = map_max - map_min;
        user_range = render_range.max - render_range.min;

        if (horiz)
            nsteps = (int)(0.5 + (map_range * (width - 3) / user_range));
        else
            nsteps = (int)(0.5 + (map_range * (height - 3) / user_range));

        G_debug(1,
                "number of steps for r.stats = %d, height-3=%d  width-3=%d",
                nsteps, height - 3, width - 3);

        /* need to know the % of the MAP range where user range starts and stops.
         *   note that MAP range can be fully inside user range, in which case
         *   keep 0-100% aka 0,nsteps, i.e. the step number in the nsteps range */

        if (render_range.min > map_min) {
            crop_min_perc = (render_range.min - map_min) / map_range;
            G_debug(3, "min: %.02f vs. %.02f (%.02f) ... %.02f%%",
                    render_range.min, map_min, map_range,
                    100 * crop_min_perc);
        }

        if (render_range.max > map_max) {
            crop_max_perc = 1.0 - ((render_range.max - map_max) / user_range);
            G_debug(3, "max: %.02f vs. %.02f (%.02f) ... %.02f%%",
                    map_max, render_range.max, map_range,
                    100 * crop_max_perc);
        }

        if (render_range.min < map_min) {
            pad_min_perc = (map_min - render_range.min) / user_range;
            G_debug(3, "Min: %.02f vs. %.02f (%.02f) ... %.02f%%",
                    map_min, render_range.min, user_range,
                    100 * pad_min_perc);
        }

#ifdef amplify_gain
        /* proportion of nsteps to width, use as mult factor to boost the 1.75x
           when spread out over more nsteps than we are displaying */
        G_debug(0, "max_width was: %.2f  (nsteps=%d)", max_width, nsteps);

        if (nsteps > ((horiz ? width : height) - 3.0))
            max_width *= nsteps / ((horiz ? width : height) - 3.0);

        G_debug(0, "max_width now: %.2f", max_width);
#endif
    }


    /* TODO */
    if (!is_fp && render_range.first_time) {
        G_warning(_("Histogram constrained by range not yet implemented for "
                    "categorical rasters"));
        return max;
    }


    /* get the distribution statistics */
    get_stats(map_name, &dist_stats, nsteps, map_type);

    width_mult = max_width / dist_stats.maxstat;
    ptr = dist_stats.ptr;

    if (drawh) {
        D_use_color(color);
        D_begin();

        if (!is_fp) {
            dy = (nsteps + 3.0) / (1 + dist_stats.maxcat - dist_stats.mincat);

            if (flip)
                dy *= -1;

            if (dist_stats.mincat == 0)
                y0_adjust = dy;
            else
                y0_adjust = 0;

            if (!flip)          /* mmph */
                y0_adjust += 0.5;
        }
    }


    G_debug(3, "mincat=%ld  maxcat=%ld", dist_stats.mincat,
            dist_stats.maxcat);

    for (i = dist_stats.mincat, ystep = 0; i <= dist_stats.maxcat; i++) {
        cell_count = 0;
        if (!ptr)
            break;

        /* jump out if user range cuts things shorter than the map's native range */
        if ((horiz && ystep > width - 4) || (!horiz && ystep > height - 4))
            break;

        /* jump out if user range goes beyond max of map data */
        if (((double)ystep / ((horiz ? width : height) - 3.0)) >
            crop_max_perc)
            break;
        /* TODO if (!is_fp && i > render_range.max)
           break;
         */
        /* haven't made it to the min of the user range yet */
        if (((double)i / nsteps) < crop_min_perc) {
            continue;
        }

        /* now it's ok advance the plotter position */
        ystep++;

        /* if user range is below the minimum real map value, we need to pad out the space */
        if (render_range.first_time && render_range.min < map_min) {
            if (((double)ystep / ((horiz ? width : height) - 3.0)) <
                pad_min_perc) {
                i--;
                continue;
            }
        }

        if (ptr->cat == i) {    /* AH-HA!! found the stat */
            cell_count = ptr->stat;

            if (ptr->next != NULL)
                ptr = ptr->next;
        }
        else {                  /* we have to look for the stat */

            /* loop until we find it, or pass where it should be */
            while (ptr->cat < i && ptr->next != NULL)
                ptr = ptr->next;
            if (ptr->cat == i) {        /* AH-HA!! found the stat */
                cell_count = ptr->stat;

                if (ptr->next != NULL)
                    ptr = ptr->next;
            }
            else                /* stat cannot be found */
                G_debug(5, "No matching stat found, i=%d", i);
        }

        G_debug(5, "i=%d  ptr->cat=%ld  cell_count=%ld", i, ptr->cat,
                cell_count);

        if (!cell_count)
            continue;

        dx = cell_count * width_mult;

        if (drawh) {
            if (is_fp) {
                if (horiz) {
                    if (flip)
                        D_move_abs(x0 + width - ystep - 1, y0 - 1);
                    else
                        D_move_abs(x0 + ystep + 1, y0 - 1);

                    D_cont_rel(0, -dx);
                }
                else {          /* vertical */
                    if (flip)
                        D_move_abs(x0 - 1, y0 - 1 + height - ystep);
                    else
                        D_move_abs(x0 - 1, y0 + 1 + ystep);

                    D_cont_rel(-dx, 0);
                }
            }
            else {              /* categorical */

                if (horiz) {
                    if (flip)
                        D_box_abs(x0 + width + y0_adjust + ((i - 2) * dy),
                                  y0 - 1,
                                  x0 + width + y0_adjust + 1 + ((i - 1) * dy),
                                  y0 - 1 - dx);
                    else
                        D_box_abs(x0 + y0_adjust + ((i - 2) * dy),
                                  y0 - 1,
                                  x0 - 1 + y0_adjust + ((i - 1) * dy), y0 - 1 - dx);
                }
                else {          /* vertical */

                    if (flip)
                        /* GRASS_EPSILON fudge around D_box_abs() weirdness + PNG driver */
                        D_box_abs(x0 - 1 - GRASS_EPSILON * 10,
                                  y0 + height + y0_adjust + ((i - 2) * dy),
                                  x0 - 1 - dx,
                                  y0 + height + y0_adjust + 1 + ((i - 1) * dy));
                    else
                        D_box_abs(x0 - 1 - GRASS_EPSILON * 10,
                                  y0 + y0_adjust + ((i - 2) * dy),
                                  x0 - 1 - dx, y0 + y0_adjust - 1 + ((i - 1) * dy));
                }
            }
        }
        if (dx > max)
            max = dx;
    }

    if (drawh) {
        D_close();
        D_end();
        D_stroke();
    }

    return max;
}
