#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"


FILE *output;

int measurements(int color1, int color2, int s_flag, int m_flag, int k_flag)
{
    double *x, *y;
    int npoints, nalloc;
    double area;
    double cur_ux, cur_uy;
    double length;
    double ux, uy;
    int button;
    int cur_screen_x, cur_screen_y;
    int screen_x, screen_y;
    struct Cell_head window;
    double t, b, l, r;

    nalloc = 128;
    x = (double *)G_calloc(nalloc, sizeof(double));
    y = (double *)G_calloc(nalloc, sizeof(double));


    /* Use stderr for TCLTK-Output */
    if (s_flag)
	output = stderr;
    else
	output = stdout;

    /* Set up area/distance calculations  */
    G_begin_polygon_area_calculations();
    G_begin_distance_calculations();

    G_get_window(&window);
    D_get_dst(&t, &b, &l, &r);
    D_do_conversions(&window, t, b, l, r);

    for (;;) {
	npoints = 0;
	if (!s_flag)
	    G_clear_screen();
	fprintf(output, "\nButtons:\n");
	fprintf(output, "Left:   where am i\n");
	fprintf(output, "Middle: set FIRST vertex\n");
	fprintf(output, "Right:  quit this\n");

	screen_y = (t + b) / 2;
	screen_x = (l + r) / 2;

	do {
	    R_get_location_with_pointer(&screen_x, &screen_y, &button);
	    cur_uy = D_d_to_u_row((double)screen_y);
	    cur_ux = D_d_to_u_col((double)screen_x);
	    if (button == 1)
		print_en(cur_ux, cur_uy, s_flag);
	    if (button == 3)
		return (0);
	} while (button != 2);

	add_point(&x, &y, &npoints, &nalloc, cur_ux, cur_uy);
	if (!s_flag)
	    G_clear_screen();
	fprintf(output, "\nLeft:   where am i\n");
	fprintf(output, "Middle: set NEXT vertex\n");
	fprintf(output, "Right:  FINISH\n");

	R_move_abs(screen_x, screen_y);
	cur_screen_x = screen_x;
	cur_screen_y = screen_y;

	length = 0.0;

	do {
	    D_use_color(color1);
	    R_get_location_with_line(cur_screen_x, cur_screen_y, &screen_x,
				     &screen_y, &button);
	    uy = D_d_to_u_row((double)screen_y);
	    ux = D_d_to_u_col((double)screen_x);
	    if (button == 1) {
		print_en(ux, uy, s_flag);
	    }
	    else if (button == 2) {
		draw_line(screen_x, screen_y, cur_screen_x, cur_screen_y,
			  color1, color2);
		add_point(&x, &y, &npoints, &nalloc, ux, uy);
		length += G_distance(cur_ux, cur_uy, ux, uy);
		print_length(length, s_flag, k_flag);
		cur_screen_x = screen_x;
		cur_screen_y = screen_y;
		cur_ux = ux;
		cur_uy = uy;
	    }
	} while (button != 3);

	R_flush();

	if (!s_flag)
	    G_clear_screen();
	fprintf(output, "\nButtons:\n");
	fprintf(output, "Left:   DO ANOTHER\n");
	fprintf(output, "Middle:\n");
	fprintf(output, "Right:  quit this\n");
	/*
	 * 10000 is sq meters per hectare
	 * 2589988 is sq meters per sq mile
	 */
	fprintf(output, "\n");
	print_length(length, s_flag, k_flag);
	if (npoints > 3) {
	    area = G_area_of_polygon(x, y, npoints);
	    if (!m_flag) {
		fprintf(output, "AREA:  %10.2f hectares\n", area / 10000);
		fprintf(output, "AREA:  %10.4f square miles\n",
			area / 2589988.11);
	    }
	    fprintf(output, "AREA:  %10.2f square meters\n", area);
	    if (k_flag)
		fprintf(output, "AREA:   %10.4f square kilometers\n",
			area / 1000000);
	}

	R_get_location_with_pointer(&screen_x, &screen_y, &button);
	if (button == 3)
	    return (0);
    }

    return 0;
}

int print_en(double e, double n, int s_flag)
{
    char buf[100];

    /* Use stderr for TCLTK-Output */
    if (s_flag)
	output = stderr;
    else
	output = stdout;

    G_format_easting(e, buf, G_projection());
    fprintf(output, "EAST:  %s\n", buf);
    G_format_northing(n, buf, G_projection());
    fprintf(output, "NORTH: %s\n", buf);

    return 0;
}

int print_length(double length, int s_flag, int k_flag)
{
    /* Use stderr for TCLTK-Output */
    if (s_flag)
	output = stderr;
    else
	output = stdout;

    fprintf(output, "LEN:   %10.2f meters\n", length);
    if (k_flag)
	fprintf(output, "LEN:   %10.4f kilometers\n", length / 1000);

    return 0;
}

int add_point(double **x, double **y,
	      int *npoints, int *nalloc, double ux, double uy)
{
    double *px, *py;

    px = *x;
    py = *y;

    if (*npoints >= *nalloc) {
	*nalloc *= 2;
	*x = px = (double *)G_realloc(px, *nalloc * sizeof(double));
	*y = py = (double *)G_realloc(py, *nalloc * sizeof(double));
    }
    px[*npoints] = ux;
    py[*npoints] = uy;
    *npoints += 1;

    return 0;
}
