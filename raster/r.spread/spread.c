/***********************************************************************
 *
 *        spread.c        in ../r.spread
 *
 *        This is a raster version of Dijkstra's shortest path algorithm
 *        that is suited for simulating elliptical spread phenomena. It
 *                1) starts from each spread origin (stored in
 *                   a linked list of type costHa) - spread();
 *                2) selects appropriate cells as links for the current
 *                   spread cell and stored in a linked list of type
 *                   cell_ptrHa - select() ;
 *                        A) calculates the cumulative cost (time) of the
 *                           end cell of each link - calculate();
 *                        B) compares this new cumulative cost (time) with
 *                           the previously computed cumulative time/cost,
 *                           if there is any, of the same cell - update();
 *                        C) puts this cell into a min-heap and puts the
 *                           new cumulative cost (time) together with UTM
 *                           coordinates in the cumulative cost (time)
 *                           map, x (East) map and y (North) map if
 *                           there is no previous cumulative cost (time);
 *                           otherwise, if the new cumulative cost (time)
 *                           is less, replaces with it both in the heap
 *                           and the output maps - update().
 *                3) gets the first cell in the min-heap, which is the
 *                   cell with the least cumulative cost (time), and
 *                   repeats Step 2 until the heap is empty or desired
 *                   simulated cumulative cost (time) is reached - spread().
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "cmd_line.h"
#include "costHa.h"
#include "cell_ptrHa.h"
#include "local_proto.h"

#ifndef PI
#define PI M_PI
#endif
#define DATA(map, r, c) (map)[(r) * ncols + (c)]

/*#define DEBUG */

extern CELL *map_max, *map_base, *map_dir, *map_visit;
extern CELL *map_x_out, *map_y_out;
extern float *map_out;
extern int BARRIER;
extern int nrows, ncols;

/* extern float         PI; */
extern long heap_len;
extern struct costHa *heap;
extern struct Cell_head window;

struct cell_ptrHa *front_cell = NULL, *rear_cell = NULL;

void spread(void)
{
    float min_cost;
    int ros_max, ros_base, dir;
    int row, col;
    int cell_count = 0, ncells = 0;
    struct cell_ptrHa *to_cell, *old_to_cell;
    struct costHa *pres_cell;

    /* initialize using arbitrary value, this value is never used except for
     * debug */
    min_cost = 0;

    ncells = nrows * ncols;
    G_message("Finding spread time - number of cells visited in percentage ... "
              " %3d%%",
              0);
    pres_cell = (struct costHa *)G_malloc(sizeof(struct costHa));
    get_minHa(heap, pres_cell, heap_len);
    G_debug(2, "begin spread: cost(%d,%d)=%f", pres_cell->row, pres_cell->col,
            pres_cell->min_cost);
    G_debug(2, "              heap_len=%ld pres_cell->min_cost=%f time_lag=%d",
            heap_len, pres_cell->min_cost, time_lag);
    while (heap_len-- > 0 && pres_cell->min_cost < init_time + time_lag + 1.0) {
        ros_max = DATA(map_max, pres_cell->row, pres_cell->col);
        ros_base = DATA(map_base, pres_cell->row, pres_cell->col);
        dir = DATA(map_dir, pres_cell->row, pres_cell->col);

        /*Select end cells of links of the present cell */
        select_linksB(pres_cell, least / 2, comp_dens);

#ifdef DEBUG
        to_cell = front_cell;
        while (to_cell != NULL) {
            printf("(%d,%d) ", to_cell->row, to_cell->col);
            to_cell = to_cell->next;
        }
#endif
        /*Get a cell in the list each time, and compute cumulative costs
         *via the current spread cell*/
        to_cell = front_cell;
        while (to_cell != NULL) {
            /*calculate cumulative costs,
             *function returns -1 if detected a barrier */
            if (cumulative(pres_cell, to_cell, ros_max, ros_base, dir,
                           &min_cost) == -1) {
                old_to_cell = to_cell;
                to_cell = to_cell->next;
                front_cell = to_cell;
                G_free(old_to_cell);
                continue;
            }

            G_debug(2, "        finish a link: cost(%d,%d)->(%d,%d)=%f",
                    pres_cell->row, pres_cell->col, to_cell->row, to_cell->col,
                    min_cost);
            /*update the cumulative time/cost */
            update(pres_cell, to_cell->row, to_cell->col, to_cell->angle,
                   min_cost);
            old_to_cell = to_cell;
            to_cell = to_cell->next;
            front_cell = to_cell;
            G_free(old_to_cell);
        }

        /*compute spotting fires */
        if (spotting)
            spot(pres_cell, dir);

        /*mark a visited cell */
        DATA(map_visit, pres_cell->row, pres_cell->col) = YES;
#if 0
        if (display)
            draw_a_cell(pres_cell->row, pres_cell->col,
                        (int)pres_cell->min_cost);
#endif

        cell_count++;
        if ((100 * cell_count / ncells) % 2 == 0 &&
            (100 * (cell_count + (int)(0.009 * ncells)) / ncells) % 2 == 0) {
            G_percent(cell_count, ncells, 2);
        }

        get_minHa(heap, pres_cell, heap_len);
        G_debug(2,
                "in while:     heap_len=%ld pres_cell->min_cost=%f time_lag=%d",
                heap_len, pres_cell->min_cost, time_lag);
    } /*end 'while (heap_len-- >0)' */
    G_free(pres_cell);

    /*Assign min_cost values to un-reached area */
    for (row = 0; row < nrows; row++) {
        for (col = 0; col < ncols; col++) {
            if (!DATA(map_visit, row, col)) {
                DATA(map_out, row, col) = (float)BARRIER;
                if (x_out)
                    DATA(map_x_out, row, col) = 0;
                if (y_out)
                    DATA(map_y_out, row, col) = 0;
            }
        }
    }
    G_debug(2, "end spread");
} /*end spread () */

/******* function computing cumulative spread time/cost, ***************
 ******* good for both adjacent cell links and non-adjacent cell links */

int cumulative(struct costHa *pres_cell, struct cell_ptrHa *to_cell,
               int ros_max, int ros_base, int dir, float *min_cost)
{
    float ros, xros, cost;
    float xstep_len;
    float cos_angle, sin_angle;
    int xrow, xcol, xsteps, count;

    /*most of the actions below calculate the cumulative time/cost,
     *from the current spread cell, of the end cell of one link*/

    sin_angle = sin(to_cell->angle);
    cos_angle = cos(to_cell->angle);

    if (abs(pres_cell->row - to_cell->row) >
        abs(pres_cell->col - to_cell->col)) {
        xsteps = abs(pres_cell->row - to_cell->row);
        xstep_len = 1 / cos_angle;
        if (xstep_len < 0.0)
            xstep_len = -xstep_len;
    }
    else {
        xsteps = abs(pres_cell->col - to_cell->col);
        xstep_len = 1 / sin_angle;
        if (xstep_len < 0.0)
            xstep_len = -xstep_len;
    }

    /*ROS value based on a 'from_cell', (elliptical cases) */
    ros = ros_base / (1 - (1 - ros_base / (float)ros_max) *
                              cos(to_cell->angle - dir % 360 * PI / 180));

    /*the next cell */
    xrow = pres_cell->row - xstep_len * cos_angle + 0.5;
    xcol = pres_cell->col + xstep_len * sin_angle + 0.5;

    cost = 0.0;
    count = 1;
    while (count <= xsteps) {
        /*Can't go through a barrier in a path */
        if (DATA(map_base, xrow, xcol) <= 0)
            return -1;

        /*ROS value based on current 'to_cell', (elliptical cases) */
        xros = DATA(map_base, xrow, xcol) /
               (1 - (1 - DATA(map_base, xrow, xcol) /
                             (float)DATA(map_max, xrow, xcol)) *
                        cos(to_cell->angle -
                            DATA(map_dir, xrow, xcol) % 360 * PI / 180));
        /*Calculate cost to this cell */
        cost = cost + 0.5 * (xstep_len * window.ns_res / ros +
                             xstep_len * window.ns_res / xros);

        /*Update temp cell along the path, and counter */
        ros = xros;
        xrow = pres_cell->row - count * xstep_len * cos_angle + 0.5;
        xcol = pres_cell->col + count * xstep_len * sin_angle + 0.5;
        count++;
    } /*end'while (count<= ..)' */
    G_debug(2, "                in cumulatvie() cost=%.2f pre min_cost=%.2f",
            cost, *min_cost);
    /*from the origin, cumulative time/cost of the end cell of one link */
    *min_cost = pres_cell->min_cost + cost;
    G_debug(2, "                in cumulatvie()          post min_cost=%.2f",
            *min_cost);

    return 0;
}

/****** function for updating the cumulative cost/time, possibaly     ********
 ****** back path x,y coordinates, both in the output(s) and the heap ********/

void update(struct costHa *pres_cell, int row, int col, double angle,
            float min_cost)
{
    if (DATA(map_out, row, col) < -1.0) {
        G_debug(2, "        insert: out(%d,%d)=%f min_cost=%f", row, col,
                DATA(map_out, row, col), min_cost);
        DATA(map_out, row, col) = min_cost;
        if (x_out)
            DATA(map_x_out, row, col) = pres_cell->col;
        if (y_out)
            DATA(map_y_out, row, col) = pres_cell->row;

        insertHa(min_cost, angle, row, col, heap, &heap_len);
#if 0
        if (display && min_cost < init_time + time_lag + 1.0)
            draw_a_burning_cell(row, col);
#endif
    }
    else {
        if (DATA(map_out, row, col) > min_cost + 0.001) {
            G_debug(2, "        replace: out(%d,%d)=%f min_cost=%f", row, col,
                    DATA(map_out, row, col), min_cost);
            DATA(map_out, row, col) = min_cost;
            if (x_out)
                DATA(map_x_out, row, col) = pres_cell->col;
            if (y_out)
                DATA(map_y_out, row, col) = pres_cell->row;

            replaceHa(min_cost, angle, row, col, heap, &heap_len);
#if 0
            if (display && min_cost < init_time + time_lag + 1.0)
                draw_a_burning_cell(row, col);
#endif
        }
    }
}
