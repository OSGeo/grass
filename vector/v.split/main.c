
/***************************************************************
 *
 * MODULE:       v.split
 * 
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               update for GRASS 7 Markus Metz
 *
 * PURPOSE:      Split lines to segments
 *               
 * COPYRIGHT:    (C) 2001-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#define FROM_KILOMETERS  1000.0
#define FROM_FEET        0.3048
#define FROM_SFEET       1200.0 / 3937.0
#define FROM_MILES       1609.344
#define FROM_NAUTMILES   1852.0


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *layer_opt, *out_opt, *length_opt,
                  *units_opt, *vertices_opt;
    struct Flag *nosplit_flag;
    
    struct Map_info In, Out;
    struct line_pnts *Points, *Points2, *Points3;
    struct line_cats *Cats;

    int line, nlines, layer, nosplit;
    double length = -1;
    int vertices = 0;
    double (*line_length) ();
    int geodesic = 0;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    module->description = _("Splits vector lines to shorter segments.");
    
    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    layer_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    length_opt = G_define_option();
    length_opt->key = "length";
    length_opt->type = TYPE_DOUBLE;
    length_opt->required = NO;
    length_opt->multiple = NO;
    length_opt->description = _("Maximum segment length");

    units_opt = G_define_option();
    units_opt->key = "units";
    units_opt->type = TYPE_STRING;
    units_opt->required = NO;
    units_opt->multiple = NO;
    units_opt->options = "map,meters,kilometers,feet,surveyfeet,miles,nautmiles";
    units_opt->answer = "map";
    units_opt->description = _("Length units");
    
    vertices_opt = G_define_option();
    vertices_opt->key = "vertices";
    vertices_opt->type = TYPE_INTEGER;
    vertices_opt->required = NO;
    vertices_opt->multiple = NO;
    vertices_opt->description = _("Maximum number of vertices in segment");

    nosplit_flag = G_define_flag();
    nosplit_flag->key = 'n';
    nosplit_flag->label = _("Add new vertices, but do not split");
    nosplit_flag->description = _("Applies only to 'length' option");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    if ((length_opt->answer && vertices_opt->answer) ||
	!(length_opt->answer || vertices_opt->answer))
	G_fatal_error(_("Use either length or vertices"));

    line_length = NULL;

    if (length_opt->answer) {
	length = atof(length_opt->answer);
	if (length <= 0)
	    G_fatal_error(_("Length must be positive but is %g"), length);

	/* convert length to meters */
	if (strcmp(units_opt->answer, "meters") == 0)
	    /* do nothing */ ;
	else if (strcmp(units_opt->answer, "kilometers") == 0)
	    length *= FROM_KILOMETERS;
	else if (strcmp(units_opt->answer, "feet") == 0)
	    length *= FROM_FEET;
	else if (strcmp(units_opt->answer, "surveyfeet") == 0)
	    length *= FROM_SFEET;
	else if (strcmp(units_opt->answer, "miles") == 0)
	    length *= FROM_MILES;
	else if (strcmp(units_opt->answer, "nautmiles") == 0)
	    length *= FROM_NAUTMILES;
	else if (strcmp(units_opt->answer, "map") != 0)
	    G_fatal_error(_("Unknown unit %s"), units_opt->answer); 

	/* set line length function */
	if (G_projection() == PROJECTION_LL) {
	    if (strcmp(units_opt->answer, "map") == 0)
		line_length = Vect_line_length;
	    else {
		line_length = Vect_line_geodesic_length;
		geodesic = 1;
	    }
	}
	else {
	    double factor;
	    
	    line_length = Vect_line_length;
	    
	    /* convert length to map units */
	    if ((factor = G_database_units_to_meters_factor()) == 0)
		G_fatal_error(_("Can not get projection units"));
	    else if (strcmp(units_opt->answer, "map") != 0) {
		/* meters to units */
		length = length / factor;
	    }
	}
	if (strcmp(units_opt->answer, "map") == 0)
	    G_verbose_message(_("Length in map units: %g"), length);
	else
	    G_verbose_message(_("Length in meters: %g"), length);
    }

    if (vertices_opt->answer) {
	vertices = atoi(vertices_opt->answer);
	if (vertices < 2)
	    G_fatal_error(_("Number of vertices must be at least 2"));
    }
    nosplit = nosplit_flag->answer;
    
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", layer_opt->answer);
    layer = Vect_get_field_number(&In, layer_opt->answer);
    
    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));
    
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    Vect_copy_tables(&In, &Out, layer);
    
    Points = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Points3 = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(&In);

    for (line = 1; line <= nlines; line++) {
	int ltype;

	G_percent(line, nlines, 1);

	if (!Vect_line_alive(&In, line))
	    continue;

	ltype = Vect_read_line(&In, Points, Cats, line);

	if (layer != -1 && !Vect_cat_get(Cats, layer, NULL))
	  continue;

	if (ltype & GV_LINES) {
	    if (length > 0) {
		double l, from, to, step;

		l = line_length(Points);

		if (l <= length) {
		    Vect_write_line(&Out, ltype, Points, Cats);
		}
		else {
		    long n, i;

		    G_debug(3, "l: %f, length: %f", l, length);

		    n = ceil(l / length);
		    if (geodesic)
			l = Vect_line_length(Points);

		    step = l / n;
		    from = 0.;
		    
		    G_debug(3, "n: %ld, step: %f", n, step);
		    
		    if (nosplit)
			Vect_reset_line(Points3);

		    for (i = 0; i < n; i++) {
			int ret;
			double x, y, z;

			if (i == n - 1) {
			    to = l;	/* to be sure that it goes to end */
			}
			else {
			    to = from + step;
			}

			ret = Vect_line_segment(Points, from, to, Points2);
			if (ret == 0) {
			    G_warning(_("Unable to make line segment: %f - %f (line length = %f)"),
				      from, to, l);
			    continue;
			}

			/* To be sure that the coordinates are identical */
			if (i > 0) {
			    Points2->x[0] = x;
			    Points2->y[0] = y;
			    Points2->z[0] = z;
			}
			if (i == n - 1) {
			    Points2->x[Points2->n_points - 1] =
				Points->x[Points->n_points - 1];
			    Points2->y[Points2->n_points - 1] =
				Points->y[Points->n_points - 1];
			    Points2->z[Points2->n_points - 1] =
				Points->z[Points->n_points - 1];
			}

			if (nosplit) {
			    if (Points3->n_points > 0)
				Points3->n_points--;
			    Vect_append_points(Points3, Points2, GV_FORWARD);
			}
			else
			    Vect_write_line(&Out, ltype, Points2, Cats);

			/* last point */
			x = Points2->x[Points2->n_points - 1];
			y = Points2->y[Points2->n_points - 1];
			z = Points2->z[Points2->n_points - 1];

			from += step;
		    }
		    if (nosplit)
			Vect_write_line(&Out, ltype, Points3, Cats);
		}
	    }
	    else {
		int start = 0;	/* number of coordinates written */

		while (start < Points->n_points - 1) {
		    int i, v = 0;

		    Vect_reset_line(Points2);
		    for (i = 0; i < vertices; i++) {
			v = start + i;
			if (v == Points->n_points)
			    break;

			Vect_append_point(Points2, Points->x[v], Points->y[v],
					  Points->z[v]);
		    }

		    Vect_write_line(&Out, ltype, Points2, Cats);

		    start = v;
		}
	    }
	}
	else {
	    Vect_write_line(&Out, ltype, Points, Cats);
	}
    }

    Vect_close(&In);
    Vect_build(&Out);
    Vect_close(&Out);
    
    exit(EXIT_SUCCESS);
}
