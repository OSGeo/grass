#include <stdio.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "grid_structs.h"
#include "local_proto.h"

static int write_line(struct grid_description *grid_info, 
                      struct Map_info *Map, struct line_pnts *Points, 
                      struct line_cats *Cats, int *cat, int type,
	              double x1, double y1, double x2, double y2,
	              int nbreaks)
{
    int i;
    double x, y;
    double dx, dy;

    Vect_reset_line(Points);
    Vect_reset_cats(Cats);

    dx = x2 - x1;
    dy = y2 - y1;

    x = x1;
    y = y1;
    rotate(&x, &y, grid_info->xo, grid_info->yo, grid_info->angle);
    Vect_append_point(Points, x, y, 0);

    for (i = 1; i < nbreaks; i++) {
	x = x1 + dx * i / nbreaks;
	y = y1 + dy * i / nbreaks;
	rotate(&x, &y, grid_info->xo, grid_info->yo, grid_info->angle);
	Vect_append_point(Points, x, y, 0);
    }

    x = x2;
    y = y2;
    rotate(&x, &y, grid_info->xo, grid_info->yo, grid_info->angle);
    Vect_append_point(Points, x, y, 0);

    if (type == GV_LINE) {
	Vect_cat_set(Cats, 1, *cat);
	(*cat)++;
    }

    Vect_write_line(Map, type, Points, Cats);

    return 0;
}

int hexgrid(struct grid_description *grid_info, struct Map_info *Map,
               int nbreaks, int otype)
{
    int row, col, rows, cols;
    double x1, y1, x2, y2;
    double north, east;
    double angle;
    int ptype, ltype;
    int cat;

    struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    rows = grid_info->num_vect_rows;
    cols = grid_info->num_vect_cols;
    angle = grid_info->angle;

    ptype = (otype & GV_POINTS);
    ltype = (otype & GV_LINES);

    nbreaks++;

    if ((otype & GV_POINTS)) {
	if (ptype != GV_POINT && ptype != GV_CENTROID)
	    G_fatal_error("Wrong point type");
    }
    if ((otype & GV_LINES)) {
	if (ltype != GV_LINE && ltype != GV_BOUNDARY)
	    G_fatal_error("Wrong line type");
    }
    cat = 1;

    G_message(_("Writing out hexagon grid..."));
    for (row = 0; row < rows; row++) {

	G_percent(row, rows, 9);
	north = grid_info->north - grid_info->rstep * (row + 1);

	if (row & 1) {
	    if (otype & GV_POINTS) {
		for (col = 1; col < cols; col += 2) {
		    east = grid_info->west + grid_info->cstep * col + grid_info->crad;
		    x1 = east;
		    y1 = north;

		    Vect_reset_cats(Cats);
		    Vect_cat_set(Cats, 1, cat);
		    cat++;
		    Vect_reset_line(Points);
		    rotate(&x1, &y1, grid_info->xo, grid_info->yo, angle);
		    Vect_append_point(Points, x1, y1, 0);
		    Vect_write_line(Map, ptype, Points, Cats);
		}
	    }
	    /* close last col */
	    col = cols - 1;
	    if ((col & 1) && (otype & GV_LINES)) {

		x1 = grid_info->west + grid_info->cstep * (col + 1);
		y1 = grid_info->north - grid_info->rstep * row;
		x2 = grid_info->west + grid_info->cstep * (col + 1) + grid_info->crad * 0.5;
		y2 = grid_info->north - grid_info->rstep * (row + 1);

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);

		x1 = x2;
		y1 = y2;
		x2 = grid_info->west + grid_info->cstep * (col + 1);
		y2 = grid_info->north - grid_info->rstep * (row + 2);

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);
	    }
	}
	else {
	    for (col = 0; col < cols; col += 2) {
		east = grid_info->west + grid_info->cstep * col + grid_info->crad;

		if (otype & GV_POINTS) {
		    Vect_reset_cats(Cats);
		    Vect_cat_set(Cats, 1, cat);
		    cat++;
		    Vect_reset_line(Points);
		    x1 = east;
		    y1 = north;
		    rotate(&x1, &y1, grid_info->xo, grid_info->yo, angle);
		    Vect_append_point(Points, x1, y1, 0);
		    Vect_write_line(Map, ptype, Points, Cats);
		}
		if (otype & GV_LINES) {
		    /* draw hexagon without bottom */
		    x1 = grid_info->west + grid_info->cstep * col + grid_info->crad * 0.5;
		    y1 = grid_info->north - grid_info->rstep * (row + 2);
		    x2 = grid_info->west + grid_info->cstep * col;
		    y2 = grid_info->north - grid_info->rstep * (row + 1);

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);

		    x1 = x2;
		    y1 = y2;
		    x2 = grid_info->west + grid_info->cstep * col + grid_info->crad * 0.5;
		    y2 = grid_info->north - grid_info->rstep * row;

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);

		    x1 = x2;
		    y1 = y2;
		    x2 = grid_info->west + grid_info->cstep * (col + 1);

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);

		    x1 = x2;
		    y1 = y2;
		    x2 = grid_info->west + grid_info->cstep * (col + 1) + grid_info->crad * 0.5;
		    y2 = grid_info->north - grid_info->rstep * (row + 1);

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);

		    x1 = x2;
		    y1 = y2;
		    x2 = grid_info->west + grid_info->cstep * (col + 1);
		    y2 = grid_info->north - grid_info->rstep * (row + 2);

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);
		}
		/* next to left */
		if ((otype & GV_LINES) && col < cols - 1) {

		    x1 = grid_info->west + grid_info->cstep * (col + 1) + grid_info->crad * 0.5;
		    y1 = north;
		    x2 = grid_info->west + grid_info->cstep * (col + 2);
		    y2 = y1;

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);
		}
	    }
	}
    }
    /* close last row */
    if (otype & GV_LINES) {
	if ((row - 1) & 1) {
	    for (col = 1; col < cols; col += 2) {

		/* close even rows */
		x1 = grid_info->west + grid_info->cstep * (col - 1) + grid_info->crad * 0.5;
		y1 = grid_info->north - grid_info->rstep * row;
		x2 = grid_info->west + col * grid_info->cstep;
		y2 = y1;

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);

		x1 = grid_info->west + grid_info->cstep * col;
		y1 = grid_info->north - grid_info->rstep * row;
		x2 = grid_info->west + grid_info->cstep * col + grid_info->crad * 0.5;
		y2 = grid_info->north - grid_info->rstep * (row + 1);

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);

		x1 = x2;
		y1 = y2;
		x2 = grid_info->west + grid_info->cstep * (col + 1);

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);

		if (col < cols - 1) {
		    x1 = x2;
		    y1 = y2;
		    x2 = grid_info->west + grid_info->cstep * (col + 1) + grid_info->crad * 0.5;
		    y2 = grid_info->north - grid_info->rstep * row;

		    write_line(grid_info, Map, Points, Cats, &cat, ltype,
			       x1, y1, x2, y2, nbreaks);
		}
	    }
	    /* close last col */
	    col = cols - 1;
	    if (!(col & 1)) {
		/* close even rows */
		x1 = grid_info->west + grid_info->cstep * col + grid_info->crad * 0.5;
		y1 = grid_info->north - grid_info->rstep * row;
		x2 = grid_info->west + grid_info->cstep * (col + 1);
		y2 = y1;

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);
	    }
	}
	else {
	    y1 = grid_info->north - (row + 1) * grid_info->rstep;
	    y2 = y1;
	    for (col = 0; col < cols; col += 2) {

		x1 = grid_info->west + grid_info->cstep * col + grid_info->crad * 0.5;
		x2 = grid_info->west + grid_info->cstep * (col + 1);

		write_line(grid_info, Map, Points, Cats, &cat, ltype,
			   x1, y1, x2, y2, nbreaks);
	    }
	}
    }

    G_percent(row, rows, 4);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return (cat);
}
