#include "local_proto.h"

int read_points(char *in_point, SEGMENT * streams, SEGMENT * accum)
{
    struct Cell_head window;
    struct Map_info Map;
    struct bound_box box;
    int num_point = 0;
    int total_points = 0;
    int type, i, j, cat;
    struct line_pnts *sites;
    struct line_cats *cats;
    double absaccum;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    Vect_open_old(&Map, in_point, "");
    
    G_get_window(&window);
    Vect_region_box(&window, &box);

    while ((type = Vect_read_next_line(&Map, sites, cats)) > -1) {
	if (type != GV_POINT)
	    continue;
	if (Vect_point_in_box(sites->x[0], sites->y[0], sites->z[0], &box))
	    num_point++;
    }

    points = (OUTLET *) G_malloc(num_point * sizeof(OUTLET));
    total_points = Vect_get_num_lines(&Map);
    i = 0;

    for (j = 0; j < total_points; ++j) {

	type = Vect_read_line(&Map, sites, cats, j + 1);

	if (type != GV_POINT)
	    continue;

	if (!Vect_point_in_box(sites->x[0], sites->y[0], sites->z[0], &box))
	    continue;

	Vect_cat_get(cats, 1, &cat);

	points[i].r = (int)Rast_northing_to_row(sites->y[0], &window);
	points[i].c = (int)Rast_easting_to_col(sites->x[0], &window);
	points[i].di = 0;
	points[i].dj = 0;
	points[i].cat = cat;
	if (streams)
	    segment_get(streams, &points[i].stream, points[i].r, points[i].c);
	else
	    points[i].stream = 0;
	if (accum) {
	    segment_get(accum, &absaccum, points[i].r, points[i].c);
	    points[i].accum = fabs(absaccum);
	}
	else {
	    points[i].accum = 0;
	    points[i].status = 4;	/* default status is 'correct' */
	}
	//dodać skip category

	i++;
    }
    return num_point;
}

int write_points(char *out_vector, int number_of_points)
{

    int i;
    int r, c;
    int cat_layer_1, cat_layer_2;
    float northing, easting;
    struct Cell_head window;
    struct Map_info Out;
    struct line_pnts *Segments;
    struct line_cats *Cats;

    G_get_window(&window);
    Segments = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Vect_open_new(&Out, out_vector, 0);

    Vect_reset_line(Segments);
    Vect_reset_cats(Cats);

    for (i = 0; i < number_of_points; ++i) {

	r = points[i].r + points[i].di;
	c = points[i].c + points[i].dj;

	cat_layer_1 = points[i].cat;
	cat_layer_2 = points[i].status;
	Vect_cat_set(Cats, 1, cat_layer_1);
	Vect_cat_set(Cats, 2, cat_layer_2);
	easting = window.west + (c + .5) * window.ew_res;
	northing = window.north - (r + .5) * window.ns_res;
	Vect_append_point(Segments, easting, northing, 0);
	Vect_write_line(&Out, GV_POINT, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_reset_cats(Cats);
    }

    /* build vector */
    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);

    return 0;
}
