#include <stdio.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "grid_structs.h"
#include "local_proto.h"

static double xarray[10];
static double yarray[10];

#define  NUM_POINTS  2

int write_vect(double x1, double y1, double x2, double y2,
	       struct Map_info *Map, struct line_pnts *Points, int out_type)
{
    static struct line_cats *Cats = NULL;

    if (!Cats) {
	Cats = Vect_new_cats_struct();
    }

    xarray[0] = x1;
    xarray[1] = x2;
    yarray[0] = y1;
    yarray[1] = y2;

    if (0 > Vect_copy_xyz_to_pnts(Points, xarray, yarray, NULL, NUM_POINTS))
	G_fatal_error(_("Out of memory"));
    Vect_write_line(Map, out_type, Points, Cats);

    return 0;
}

int write_grid(struct grid_description *grid_info, struct Map_info *Map,
               int nbreaks, int out_type, int diag)
{

    int i, k, j;
    int rows, cols;
    int num_v_rows, num_v_cols;
    double x, y, x_len, y_len;
    double sx, sy;
    double width, height;
    double next_x, next_y;
    double snext_x, snext_y;
    double xdiag, ydiag;
    double angle, dum;

    struct line_pnts *Points;

    Points = Vect_new_line_struct();

    num_v_rows = grid_info->num_vect_rows;
    num_v_cols = grid_info->num_vect_cols;
    rows = grid_info->num_rows;
    cols = grid_info->num_cols;
    width = grid_info->width;
    height = grid_info->height;
    angle = grid_info->angle;

    /*
     * For latlon, must draw in shorter sections
     * to make sure that each section of the grid
     * line is less than half way around the globe
     */
    x_len = width / (1. * nbreaks + 1);
    y_len = height / (1. * nbreaks + 1);

    /* write out all the vector lengths (x vectors) of the entire grid  */
    G_message(_("Writing out vector rows..."));
    y = grid_info->south;
    for (i = 0; i < num_v_rows; ++i) {
	double startx;

	startx = grid_info->west;
	G_percent(i, num_v_rows, 2);

	for (k = 0; k < cols; k++) {
	    x = startx;
            j = 0;
	    do {
		if (j < nbreaks)
		    next_x = x + x_len;
		else
		    next_x = startx + width;

		sx = x;
		sy = y;
		snext_x = next_x;
		dum = y;

		rotate(&x, &y, grid_info->xo, grid_info->yo,
		       angle);
		rotate(&next_x, &dum, grid_info->xo, grid_info->yo, 
		       angle);
		write_vect(x, y, next_x, dum, Map, Points, out_type);
		if (diag && i < num_v_rows - 1) {
		    xdiag = snext_x;
		    ydiag = sy + height;
		    rotate(&xdiag, &ydiag, grid_info->xo, grid_info->yo, 
			   angle);
		    write_vect(x, y, xdiag, ydiag, Map, Points, out_type);

		    xdiag = sx;
		    ydiag = sy + height;
		    rotate(&xdiag, &ydiag, grid_info->xo, grid_info->yo, 
			   angle);
		    write_vect(xdiag, ydiag, next_x, dum, Map, Points, out_type);
		}

		y = sy;
		x = next_x = snext_x;
                j++;
	    } while (j <= nbreaks);
	    startx += width;
	}
	y += height;
    }
    G_percent(1, 1, 1);
    
    /* write out all the vector widths (y vectors) of the entire grid  */
    G_message(_("Writing out vector columns..."));
    x = grid_info->west;
    for (i = 0; i < num_v_cols; ++i) {
        double starty;
	starty = grid_info->south;
	G_percent(i, num_v_cols, 2);

	for (k = 0; k < rows; k++) {
	    y = starty;
	    j = 0;
	    do {
	        if (j < nbreaks)
		    next_y = y + y_len;
	        else
		    next_y = starty + height;

		sx = x;
		sy = y;
		snext_y = next_y;
		dum = x;
		rotate(&x, &y, grid_info->xo, grid_info->yo, angle);
		rotate(&dum, &next_y, grid_info->xo, grid_info->yo,
		    angle);

		write_vect(x, y, dum, next_y, Map, Points, out_type);

	        x = sx;
	        y = next_y = snext_y;
	        j++;
	    } while (j <= nbreaks);
	    /* To get exactly the same coordinates as above, y+=width is wrong */
	    starty += height;
	}
	x += width;
    }
    G_percent(1, 1, 1);
    
    /* new with Vlib */
    Vect_destroy_line_struct(Points);

    return (0);
}
