/* ***************************************************************
 * *
 * * MODULE:       v.out.pov
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               OGR support by Martin Landa <landa.martin gmail.com>
 * *               
 * * PURPOSE:      Export vector to renderers' format (PovRay)
 * *               
 * * COPYRIGHT:    (C) 2001-2012 by the GRASS Development Team
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
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int i, j, centroid, otype, count;
    int nlines, nareas;
    int field;
    struct GModule *module;
    struct Option *in_opt, *field_opt, *out_opt, *type_opt;
    struct Option *size_opt, *zmod_opt, *objmod_opt;
    FILE *fd;

    /* Vector */
    struct Map_info In;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int type;

    G_gisinit(argv[0]);

    /* Module options */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    module->description =
	_("Converts GRASS x,y,z points to POV-Ray x,z,y format.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    
    type_opt = G_define_standard_option(G_OPT_V3_TYPE);
    type_opt->answer = "point,line,area,face";

    out_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    out_opt->required = YES;
    out_opt->description = _("Name for output POV file");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_STRING;
    size_opt->required = NO;
    size_opt->answer = "10";
    size_opt->label = _("Radius of sphere for points and tube for lines");
    size_opt->description = _("May be also variable, e.g. grass_r.");

    zmod_opt = G_define_option();
    zmod_opt->key = "zmod";
    zmod_opt->type = TYPE_STRING;
    zmod_opt->required = NO;
    zmod_opt->description = _("Modifier for z coordinates");
    zmod_opt->description = _("This string is appended to each z coordinate. "
			      "Examples: '*10', '+1000', '*10+100', '*exaggeration'");

    objmod_opt = G_define_option();
    objmod_opt->key = "objmod";
    objmod_opt->type = TYPE_STRING;
    objmod_opt->required = NO;
    objmod_opt->label = _("Object modifier (OBJECT_MODIFIER in POV-Ray documentation)");
    objmod_opt->description = _("Example: \"pigment { color red 0 green 1 blue 0 }\"");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Check output type */
    otype = Vect_option_to_types(type_opt);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);
    
    field = Vect_get_field_number(&In, field_opt->answer);
    
    /* Open output file */
    if ((fd = fopen(out_opt->answer, "w")) == NULL) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create output file <%s>"), out_opt->answer);
    }

    if (zmod_opt->answer == NULL)
	    zmod_opt->answer = G_store("");
    if (objmod_opt->answer == NULL)
	    objmod_opt->answer = G_store("");

    nlines = Vect_get_num_lines(&In);
    nareas = Vect_get_num_areas(&In);
    count = 0;
    /* Lines */
    if ((otype &
	 (GV_POINTS | GV_LINES | GV_BOUNDARY | GV_CENTROID | GV_FACE |
	  GV_KERNEL))) {
	for (i = 1; i <= nlines; i++) {
	    G_percent(i, nlines, 2);
	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(2, "line = %d type = %d", i, type);
	    
	    if (field != -1 && Vect_cat_get(Cats, field, NULL) == 0)
		continue;
	    
	    if (!(otype & type)) {
		continue;
	    }
	    
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
    if (otype & GV_AREA && nareas > 0) {
	G_message(_("Processing areas..."));
	for (i = 1; i <= nareas; i++) {
	    G_percent(i, nareas, 2);
	    /* TODO : Use this later for attributes from database: */
	    centroid = Vect_get_area_centroid(&In, i);
	    if (centroid > 0) {
		Vect_read_line(&In, NULL, Cats, centroid);
		if (field != -1 && Vect_cat_get(Cats, field, NULL) < 0)
		    continue;
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
    G_done_msg(n_("%d feature written.",
                  "%d features written.",
                  count), count);

    exit(EXIT_SUCCESS);
}
