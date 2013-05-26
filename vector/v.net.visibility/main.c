
/****************************************************************
 * MODULE:     v.net.visibility
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "visibility.h"
#include "proto.h"

/** TODO: need to check if vis and input have same set of vertices */

int main(int argc, char *argv[])
{
    struct Map_info in, out, vis;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *input, *output;	/* The input map */
    struct Option *coor, *ovis;

    struct Point *points;
    struct Line *lines;
    int num_points, num_lines;
    int n = 0;



    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("shortest path"));
    G_add_keyword(_("visibility"));
    module->description = _("Visibility graph construction.");

    /* define the arguments needed */
    input = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    coor = G_define_option();
    coor->key = "coordinate";
    coor->key_desc = "x,y";
    coor->type = TYPE_STRING;
    coor->required = NO;
    coor->multiple = YES;
    coor->description = _("One or more coordinates");

    ovis = G_define_option();
    ovis->key = "vis";
    ovis->type = TYPE_STRING;
    ovis->required = NO;
    ovis->description = _("Add points after computing the vis graph");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Vect_check_input_output_name(input->answer, output->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);

    if (Vect_open_old(&in, input->answer, "") < 1)	/* opens the map */
	G_fatal_error(_("Unable to open vector map <%s>"), input->answer);

    if (Vect_open_new(&out, output->answer, WITHOUT_Z) < 0) {
	Vect_close(&in);
	G_fatal_error(_("Unable to create vector map <%s>"), output->answer);
    }

    if (ovis->answer != NULL) {
	if (Vect_open_old(&vis, ovis->answer, "") < 1)
	    G_fatal_error(_("Unable to open vector map <%s>"), ovis->answer);

	if (Vect_copy_map_lines(&vis, &out) > 0)
	    G_fatal_error(_("Unable to copy elements from vector map <%s>"),
			  ovis->answer);
    }

    if (G_projection() == PROJECTION_LL)
	G_warning(_("Lat-long projection"));


    /* counting how many points and lines we have to allocate */
    count(&in, &num_points, &num_lines);

    /* modify the number if we have new points to add */
    if (coor->answers != NULL)
	num_points += count_new(coor->answers);

    /* and allocate */
    points = G_malloc(num_points * sizeof(struct Point));
    lines = G_malloc(num_lines * sizeof(struct Line));

    /* and finally set the lines */
    load_lines(&in, &points, &num_points, &lines, &num_lines);

    if (coor->answers != NULL)
	add_points(coor->answers, &points, &num_points);

    if (ovis->answer == NULL)
	construct_visibility(points, num_points, lines, num_lines, &out);
    else
	visibility_points(points, num_points, lines, num_lines, &out, n);

    G_free(points);
    G_free(lines);

    Vect_build(&out);
    Vect_close(&out);
    Vect_close(&in);

    exit(EXIT_SUCCESS);
}


/* count how many new points we have to add */
int count_new(char **coor)
{
    int n, i;

    /* first count how many points we are going to add */
    n = 0;
    i = 0;
    while (coor[i] != NULL) {
	n++;
	i += 2;
    }

    return n;
}

/** add points to the visibility graph
*/
void add_points(char **coor, struct Point **points, int *index_point)
{
    int i;
    double x, y;


    /* and defining the points */
    for (i = 0; coor[i] != NULL; i += 2) {
	G_scan_easting(coor[i], &x, G_projection());
	G_scan_northing(coor[i + 1], &y, G_projection());

	(*points)[*index_point].x = x;
	(*points)[*index_point].y = y;
	(*points)[*index_point].cat = -1;

	(*points)[*index_point].line1 = NULL;
	(*points)[*index_point].line2 = NULL;

	(*points)[*index_point].left_brother = NULL;
	(*points)[*index_point].right_brother = NULL;
	(*points)[*index_point].father = NULL;
	(*points)[*index_point].rightmost_son = NULL;

	(*index_point)++;

    }
}

/** counts the number of individual segments ( boundaries and lines ) and vertices
*/
void count(struct Map_info *map, int *num_points, int *num_lines)
{
    int index_line = 0;
    int index_point = 0;
    struct line_pnts *sites;
    struct line_cats *cats;
    int type, i;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    for (i = 1; i <= map->plus.n_lines; i++) {

	type = Vect_read_line(map, sites, cats, i);

	if (type != GV_LINE && type != GV_BOUNDARY && type != GV_POINT)
	    continue;

	if (type == GV_LINE) {
	    index_point += sites->n_points;
	    index_line += sites->n_points - 1;
	}
	else if (type == GV_BOUNDARY) {
	    index_point += sites->n_points - 1;
	    index_line += sites->n_points - 1;
	}
	else if (type == GV_POINT) {
	    index_point++;
	}


    }

    *num_points = index_point;
    *num_lines = index_line;

    Vect_destroy_line_struct(sites);
    Vect_destroy_cats_struct(cats);
}


/** Get the lines and boundaries from the map and load them in an array
*/
void load_lines(struct Map_info *map, struct Point **points, int *num_points,
		struct Line **lines, int *num_lines)
{
    int index_line = 0;
    int index_point = 0;
    struct line_pnts *sites;
    struct line_cats *cats;
    int cat = 0;
    int type;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    while ((type = Vect_read_next_line(map, sites, cats)) > -1) {

	if (type != GV_LINE && type != GV_BOUNDARY && type != GV_POINT)
	    continue;

	if (type == GV_LINE)
	    process_line(sites, points, &index_point, lines, &index_line, -1);
	else if (type == GV_BOUNDARY)
	    process_boundary(sites, points, &index_point, lines, &index_line,
			     cat++);
	else if (type == GV_POINT)
	    process_point(sites, points, &index_point, -1);

    }

    *num_points = index_point;
    *num_lines = index_line;

    Vect_destroy_line_struct(sites);
    Vect_destroy_cats_struct(cats);
}

void process_point(struct line_pnts *sites, struct Point **points,
		   int *index_point, int cat)
{
    (*points)[*index_point].x = sites->x[0];
    (*points)[*index_point].y = sites->y[0];
    (*points)[*index_point].cat = cat;

    (*points)[*index_point].line1 = NULL;
    (*points)[*index_point].line2 = NULL;

    (*points)[*index_point].left_brother = NULL;
    (*points)[*index_point].right_brother = NULL;
    (*points)[*index_point].father = NULL;
    (*points)[*index_point].rightmost_son = NULL;

    (*index_point)++;
}

/** extract all segments from the line 
*/
void process_line(struct line_pnts *sites, struct Point **points,
		  int *index_point, struct Line **lines, int *index_line,
		  int cat)
{
    int n = sites->n_points;
    int i;

    for (i = 0; i < n; i++) {

	(*points)[*index_point].x = sites->x[i];
	(*points)[*index_point].y = sites->y[i];
	(*points)[*index_point].cat = cat;

	if (i == 0) {
	    (*points)[*index_point].line1 = NULL;
	    (*points)[*index_point].line2 = &((*lines)[*index_line]);
	}
	else if (i == n - 1) {
	    (*points)[*index_point].line1 = &((*lines)[(*index_line) - 1]);
	    (*points)[*index_point].line2 = NULL;
	}
	else {
	    (*points)[*index_point].line1 = &((*lines)[(*index_line) - 1]);
	    (*points)[*index_point].line2 = &((*lines)[*index_line]);
	}

	(*points)[*index_point].left_brother = NULL;
	(*points)[*index_point].right_brother = NULL;
	(*points)[*index_point].father = NULL;
	(*points)[*index_point].rightmost_son = NULL;


	(*index_point)++;

	if (i < n - 1) {
	    (*lines)[*index_line].p1 = &((*points)[(*index_point) - 1]);
	    (*lines)[*index_line].p2 = &((*points)[*index_point]);
	    (*index_line)++;
	}
    }
}

/** extract all segments from the boundary 
*/
void process_boundary(struct line_pnts *sites, struct Point **points,
		      int *index_point, struct Line **lines, int *index_line,
		      int cat)
{
    int n = sites->n_points;
    int i;

    for (i = 0; i < n - 1; i++) {
	(*points)[*index_point].cat = cat;
	(*points)[*index_point].x = sites->x[i];
	(*points)[*index_point].y = sites->y[i];

	if (i == 0) {
	    (*points)[*index_point].line1 =
		&((*lines)[(*index_line) + n - 2]);
	    (*points)[*index_point].line2 = &((*lines)[*index_line]);
	}
	else {
	    (*points)[*index_point].line1 = &((*lines)[(*index_line) - 1]);
	    (*points)[*index_point].line2 = &((*lines)[*index_line]);
	}

	(*points)[*index_point].left_brother = NULL;
	(*points)[*index_point].right_brother = NULL;
	(*points)[*index_point].father = NULL;
	(*points)[*index_point].rightmost_son = NULL;


	(*index_point)++;

	if (i == n - 2) {
	    (*lines)[*index_line].p1 = &((*points)[(*index_point) - 1]);
	    (*lines)[*index_line].p2 = &((*points)[(*index_point) - n + 2]);
	}
	else {
	    (*lines)[*index_line].p1 = &((*points)[(*index_point) - 1]);
	    (*lines)[*index_line].p2 = &((*points)[*index_point]);
	}

	(*index_line)++;
    }

}
