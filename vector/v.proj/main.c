
/****************************************************************************
 *
 * MODULE:       v.proj
 * AUTHOR(S):    Irina Kosinovsky, US ARMY CERL,
 *               M.L. Holko, USDA, SCS, NHQ-CGIS,
 *               R.L. Glenn, USDA, SCS, NHQ-CGIS (original contributors
 *               Update to GRASS 6: Radim Blazek <radim.blazek gmail.com> 
 *               Huidae Cho <grass4u gmail.com>, Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Markus Neteler <neteler itc.it>,
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>
 *               Markus Metz
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2008, 2018 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int i, type, stat;
    int day, yr, Out_proj;
    int out_zone = 0;
    int overwrite;		/* overwrite output map */
    const char *mapset;
    const char *omap_name, *map_name, *iset_name, *iloc_name;
    struct pj_info info_in;
    struct pj_info info_out;
    struct pj_info info_trans;
    const char *gbase;
    char date[40], mon[4];
    struct GModule *module;
    struct Option *omapopt, *mapopt, *isetopt, *ilocopt, *ibaseopt, *smax;
#ifdef HAVE_PROJ_H
    struct Option *pipeline;	/* name of custom PROJ pipeline */
#endif
    struct Key_Value *in_proj_keys, *in_unit_keys;
    struct Key_Value *out_proj_keys, *out_unit_keys;
    struct line_pnts *Points, *Points2;
    struct line_cats *Cats;
    struct Map_info Map;
    struct Map_info Out_Map;
    struct bound_box src_box, tgt_box;
    int nowrap = 0, recommend_nowrap = 0;
    double lmax;
    struct
    {
	struct Flag *list;	/* list files in source location */
	struct Flag *transformz;	/* treat z as ellipsoidal height */
	struct Flag *wrap;		/* latlon output: wrap to 0,360 */
	struct Flag *no_topol;		/* do not build topology */
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("import"));
    module->description = _("Re-projects a vector map from one location to the current location.");

    /* set up the options and flags for the command line parser */

    ilocopt = G_define_standard_option(G_OPT_M_LOCATION);
    ilocopt->required = YES;
    ilocopt->label = _("Location containing input vector map");
    ilocopt->guisection = _("Source");
    
    isetopt = G_define_standard_option(G_OPT_M_MAPSET);
    isetopt->label = _("Mapset containing input vector map");
    isetopt->description = _("Default: name of current mapset");
    isetopt->guisection = _("Source");

    mapopt = G_define_standard_option(G_OPT_V_INPUT);
    mapopt->required = NO;
    mapopt->label = _("Name of input vector map to re-project");
    mapopt->description = NULL;
    mapopt->guisection = _("Source");
    
    ibaseopt = G_define_standard_option(G_OPT_M_DBASE);
    ibaseopt->label = _("Path to GRASS database of input location");
    
    smax = G_define_option();
    smax->key = "smax";
    smax->type = TYPE_DOUBLE;
    smax->required = NO;
    smax->answer = "10000";
    smax->label = _("Maximum segment length in meters in output vector map");
    smax->description = _("Increases accuracy of reprojected shapes, disable with smax=0");
    smax->guisection = _("Target");

    omapopt = G_define_standard_option(G_OPT_V_OUTPUT);
    omapopt->required = NO;
    omapopt->description = _("Name for output vector map (default: input)");
    omapopt->guisection = _("Target");

#ifdef HAVE_PROJ_H
    pipeline = G_define_option();
    pipeline->key = "pipeline";
    pipeline->type = TYPE_STRING;
    pipeline->required = NO;
    pipeline->description = _("PROJ pipeline for coordinate transformation");
#endif

    flag.list = G_define_flag();
    flag.list->key = 'l';
    flag.list->description = _("List vector maps in input mapset and exit");

    flag.transformz = G_define_flag();
    flag.transformz->key = 'z';
    flag.transformz->description = _("3D vector maps only");
    flag.transformz->label =
	_("Assume z coordinate is ellipsoidal height and "
	  "transform if possible");
    flag.transformz->guisection = _("Target");

    flag.wrap = G_define_flag();
    flag.wrap->key = 'w';
    flag.wrap->description = _("Latlon output only, default is -180,180");
    flag.wrap->label =
	_("Disable wrapping to -180,180 for latlon output");
    flag.wrap->guisection = _("Target");

    flag.no_topol = G_define_flag();
    flag.no_topol->key = 'b';
    flag.no_topol->label = _("Do not build vector topology");
    flag.no_topol->description = _("Recommended for massive point projection");

    /* The parser checks if the map already exists in current mapset,
       we switch out the check and do it
       in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* start checking options and flags */
    /* set input vector map name and mapset */
    map_name = mapopt->answer;
    if (omapopt->answer)
	omap_name = omapopt->answer;
    else
	omap_name = map_name;
    if (omap_name && !flag.list->answer && !overwrite &&
	G_find_vector2(omap_name, G_mapset()))
	G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the --overwrite flag"), omapopt->key,
		      omap_name);
    if (isetopt->answer)
	iset_name = isetopt->answer;
    else
	iset_name = G_store(G_mapset());

    iloc_name = ilocopt->answer;

    if (ibaseopt->answer)
	gbase = ibaseopt->answer;
    else
	gbase = G_store(G_gisdbase());

    if (!ibaseopt->answer && strcmp(iloc_name, G_location()) == 0)
	G_fatal_error(_("Input and output locations can not be the same"));

    lmax = atof(smax->answer);
    if (lmax < 0)
	lmax = 0;

    Out_proj = G_projection();
    if (Out_proj == PROJECTION_LL && flag.wrap->answer)
	nowrap = 1;
    
    G_begin_distance_calculations();

    /* Change the location here and then come back */

    select_target_env();
    G_setenv_nogisrc("GISDBASE", gbase);
    G_setenv_nogisrc("LOCATION_NAME", iloc_name);
    G_setenv_nogisrc("MAPSET", iset_name);
    stat = G_mapset_permissions(iset_name);
    
    if (stat >= 0) {		/* yes, we can access the mapset */
	/* if requested, list the vector maps in source location - MN 5/2001 */
	if (flag.list->answer) {
	    char **list;
	    G_verbose_message(_("Checking location <%s> mapset <%s>"),
			      iloc_name, iset_name);
	    list = G_list(G_ELEMENT_VECTOR, G_getenv_nofatal("GISDBASE"),
			  G_getenv_nofatal("LOCATION_NAME"), iset_name);
	    if (list[0]) {
		for (i = 0; list[i]; i++) {
		    fprintf(stdout, "%s\n", list[i]);
		}
		fflush(stdout);
	    }
	    else {
		G_important_message(_("No vector maps found"));
	    }
	    exit(EXIT_SUCCESS);	/* leave v.proj after listing */
	}

	if (mapopt->answer == NULL) {
	    G_fatal_error(_("Required parameter <%s> not set"), mapopt->key);
	}

	G_setenv_nogisrc("MAPSET", iset_name);
	/* Make sure map is available */
	mapset = G_find_vector2(map_name, iset_name);
	if (mapset == NULL)
	    G_fatal_error(_("Vector map <%s> in location <%s> mapset <%s> not found"),
			  map_name, iloc_name, iset_name);

	 /*** Get projection info for input mapset ***/
	in_proj_keys = G_get_projinfo();
	if (in_proj_keys == NULL)
	    exit(EXIT_FAILURE);

	/* apparently the +over switch must be set in the input projection,
	 * not the output latlon projection */
	if (Out_proj == PROJECTION_LL && nowrap == 1)
	    G_set_key_value("+over", "defined", in_proj_keys);

	in_unit_keys = G_get_projunits();
	if (in_unit_keys == NULL)
	    exit(EXIT_FAILURE);

	if (pj_get_kv(&info_in, in_proj_keys, in_unit_keys) < 0)
	    exit(EXIT_FAILURE);

	Vect_set_open_level(1);
	G_debug(1, "Open old: location: %s mapset : %s", G_location_path(),
		G_mapset());
	if (Vect_open_old(&Map, map_name, mapset) < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"), map_name);
    }
    else if (stat < 0)
    {				/* allow 0 (i.e. denied permission) */
	/* need to be able to read from others */
	if (stat == 0)
	    G_fatal_error(_("Mapset <%s> in input location <%s> - permission denied"),
			  iset_name, iloc_name);
	else
	    G_fatal_error(_("Mapset <%s> in input location <%s> not found"),
			  iset_name, iloc_name);
    }

    select_current_env();

    /****** get the output projection parameters ******/
    out_proj_keys = G_get_projinfo();
    if (out_proj_keys == NULL)
	exit(EXIT_FAILURE);

    out_unit_keys = G_get_projunits();
    if (out_unit_keys == NULL)
	exit(EXIT_FAILURE);

    if (pj_get_kv(&info_out, out_proj_keys, out_unit_keys) < 0)
	exit(EXIT_FAILURE);

    G_free_key_value(in_proj_keys);
    G_free_key_value(in_unit_keys);
    G_free_key_value(out_proj_keys);
    G_free_key_value(out_unit_keys);

    if (G_verbose() == G_verbose_max()) {
	pj_print_proj_params(&info_in, &info_out);
    }

    info_trans.def = NULL;
#ifdef HAVE_PROJ_H
    if (pipeline->answer) {
	info_trans.def = G_store(pipeline->answer);
    }
#endif
    if (GPJ_init_transform(&info_in, &info_out, &info_trans) < 0)
	G_fatal_error(_("Unable to initialize coordinate transformation"));

    /* Initialize the Point / Cat structure */
    Points = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* test if latlon wrapping to -180,180 should be disabled */
    if (Out_proj == PROJECTION_LL && nowrap == 0) {
	int first = 1, counter = 0;
	double x, y;
	
	/* Cycle through all lines */
	Vect_rewind(&Map);
	while (1) {
	    type = Vect_read_next_line(&Map, Points, Cats);	/* read line */
	    if (type == 0)
		continue;		/* Dead */

	    if (type == -1)
		G_fatal_error(_("Reading input vector map"));
	    if (type == -2)
		break;
		
	    if (first && Points->n_points > 0) {
		first = 0;
		src_box.E = src_box.W = Points->x[0];
		src_box.N = src_box.S = Points->y[0];
		src_box.T = src_box.B = Points->z[0];
	    }
	    for (i = 0; i < Points->n_points; i++) {
		if (src_box.E < Points->x[i])
		    src_box.E = Points->x[i];
		if (src_box.W > Points->x[i])
		    src_box.W = Points->x[i];
		if (src_box.N < Points->y[i])
		    src_box.N = Points->y[i];
		if (src_box.S > Points->y[i])
		    src_box.S = Points->y[i];
	    }
	    counter++;
	}
	if (counter == 0) {
	    G_warning(_("Input vector map <%s> is empty"), omap_name);
	    exit(EXIT_SUCCESS);
	}
	/* NW corner */
	x = src_box.W;
	y = src_box.N;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			  &x, &y, NULL) < 0)
	    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
			   "GPJ_transform()");

	tgt_box.E = x;
	tgt_box.W = x;
	tgt_box.N = y;
	tgt_box.S = y;
	/* SW corner */
	x = src_box.W;
	y = src_box.S;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			  &x, &y, NULL) < 0)
	    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
			   "GPJ_transform()");
	if (tgt_box.W > x)
	    tgt_box.W = x;
	if (tgt_box.E < x)
	    tgt_box.E = x;
	if (tgt_box.N < y)
	    tgt_box.N = y;
	if (tgt_box.S > y)
	    tgt_box.S = y;
	/* NE corner */
	x = src_box.E;
	y = src_box.N;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			  &x, &y, NULL) < 0)
	    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
			   "GPJ_transform()");
	if (tgt_box.W > x) {
	    tgt_box.E = x + 360;
	    recommend_nowrap = 1;
	}
	if (tgt_box.N < y)
	    tgt_box.N = y;
	if (tgt_box.S > y)
	    tgt_box.S = y;
	/* SE corner */
	x = src_box.E;
	y = src_box.S;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			  &x, &y, NULL) < 0)
	    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
			   "GPJ_transform()");
	if (tgt_box.W > x) {
	    if (tgt_box.E < x + 360)
		tgt_box.E = x + 360;
	    recommend_nowrap = 1;
	}
	if (tgt_box.N < y)
	    tgt_box.N = y;
	if (tgt_box.S > y)
	    tgt_box.S = y;
    }

    G_debug(1, "Open new: location: %s mapset : %s", G_location_path(),
	    G_mapset());

    if (Vect_open_new(&Out_Map, omap_name, Vect_is_3d(&Map)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), omap_name);

    Vect_set_error_handler_io(NULL, &Out_Map); /* register standard i/o error handler */
    
    Vect_copy_head_data(&Map, &Out_Map);
    Vect_hist_copy(&Map, &Out_Map);
    Vect_hist_command(&Out_Map);

    out_zone = info_out.zone;
    Vect_set_zone(&Out_Map, out_zone);

    /* Read and write header info */
    sprintf(date, "%s", G_date());
    sscanf(date, "%*s%s%d%*s%d", mon, &day, &yr);
    if (yr < 2000)
	yr = yr - 1900;
    else
	yr = yr - 2000;
    sprintf(date, "%s %d %d", mon, day, yr);
    Vect_set_date(&Out_Map, date);

    /* line densification works only with vector topology */
    if (Map.format != GV_FORMAT_NATIVE)
	lmax = 0;

    /* Cycle through all lines */
    Vect_rewind(&Map);
    i = 0;
    G_message(_("Reprojecting primitives ..."));
    while (TRUE) {
	++i;
	G_progress(i, 1e3);
	type = Vect_read_next_line(&Map, Points, Cats);	/* read line */
	if (type == 0)
	    continue;		/* Dead */

	if (type == -1)
	    G_fatal_error(_("Reading input vector map"));
	if (type == -2)
	    break;

	Vect_line_prune(Points);
	if (lmax > 0 && (type & GV_LINES) && Points->n_points > 1) {
	    double x1, y1, z1, x2, y2, z2;
	    double dx, dy, dz;
	    double l;
	    int n;

	    Vect_reset_line(Points2);
	    for (i = 0; i < Points->n_points - 1; i++) {
		x1 = Points->x[i];
		y1 = Points->y[i];
		z1 = Points->z[i];
		n = i + 1;
		x2 = Points->x[n];
		y2 = Points->y[n];
		z2 = Points->z[n];

		dx = x2 - x1;
		dy = y2 - y1;
		dz = z2 - z1;

		if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
				  &x1, &y1, flag.transformz->answer ? &z1 : NULL) < 0)
		    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
				   "GPJ_transform()");

		if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
				  &x2, &y2, flag.transformz->answer ? &z2 : NULL) < 0)
		    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
				   "GPJ_transform()");

		Vect_append_point(Points2, x1, y1, z1);

		l = G_distance(x1, y1, x2, y2);

		if (l > lmax) {
		    int j;
		    double x, y, z;

		    x1 = Points->x[i];
		    y1 = Points->y[i];
		    z1 = Points->z[i];

		    n = ceil(l / lmax);

		    for (j = 1; j < n; j++) {
			x = x1 + dx * j / n;
			y = y1 + dy * j / n;
			z = z1 + dz * j / n;

			if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
					  &x, &y, flag.transformz->answer ? &z : NULL) < 0)
			  G_fatal_error(_("Unable to re-project vector map <%s> from <%s>"),
					Vect_get_full_name(&Map), ilocopt->answer);
			Vect_append_point(Points2, x, y, z);
		    }
		}
	    }
	    Vect_append_point(Points2, x2, y2, z2);
	    Vect_write_line(&Out_Map, type, Points2, Cats);	/* write line */
	}
	else {
	    if (GPJ_transform_array(&info_in, &info_out, &info_trans, PJ_FWD,
			            Points->x, Points->y,
			            flag.transformz->answer ? Points->z : NULL,
			            Points->n_points) < 0) {
	      G_fatal_error(_("Unable to re-project vector map <%s> from <%s>"),
			    Vect_get_full_name(&Map), ilocopt->answer);
	    }

	    Vect_write_line(&Out_Map, type, Points, Cats);	/* write line */
	}
    }				/* end lines section */
    G_progress(1, 1);

    /* Copy tables */
    if (Vect_copy_tables(&Map, &Out_Map, 0))
        G_warning(_("Failed to copy attribute table to output map"));

    Vect_close(&Map);

    if (!flag.no_topol->answer)
        Vect_build(&Out_Map);
    Vect_close(&Out_Map);

    if (recommend_nowrap)
	G_important_message(_("Try to disable wrapping to -180,180 "
			      "if topological errors occurred"));

    exit(EXIT_SUCCESS);
}
