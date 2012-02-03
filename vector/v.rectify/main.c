
/****************************************************************************
 *
 * MODULE:       v.rectify
 * AUTHOR(S):    Markus Metz
 *               based on i.rectify
 * PURPOSE:      calculate a transformation matrix and then convert x,y(,z) 
 *               coordinates to standard map coordinates for all objects in 
 *               the vector
 *               control points can come from i.points or i.vpoints or 
 *               a user-given text file
 * COPYRIGHT:    (C) 2002-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "global.h"
#include "crs.h"


/* georef coefficients */

double E12[20], N12[20], Z12[20];
double E21[20], N21[20], Z21[20];


int main(int argc, char *argv[])
{
    char group[INAME_LEN];
    int order;			/* ADDED WITH CRS MODIFICATIONS */
    int n, i, nlines, type;
    int target_overwrite = 0;
    char *points_file, *overstr, *rms_sep;
    struct Map_info In, Out;
    struct line_pnts *Points, *OPoints;
    struct line_cats *Cats;
    double x, y, z;
    int use3d;

    struct Option *grp,         /* imagery group */
     *val,                      /* transformation order */
     *in_opt,			/* input vector name */
     *out_opt,			/* output vector name */
     *pfile,			/* text file with GCPs */
     *sep;			/* field separator for RMS report */

    struct Flag *flag_use3d, *no_topo, *print_rms;

    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("rectify"));
    module->description =
	_("Rectifies a vector by computing a coordinate "
	  "transformation for each object in the vector based on the "
	  "control points.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->required = YES;

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = YES;

    grp = G_define_standard_option(G_OPT_I_GROUP);
    grp->required = NO;

    pfile = G_define_standard_option(G_OPT_F_INPUT);
    pfile->key = "points";
    pfile->description = _("Name to input file with control points");
    pfile->required = NO;

    val = G_define_option();
    val->key = "order";
    val->type = TYPE_INTEGER;
    val->required = NO;
    val->options = "1-3";
    val->answer = "1";
    val->description = _("Rectification polynom order (1-3)");
    
    sep = G_define_standard_option(G_OPT_F_SEP);
    sep->label = _("Field separator for RMS report");
    sep->description = _("Special characters: newline, space, comma, tab");

    flag_use3d = G_define_flag();
    flag_use3d->key = '3';
    flag_use3d->description = _("Perform 3D transformation");

    print_rms = G_define_flag();
    print_rms->key = 'r';
    print_rms->label = _("Print RMS errors");
    print_rms->description = _("Print RMS errors and exit without rectifying the input map");

    no_topo = G_define_flag();
    no_topo->key = 'b';
    no_topo->description = _("Do not build topology for output");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (grp->answer) {
	G_strip(grp->answer);
	strcpy(group, grp->answer);
    }
    else
	group[0] = '\0';

    points_file = pfile->answer;

    if (grp->answer == NULL && points_file == NULL)
	G_fatal_error(_("Please select a group or give an input file."));
    else if (grp->answer != NULL && points_file != NULL)
	G_warning(_("Points in group will be ignored, GCPs in input file are used."));

    order = atoi(val->answer);

    if (order < 1 || order > MAXORDER)
	G_fatal_error(_("Invalid order (%d); please enter 1 to %d"), order,
		      MAXORDER);

    Vect_set_open_level(1);
    Vect_open_old2(&In, in_opt->answer, "", "");
    
    use3d = (Vect_is_3d(&In) && flag_use3d->answer);
    
    if (use3d && !points_file)
	G_fatal_error(_("A file with 3D control points is needed for 3d transformation"));

    if (print_rms->answer) {
	if (sep->answer) {
	    rms_sep = sep->answer;
	    if (strcmp(rms_sep, "\\t") == 0)
		rms_sep = "\t";
	    if (strcmp(rms_sep, "tab") == 0)
		rms_sep = "\t";
	    if (strcmp(rms_sep, "space") == 0)
		rms_sep = " ";
	    if (strcmp(rms_sep, "comma") == 0)
		rms_sep = ",";
	}
	else
	    rms_sep = "|";
    }
    else
	rms_sep = NULL;

    /* read the control points for the group */
    get_control_points(group, points_file, order, use3d,
                       print_rms->answer, rms_sep);
    
    if (print_rms->answer) {
	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    /* get the target */
    get_target(group);

    /* Check the GRASS_OVERWRITE environment variable */
    if ((overstr = getenv("GRASS_OVERWRITE")))  /* OK ? */
	target_overwrite = atoi(overstr);

    if (!target_overwrite) {
	/* check if output exists in target location/mapset */
	
	select_target_env();
		
	if (G_find_vector2(out_opt->answer, G_mapset())) {
	    G_warning(_("The vector map <%s> already exists in"), out_opt->answer);
	    G_warning(_("target LOCATION %s, MAPSET %s:"),
		      G_location(), G_mapset());
	    G_fatal_error(_("Rectification cancelled."));
	}
	select_current_env();
    }
    else
	G_debug(1, "Overwriting OK");

    select_target_env();
    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    select_current_env();
    
    Points = Vect_new_line_struct();
    OPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* count lines */
    nlines = 0;
    while (1) {
	type = Vect_read_next_line(&In, Points, Cats);
	if (type == 0)
	    continue;		/* Dead */

	if (type == -1)
	    G_fatal_error(_("Reading input vector map"));
	if (type == -2)
	    break;

	nlines++;
    }
	
    Vect_rewind(&In);

    i = 0;
    z = 0.0;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	G_percent(i++, nlines, 4);
	
	Vect_reset_line(OPoints);
	for (n = 0; n < Vect_get_num_line_points(Points); n++) {
	    if (use3d) {
		CRS_georef_3d(Points->x[n], Points->y[n], Points->z[n],
			      &x, &y, &z, E12, N12, Z12, order);
	    }
	    else {
		I_georef(Points->x[n], Points->y[n],
			      &x, &y, E12, N12, order);
		z = Points->z[n];
	    }
	    Vect_append_point(OPoints, x, y, z);
	}
	select_target_env();
	Vect_write_line(&Out, type, OPoints, Cats);
	select_current_env();
    }
    G_percent(1, 1, 1);
    
    select_target_env();
    if (!no_topo->answer)
	Vect_build(&Out);
    /* Copy tables */
    G_message(_("Copying attribute table(s)..."));
    if (Vect_copy_tables(&In, &Out, 0))
        G_warning(_("Failed to copy attribute table to output map"));
    Vect_close(&Out);
    
    select_current_env();

    Vect_close(&In);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
