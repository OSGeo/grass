#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/Vect.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "proto.h"

#define WDTH 5

int display(struct Map_info *Map, struct line_pnts *,
	    const struct color_rgb *, int, int, int);

int path(struct Map_info *Map, const struct color_rgb *color,
	 const struct color_rgb *hcolor, const struct color_rgb *bgcolor,
	 int be_bold)
{
    int button, ret;
    int screen_x, screen_y;
    double x, y, nx, ny, fx, fy, tx, ty, msize;
    double x1, y1, x2, y2, maxdist;
    struct line_pnts *Points;
    int node;
    int from_disp = 0, to_disp = 0, sp_disp = 0, from_node = 0, to_node = 0;

    Points = Vect_new_line_struct();

    msize = 10 * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */
    G_debug(1, "msize = %f\n", msize);

    G_message(_("\nMouse Buttons:"));
    fprintf(stderr, _("Left:   Select From\n"));
    fprintf(stderr, _("Middle: Select To\n"));
    fprintf(stderr, _("Right:  Quit\n\n"));

    while (1) {
	R_get_location_with_pointer(&screen_x, &screen_y, &button);

	x = D_d_to_u_col((double)(screen_x));
	y = D_d_to_u_row((double)(screen_y));
	/* fprintf (stderr, "%f %f\n", x, y); */

	x1 = D_d_to_u_col((double)(screen_x - WDTH));
	y1 = D_d_to_u_row((double)(screen_y - WDTH));
	x2 = D_d_to_u_col((double)(screen_x + WDTH));
	y2 = D_d_to_u_row((double)(screen_y + WDTH));

	x1 = fabs(x2 - x1);
	y1 = fabs(y2 - y1);

	if (x1 > y1)
	    maxdist = x1;
	else
	    maxdist = y1;
	G_debug(1, "Maximum distance in map units = %f\n", maxdist);

	node = Vect_find_node(Map, x, y, 0.0, maxdist, 0);

	if (node > 0) {
	    Vect_get_node_coor(Map, node, &nx, &ny, NULL);
	    fprintf(stderr, _("Node %d: %f %f\n"), node, nx, ny);
	}

	if (sp_disp) {		/* erase old */
	    /* delete old highlight */

	    display(Map, Points, color, from_node, to_node, be_bold);

	    R_RGB_color(bgcolor->r, bgcolor->g, bgcolor->b);
	    if (!from_node)
		G_plot_line(Points->x[0], Points->y[0], Points->x[1],
			    Points->y[1]);

	    if (!to_node)
		G_plot_line(Points->x[Points->n_points - 2],
			    Points->y[Points->n_points - 2],
			    Points->x[Points->n_points - 1],
			    Points->y[Points->n_points - 1]);
	}

	switch (button) {
	case 1:
	    if (from_disp) {
		R_RGB_color(bgcolor->r, bgcolor->g, bgcolor->b);
		G_plot_icon(fx, fy, G_ICON_BOX, 0.0, msize);
	    }
	    if (node > 0) {
		fx = nx;
		fy = ny;
		from_node = 1;
	    }
	    else {
		fx = x;
		fy = y;
		from_node = 0;
	    }
	    R_RGB_color(hcolor->r, hcolor->g, hcolor->b);
	    G_plot_icon(fx, fy, G_ICON_BOX, 0.0, msize);
	    R_flush();
	    from_disp = 1;
	    break;
	case 2:
	    if (to_disp) {
		R_RGB_color(bgcolor->r, bgcolor->g, bgcolor->b);
		G_plot_icon(tx, ty, G_ICON_CROSS, 0.0, msize);
	    }
	    if (node > 0) {
		tx = nx;
		ty = ny;
		to_node = 1;
	    }
	    else {
		tx = x;
		ty = y;
		to_node = 0;
	    }
	    R_RGB_color(hcolor->r, hcolor->g, hcolor->b);
	    G_plot_icon(tx, ty, G_ICON_CROSS, 0.0, msize);
	    R_flush();
	    to_disp = 1;
	    break;
	case 3:
	    if (from_disp) {
		R_RGB_color(bgcolor->r, bgcolor->g, bgcolor->b);
		G_plot_icon(fx, fy, G_ICON_BOX, 0.0, msize);
	    }
	    if (to_disp) {
		R_RGB_color(bgcolor->r, bgcolor->g, bgcolor->b);
		G_plot_icon(tx, ty, G_ICON_CROSS, 0.0, msize);
	    }
	    return 1;
	    break;
	}
	if (from_disp && to_disp) {
	    double fdist, tdist, cost;

	    G_debug(2, "find path %f %f -> %f %f", fx, fy, tx, ty);

	    ret =
		Vect_net_shortest_path_coor(Map, fx, fy, 0.0, tx, ty, 0.0,
					    5 * maxdist, 5 * maxdist, &cost,
					    Points, NULL, NULL, NULL, &fdist,
					    &tdist);
	    if (ret == 0) {
		fprintf(stdout, _("Destination unreachable\n"));
		sp_disp = 0;
	    }
	    else {
		fprintf(stdout, _("Costs on the network = %f\n"), cost);
		fprintf(stdout, _("  Distance to the network = %f, "
				  "distance from the network = %f\n\n"),
			fdist, tdist);

		display(Map, Points, hcolor, 1, 1, be_bold);
		sp_disp = 1;
	    }

	}
	R_flush();
    };

    return 0;
}


int display(struct Map_info *Map, struct line_pnts *Points,
	    const struct color_rgb *color, int first, int last, int be_bold)
{
    int i, from, to;

    R_RGB_color(color->r, color->g, color->b);

    if (first)
	from = 0;
    else
	from = 1;
    if (last)
	to = Points->n_points;
    else
	to = Points->n_points - 1;

    if (be_bold)
	D_line_width(2);

    for (i = from; i < to - 1; i++)
	G_plot_line(Points->x[i], Points->y[i], Points->x[i + 1],
		    Points->y[i + 1]);

    if (be_bold)
	R_line_width(0);

    return 0;
}



/* Same as path() but get start/stop from the command line (for non-interactive use)
   Hamish Bowman March 2007 */
int coor_path(struct Map_info *Map, const struct color_rgb *hcolor,
	      int be_bold, double start_x, double start_y,
	      double end_x, double end_y)
{
    int ret;
    double nx, ny, fx, fy, tx, ty, msize, maxdist;
    struct line_pnts *Points;
    int start_node, end_node;
    double fdist, tdist, cost;

    Points = Vect_new_line_struct();

    msize = 10 * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */
    G_debug(1, "msize = %f\n", msize);

    /*
       x1 = D_d_to_u_col ((double)(screen_x-WDTH));
       y1 = D_d_to_u_row ((double)(screen_y-WDTH));
       x2 = D_d_to_u_col ((double)(screen_x+WDTH));
       y2 = D_d_to_u_row ((double)(screen_y+WDTH));

       x1 = fabs ( x2 - x1 );
       y1 = fabs ( y2 - y1 );

       if ( x1 > y1 ) maxdist = x1;
       else maxdist = y1;
     */

/**  maxdist = 10 pixels on the display (WDTH*2); ?
 **   ie related to zoom level ??  just use msize ?? **/
    maxdist = msize;

    G_debug(1, "Maximum distance in map units = %f\n", maxdist);


    /* Vect_find_node(): find number of nearest node, 0 if not found */
    start_node = Vect_find_node(Map, start_x, start_y, 0.0, maxdist, 0);
    if (start_node > 0) {
	Vect_get_node_coor(Map, start_node, &nx, &ny, NULL);
	fprintf(stderr, _("Node %d: %f %f\n"), start_node, nx, ny);
    }
    if (start_node > 0) {
	fx = nx;
	fy = ny;
    }
    else {
	fx = start_x;
	fy = start_y;
    }
    R_RGB_color(hcolor->r, hcolor->g, hcolor->b);
    G_plot_icon(fx, fy, G_ICON_BOX, 0.0, msize);


    end_node = Vect_find_node(Map, end_x, end_y, 0.0, maxdist, 0);
    if (end_node > 0) {
	Vect_get_node_coor(Map, end_node, &nx, &ny, NULL);
	fprintf(stderr, _("Node %d: %f %f\n"), end_node, nx, ny);
    }
    if (end_node > 0) {
	tx = nx;
	ty = ny;
    }
    else {
	tx = end_x;
	ty = end_y;
    }
    R_RGB_color(hcolor->r, hcolor->g, hcolor->b);
    G_plot_icon(tx, ty, G_ICON_CROSS, 0.0, msize);


    G_debug(2, "find path %f %f -> %f %f", fx, fy, tx, ty);

    ret =
	Vect_net_shortest_path_coor(Map, fx, fy, 0.0, tx, ty, 0.0,
				    5 * maxdist, 5 * maxdist, &cost, Points,
				    NULL, NULL, NULL, &fdist, &tdist);
    if (ret == 0) {
	fprintf(stdout, _("Destination unreachable\n"));
    }
    else {
	fprintf(stdout, _("Costs on the network = %f\n"), cost);
	fprintf(stdout, _("  Distance to the network = %f, "
			  "distance from the network = %f\n\n"),
		fdist, tdist);

	display(Map, Points, hcolor, 1, 1, be_bold);
    }

    return 0;
}
