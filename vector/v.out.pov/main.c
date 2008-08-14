/* ***************************************************************
 * *
 * * MODULE:       v.out.render
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Export vector to renderers' format (PovRay)
 * *               
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int i, j, centroid, otype, count;
    char *mapset;
    int field = 1;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt;
    struct Option *size_opt, *zmod_opt, *objmod_opt;
    FILE *fd;

    /* Vector */
    struct Map_info In;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int type, cat;

    G_gisinit(argv[0]);

    /* Module options */
    module = G_define_module();
    module->keywords = _("vector, export");
    module->description =
	"Converts to POV-Ray format, GRASS x,y,z -> POV-Ray x,z,y";

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,centroid,line,boundary,area,face,kernel";
    type_opt->answer = "point,line,area,face";

    out_opt = G_define_option();
    out_opt->key = "output";
    out_opt->type = TYPE_STRING;
    out_opt->required = YES;
    out_opt->description = "Output file";

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_STRING;
    size_opt->required = NO;
    size_opt->answer = "10";
    size_opt->description = "Radius of sphere for points and tube for lines. "
	"May be also variable, e.g. grass_r.";

    zmod_opt = G_define_option();
    zmod_opt->key = "zmod";
    zmod_opt->type = TYPE_STRING;
    zmod_opt->required = NO;
    zmod_opt->answer = "";
    zmod_opt->description =
	"Modifier for z coordinates, this string is appended to each z coordinate.\n"
	"\t\tExamples: '*10', '+1000', '*10+100', '*exaggeration'";

    objmod_opt = G_define_option();
    objmod_opt->key = "objmod";
    objmod_opt->type = TYPE_STRING;
    objmod_opt->required = NO;
    objmod_opt->answer = "";
    objmod_opt->description =
	"Object modifier (OBJECT_MODIFIER in POV-Ray documentation).\n"
	"\t\tExample: \"pigment { color red 0 green 1 blue 0 }\"";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Check output type */
    otype = Vect_option_to_types(type_opt);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL) {
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);
    }

    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, mapset);

    /* Open output file */
    if ((fd = fopen(out_opt->answer, "w")) == NULL) {
	Vect_close(&In);
	G_fatal_error("Cannot open output file '%s'", out_opt->answer);
    }

    count = 0;
    /* Lines */
    if ((otype &
	 (GV_POINTS | GV_LINES | GV_BOUNDARY | GV_CENTROID | GV_FACE |
	  GV_KERNEL))) {
	for (i = 1; i <= Vect_get_num_lines(&In); i++) {
	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(2, "line = %d type = %d", i, type);
	    if (!(otype & type)) {
		continue;
	    }

	    /* Vect_cat_get (Cats, field, &cat); */

	    switch (type) {
	    case GV_POINT:
	    case GV_CENTROID:
	    case GV_KERNEL:
		fprintf(fd, "sphere { <%f, %f %s, %f>, %s\n%s\n}\n",
			Points->x[0], Points->z[0], zmod_opt->answer,
			Points->y[0], size_opt->answer, objmod_opt->answer);
		count++;
		break;
	    case GV_LINE:
	    case GV_BOUNDARY:
		if (Points->n_points < 2)
		    break;	/* At least 2 points */

		fprintf(fd, "sphere_sweep { linear_spline %d,\n",
			Points->n_points);
		for (j = 0; j < Points->n_points; j++) {
		    fprintf(fd, " <%f, %f %s, %f>, %s\n",
			    Points->x[j], Points->z[j], zmod_opt->answer,
			    Points->y[j], size_opt->answer);
		}
		fprintf(fd, " %s\n}\n", objmod_opt->answer);
		count++;
		break;
	    case GV_FACE:
		if (Points->n_points < 3)
		    break;	/* At least 3 points */

		Vect_append_point(Points, Points->x[0], Points->y[0], Points->z[0]);	/* close */
		fprintf(fd, "polygon { %d, \n", Points->n_points);
		for (j = 0; j < Points->n_points; j++) {
		    fprintf(fd, " <%f, %f %s, %f>\n",
			    Points->x[j], Points->z[j], zmod_opt->answer,
			    Points->y[j]);
		}
		fprintf(fd, " %s\n}\n", objmod_opt->answer);
		count++;
		break;
	    }
	}
    }

    /* Areas (run always to count features of different type) */
    if (otype & GV_AREA) {
	for (i = 1; i <= Vect_get_num_areas(&In); i++) {
	    /* TODO : Use this later for attributes from database: */
	    centroid = Vect_get_area_centroid(&In, i);
	    cat = -1;
	    if (centroid > 0) {
		Vect_read_line(&In, NULL, Cats, centroid);
		Vect_cat_get(Cats, field, &cat);
	    }
	    G_debug(2, "area = %d centroid = %d", i, centroid);

	    /* Geometry */
	    /* Area */
	    Vect_get_area_points(&In, i, Points);
	    if (Points->n_points > 2) {
		for (j = 0; j < Points->n_points; j++) {
		    fprintf(fd, "polygon { %d, \n", Points->n_points);
		    for (j = 0; j < Points->n_points; j++) {
			fprintf(fd, " <%f, %f %s, %f>\n",
				Points->x[j], Points->z[j], zmod_opt->answer,
				Points->y[j]);
		    }
		    fprintf(fd, " %s\n}\n", objmod_opt->answer);
		}

		/* TODO: Isles */
		/*
		   for ( k = 0; k < Vect_get_area_num_isles (&In, i); k++ ) {
		   Vect_get_isle_points ( &In, Vect_get_area_isle (&In, i, k), Points );
		   for ( j = 0; j < Points->n_points; j++ ) {
		   }
		   }
		 */
		count++;
	    }
	}
    }

    fclose(fd);
    Vect_close(&In);

    /* Summary */
    G_message(_("%d features written"), count);

    exit(EXIT_SUCCESS);
}
