
/****************************************************************************
 *
 * MODULE:       v.build.polylines
 * AUTHOR(S):    Mark Lake (original contributor)
 *               Major rewrite by Radim Blazek, October 2002
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 *               Martin Landa <landa.martin gmail.com> (cats)
 *               Markus Metz (geometry type management, cats, attributes)
 * PURPOSE:      
 * COPYRIGHT:    (C) 2002-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
   v.build.polylines

   *****

   Mark Lake  5/4/00

   University College London
   Institute of Archaeology
   31-34 Gordon Square
   London.  WC1H 0PY

   Email: mark.lake@ucl.ac.uk

   Updated to grass 5.7 by Radim Blazek

   *****

   PURPOSE 

   1) Convert lines or mixed lines and polylines to polylines.  Preserve points 
   if present.  Allow the user to label the new lines as lines or area edges.

   *****

   METHOD

   1) A line is a single straight line segment defined by one start
   node, one end node and no other nodes.  In contrast, a polyline
   consists of a number of straight line segments each joined by a common
   node which is connected to exactly two lines.  The start and end nodes
   of the polyline are connected to either one line, or three or more
   lines.

   Points and centroids are ignored by build process and copied to output vector.

   *****

   FILES

   1) main.c - algorithm for ifs.

   2) walk.c - functions to find start of polylines and to pick up their
   coordinates.

   3) line_coords.c - structure for storing coordinates.

   *****

   PORTABILITY

   1) Portable

   ********************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "walk.h"

int gCopy_atts;
int gAsciiout;
char gAscii_type;

int main(int argc, char **argv)
{
    int line;
    struct line_pnts *points;
    struct line_cats *Cats;

    struct Map_info map, Out;
    struct GModule *module;
    struct Option *input;
    struct Option *output;
    struct Option *cats;
    struct Option *type_opt;
    char *desc;

    int polyline;
    int *lines_visited;
    int points_in_polyline;
    int start_line;
    int nlines;
    int write_cats, copy_tables;

    int type, ltype;

    /*  Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("topology"));
    G_add_keyword(_("geometry"));
    module->description = _("Builds polylines from lines or boundaries.");

    /* Define the options */

    input = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    cats = G_define_option();
    cats->key = "cats";
    cats->type = TYPE_STRING;
    cats->description = _("Category number mode");
    cats->options = "no,first,multi";
    desc = NULL;
    G_asprintf(&desc,
	       "no;%s;first;%s;multi;%s",
	       _("Do not assign any category number to polyline"),
	       _("Assign category number of first line to polyline"),
	       _("Assign multiple category numbers to polyline"));
    cats->descriptions = desc;
    cats->answer = "no";

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Vect_check_input_output_name(input->answer, output->answer,
				 G_FATAL_EXIT);

    /* Open binary vector map at level 2 */
    Vect_set_open_level(2);
    Vect_open_old(&map, input->answer, "");

    /* Open new vector */
    G_find_vector2(output->answer, "");
    Vect_open_new(&Out, output->answer, Vect_is_3d(&map));

    /* Copy header info. */
    Vect_copy_head_data(&map, &Out);

    /* History */
    Vect_hist_copy(&map, &Out);
    Vect_hist_command(&Out);

    /* Get the number of lines in the binary map and set up record of lines visited */

    lines_visited =
	(int *)G_calloc(Vect_get_num_lines(&map) + 1, sizeof(int));

    /* Set up points structure and coordinate arrays */
    points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* Write cats */
    if (strcmp(cats->answer, "no") == 0)
	write_cats = NO_CATS;
    else if (strcmp(cats->answer, "first") == 0)
	write_cats = ONE_CAT;
    else
	write_cats = MULTI_CATS;

    if (type_opt->answer)
	type = Vect_option_to_types(type_opt);
    else
	type = GV_LINES;

    /* Step over all lines in binary map */
    polyline = 0;
    nlines = 0;

    copy_tables = (write_cats != NO_CATS);

    for (line = 1; line <= Vect_get_num_lines(&map); line++) {
	Vect_reset_cats(Cats);
	ltype = Vect_read_line(&map, NULL, NULL, line);

	if ((ltype & GV_LINES) && (ltype & type))
	    nlines++;
	else {
	    /* copy points to output as they are, with cats */
	    Vect_read_line(&map, points, Cats, line);
	    Vect_write_line(&Out, ltype, points, Cats);
	    if (Cats->n_cats > 0)
		copy_tables = 1;
	    continue;
	}

	/* Skip line if already visited from another */
	if (lines_visited[line])
	    continue;

	/* Only get here if line is not previously visited */

	/* Find start of this polyline */
	start_line = walk_back(&map, line, ltype);

	G_debug(1, "Polyline %d: start line = %d", polyline, start_line);

	/* Walk forward and pick up coordinates */
	points_in_polyline =
	    walk_forward_and_pick_up_coords(&map, start_line, ltype, points,
					    lines_visited, Cats, write_cats);

	/* Write the line (type of the first line is used) */
	Vect_write_line(&Out, ltype, points, Cats);

	polyline++;
    }

    G_verbose_message(_("%d lines or boundaries found in input vector map"),
		      nlines, Vect_get_name(&map), Vect_get_mapset(&map));
    G_verbose_message(_("%d polylines stored in output vector map"),
		      polyline, Vect_get_name(&Out), Vect_get_mapset(&Out));

    /* Copy (all linked) tables if needed */
    if (copy_tables) {
        if (Vect_copy_tables(&map, &Out, 0))
            G_warning(_("Failed to copy attribute table to output map"));
    }

    /* Tidy up */
    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(Cats);
    G_free(lines_visited);
    Vect_close(&map);

    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
