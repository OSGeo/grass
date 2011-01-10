
/****************************************************************************
*
* MODULE:       v.transform
* AUTHOR(S):    See below also.
*               Eric G. Miller <egm2@jps.net>
*               Upgrade to 5.7 Radim Blazek
*               Column support & OGR support added by Martin Landa (09/2007)
*
* PURPOSE:      To transform a vector map's coordinates via a set of tie
*               points.
*
* COPYRIGHT:    (C) 2002-2011 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
/*
 *History:
 *- This takes an ascii digit file in one coordinate system and converts
 *  the map to another coordinate system.
 *  Uses the transform library:  $(TRANSLIB)
 *
 *  Written during the ice age of Illinois, 02/16/90, by the GRASS team, -mh.
 *
 *- Modified by Dave Gerdes  1/90  for dig_head stuff
 *- Modified by Radim Blazek to work on binary files 2002
 *- Interactive functionality disabled, 2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "trans.h"
#include "local_proto.h"

double ax[MAX_COOR];	/*  current map   */
double ay[MAX_COOR];

double bx[MAX_COOR];	/*  map we are going to   */
double by[MAX_COOR];

int use[MAX_COOR];	/*  where the coordinate came from */
double residuals[MAX_COOR];
double rms;

/*  this may be used in the future  */
int reg_cnt;		/*  count of registered points */

int main(int argc, char *argv[])
{
    struct file_info Current, Trans, Coord;

    struct GModule *module;

    struct Option *vold, *vnew, *pointsfile, *xshift, *yshift, *zshift,
	*xscale, *yscale, *zscale, *zrot, *columns, *table, *field;
    struct Flag *tozero_flag, *print_mat_flag, *swap_flag;

    char mon[4], date[40], buf[1000];
    struct Map_info Old, New;
    int day, yr;
    struct bound_box box;

    double ztozero;
    double trans_params[7];	// xshift, ..., xscale, ..., zrot

    /* columns */
    unsigned int i;
    int idx;
    char **tokens;
    char *columns_name[7];	/* xshift, yshift, zshift, xscale, yscale, zscale, zrot */

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("transformation"));
    module->description =
	_("Performs an affine transformation (shift, scale and rotate, "
	  "or GPCs) on vector map.");
    
    tozero_flag = G_define_flag();
    tozero_flag->key = 't';
    tozero_flag->description = _("Shift all z values to bottom=0");

    print_mat_flag = G_define_flag();
    print_mat_flag->key = 'm';
    print_mat_flag->description =
	_("Print the transformation matrix to stdout");
    
    swap_flag = G_define_flag();
    swap_flag->key = 's';
    swap_flag->description =
	_("Swap coordinates x, y and then apply other parameters");
    
    vold = G_define_standard_option(G_OPT_V_INPUT);

    field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field->guisection = _("Custom");

    vnew = G_define_standard_option(G_OPT_V_OUTPUT);

    pointsfile = G_define_standard_option(G_OPT_F_INPUT);
    pointsfile->key = "pointsfile";
    pointsfile->required = NO;
    pointsfile->label = _("ASCII file holding transform coordinates");
    pointsfile->description = _("If not given, transformation parameters "
				"(xshift, yshift, zshift, xscale, yscale, zscale, zrot) are used instead");

    xshift = G_define_option();
    xshift->key = "xshift";
    xshift->type = TYPE_DOUBLE;
    xshift->required = NO;
    xshift->multiple = NO;
    xshift->description = _("Shifting value for x coordinates");
    xshift->answer = "0.0";
    xshift->guisection = _("Custom");

    yshift = G_define_option();
    yshift->key = "yshift";
    yshift->type = TYPE_DOUBLE;
    yshift->required = NO;
    yshift->multiple = NO;
    yshift->description = _("Shifting value for y coordinates");
    yshift->answer = "0.0";
    yshift->guisection = _("Custom");

    zshift = G_define_option();
    zshift->key = "zshift";
    zshift->type = TYPE_DOUBLE;
    zshift->required = NO;
    zshift->multiple = NO;
    zshift->description = _("Shifting value for z coordinates");
    zshift->answer = "0.0";
    zshift->guisection = _("Custom");

    xscale = G_define_option();
    xscale->key = "xscale";
    xscale->type = TYPE_DOUBLE;
    xscale->required = NO;
    xscale->multiple = NO;
    xscale->description = _("Scaling factor for x coordinates");
    xscale->answer = "1.0";
    xscale->guisection = _("Custom");

    yscale = G_define_option();
    yscale->key = "yscale";
    yscale->type = TYPE_DOUBLE;
    yscale->required = NO;
    yscale->multiple = NO;
    yscale->description = _("Scaling factor for y coordinates");
    yscale->answer = "1.0";
    yscale->guisection = _("Custom");

    zscale = G_define_option();
    zscale->key = "zscale";
    zscale->type = TYPE_DOUBLE;
    zscale->required = NO;
    zscale->multiple = NO;
    zscale->description = _("Scaling factor for z coordinates");
    zscale->answer = "1.0";
    zscale->guisection = _("Custom");

    zrot = G_define_option();
    zrot->key = "zrot";
    zrot->type = TYPE_DOUBLE;
    zrot->required = NO;
    zrot->multiple = NO;
    zrot->description =
	_("Rotation around z axis in degrees counterclockwise");
    zrot->answer = "0.0";
    zrot->guisection = _("Custom");

    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->description =
	_("Name of table containing transformation parameters");
    table->guisection = _("Custom");

    columns = G_define_standard_option(G_OPT_DB_COLUMNS);
    columns->label =
	_("Name of attribute column(s) used as transformation parameters");
    columns->description =
	_("Format: parameter:column, e.g. xshift:xs,yshift:ys,zrot:zr");
    columns->guisection = _("Custom");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    strcpy(Current.name, vold->answer);
    strcpy(Trans.name, vnew->answer);

    Vect_check_input_output_name(vold->answer, vnew->answer, GV_FATAL_EXIT);

    if (!table->answer && columns->answer) {
	G_fatal_error(_("Table name is not defined. Please use '%s' parameter."),
		      table->key);
    }

    if (table->answer && strcmp(vnew->answer, table->answer) == 0) {
	G_fatal_error(_("Name of table and name for output vector map must be different. "
		       "Otherwise the table is overwritten."));
    }

    if (pointsfile->answer != NULL) {
	strcpy(Coord.name, pointsfile->answer);
    }
    else {
	Coord.name[0] = '\0';
    }

    /* open coord file */
    if (Coord.name[0] != '\0') {
	if ((Coord.fp = fopen(Coord.name, "r")) == NULL)
	    G_fatal_error(_("Unable to open file with coordinates <%s>"),
			  Coord.name);
    }

    /* open vector maps */
    Vect_open_old2(&Old, vold->answer, "", field->answer);

    Vect_open_new(&New, vnew->answer, Vect_is_3d(&Old) || zshift->answer);

    /* copy and set header */
    Vect_copy_head_data(&Old, &New);

    Vect_hist_copy(&Old, &New);
    Vect_hist_command(&New);

    sprintf(date, "%s", G_date());
    sscanf(date, "%*s%s%d%*s%d", mon, &day, &yr);
    sprintf(date, "%s %d %d", mon, day, yr);
    Vect_set_date(&New, date);

    Vect_set_person(&New, G_whoami());

    sprintf(buf, "transformed from %s", vold->answer);
    Vect_set_map_name(&New, buf);

    Vect_set_scale(&New, 1);
    Vect_set_zone(&New, 0);
    Vect_set_thresh(&New, 0.0);

    /* points file */
    if (Coord.name[0]) {
	create_transform_from_file(&Coord);

	if (Coord.name[0] != '\0')
	    fclose(Coord.fp);
    }

    Vect_get_map_box(&Old, &box);

    /* z to zero */
    if (tozero_flag->answer)
	ztozero = 0 - box.B;
    else
	ztozero = 0;

    /* tokenize columns names */
    for (i = 0; i <= IDX_ZROT; i++) {
	columns_name[i] = NULL;
    }
    i = 0;
    if (columns->answer) {
	while (columns->answers[i]) {
	    tokens = G_tokenize(columns->answers[i], ":");
	    if (G_number_of_tokens(tokens) == 2) {
		if (strcmp(tokens[0], xshift->key) == 0)
		    idx = IDX_XSHIFT;
		else if (strcmp(tokens[0], yshift->key) == 0)
		    idx = IDX_YSHIFT;
		else if (strcmp(tokens[0], zshift->key) == 0)
		    idx = IDX_ZSHIFT;
		else if (strcmp(tokens[0], xscale->key) == 0)
		    idx = IDX_XSCALE;
		else if (strcmp(tokens[0], yscale->key) == 0)
		    idx = IDX_YSCALE;
		else if (strcmp(tokens[0], zscale->key) == 0)
		    idx = IDX_ZSCALE;
		else if (strcmp(tokens[0], zrot->key) == 0)
		    idx = IDX_ZROT;
		else
		    idx = -1;

		if (idx != -1)
		    columns_name[idx] = G_store(tokens[1]);

		G_free_tokens(tokens);
	    }
	    else {
		G_fatal_error(_("Unable to tokenize column string: [%s]"),
			      columns->answers[i]);
	    }
	    i++;
	}
    }

    /* determine transformation parameters */
    trans_params[IDX_XSHIFT] = atof(xshift->answer);
    trans_params[IDX_YSHIFT] = atof(yshift->answer);
    trans_params[IDX_ZSHIFT] = atof(zshift->answer);
    trans_params[IDX_XSCALE] = atof(xscale->answer);
    trans_params[IDX_YSCALE] = atof(yscale->answer);
    trans_params[IDX_ZSCALE] = atof(zscale->answer);
    trans_params[IDX_ZROT] = atof(zrot->answer);

    transform_digit_file(&Old, &New, Coord.name[0] ? 1 : 0,
			 ztozero, swap_flag->answer, trans_params,
			 table->answer, columns_name, Vect_get_field_number(&Old, field->answer));

    if (Vect_copy_tables(&Old, &New, 0))
        G_warning(_("Failed to copy attribute table to output map"));
    Vect_close(&Old);
    Vect_build(&New);

    if (G_verbose() > G_verbose_std()) {
	Vect_get_map_box(&New, &box);
	G_message(_("\nNew vector map <%s> boundary coordinates:"),
		  vnew->answer);
	G_message(_(" N: %-10.3f    S: %-10.3f"), box.N, box.S);
	G_message(_(" E: %-10.3f    W: %-10.3f"), box.E, box.W);
	G_message(_(" B: %6.3f    T: %6.3f"), box.B, box.T);
    }

    /* print the transformation matrix if requested */
    if (print_mat_flag->answer)
      print_transform_matrix();
    
    Vect_close(&New);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
