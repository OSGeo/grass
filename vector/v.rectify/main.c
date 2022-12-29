
/****************************************************************************
 *
 * MODULE:       v.rectify
 * AUTHOR(S):    Markus Metz
 *               based on i.rectify
 * PURPOSE:      calculate a transformation matrix and then convert x,y(,z) 
 *               coordinates to standard map coordinates for all objects in 
 *               the vector
 *               control points can come from g.gui.gcp or 
 *               a user-given text file
 * COPYRIGHT:    (C) 2002-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "global.h"
#include "crs.h"


/* georef coefficients */

double E12[20], N12[20], Z12[20];
double E21[20], N21[20], Z21[20];
double HG12[20], HG21[20], HQ12[20], HQ21[20];
double OR12[20], OR21[20];


int main(int argc, char *argv[])
{
    char group[INAME_LEN];
    int order, orthorot;
    int n, i, nlines, type;
    int target_overwrite = 0;
    char *points_file, *overstr, *rms_sep;
    struct Map_info In, Out;
    struct line_pnts *Points, *OPoints;
    struct line_cats *Cats;
    double x, y, z;
    int use3d;
    FILE *fp;

    struct Option *grp,         /* imagery group */
     *val,                      /* transformation order */
     *in_opt,			/* input vector name */
     *out_opt,			/* output vector name */
     *pfile,			/* text file with GCPs */
     *rfile,			/* text file to hold RMS errors */
     *sep;			/* field separator for RMS report */

    struct Flag *flag_use3d, *no_topo, *print_rms, *ortho;

    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("rectify"));
    G_add_keyword(_("level1"));
    G_add_keyword(_("geometry"));

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
    pfile->description = _("Name of input file with control points");
    pfile->required = NO;

    rfile = G_define_standard_option(G_OPT_F_INPUT);
    rfile->key = "rmsfile";
    rfile->description = _("Name of output file with RMS errors (if omitted or '-' output to stdout");
    rfile->required = NO;

    val = G_define_option();
    val->key = "order";
    val->type = TYPE_INTEGER;
    val->required = NO;
    val->options = "1-3";
    val->answer = "1";
    val->description = _("Rectification polynomial order (1-3)");
    
    sep = G_define_standard_option(G_OPT_F_SEP);
    sep->label = _("Field separator for RMS report");

    flag_use3d = G_define_flag();
    flag_use3d->key = '3';
    flag_use3d->description = _("Perform 3D transformation");

    ortho = G_define_flag();
    ortho->key = 'o';
    ortho->description = _("Perform orthogonal 3D transformation");

    print_rms = G_define_flag();
    print_rms->key = 'r';
    print_rms->label = _("Print RMS errors");
    print_rms->description = _("Print RMS errors and exit without rectifying the input map");

    no_topo = G_define_standard_flag(G_FLG_V_TOPO);

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
    if (Vect_open_old2(&In, in_opt->answer, "", "") < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);
    
    use3d = (Vect_is_3d(&In) &&
             (flag_use3d->answer || ortho->answer));
    
    if (!use3d && (flag_use3d->answer || ortho->answer))
	G_fatal_error(_("3D transformation requires a 3D vector"));

    if (use3d && !points_file)
	G_fatal_error(_("A file with 3D control points is needed for 3D transformation"));
	
    orthorot = ortho->answer;

    if (print_rms->answer)
	rms_sep = G_option_to_separator(sep);
    else
	rms_sep = NULL;

    if (rfile->answer) {
	if (strcmp(rfile->answer, "-")) {
	    fp = fopen(rfile->answer, "w");
	    if (!fp)
		G_fatal_error(_("Unable to open file '%s' for writing"),
		              rfile->answer);
	}
	else
	    fp = stdout;
    }
    else
	fp = stdout;

    /* read the control points for the group */
    get_control_points(group, points_file, order, use3d, orthorot,
                       print_rms->answer, rms_sep, fp);
    
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

    if (Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);

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
		if (orthorot)
		    CRS_georef_or(Points->x[n], Points->y[n], Points->z[n],
				  &x, &y, &z, OR12);
		else
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
