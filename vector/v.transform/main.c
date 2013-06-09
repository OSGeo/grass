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

int main(int argc, char *argv[])
{
    struct file_info Current, Trans;

    struct GModule *module;

    struct Option *vold, *vnew, *xshift, *yshift, *zshift,
	*xscale, *yscale, *zscale, *zrot, *columns, *field_opt;
    struct Flag *tozero_flag, *swap_flag, *no_topo;

    char mon[4], date[40], buf[1000];
    struct Map_info Old, New;
    int day, yr;
    struct bound_box box;

    double ztozero;
    double trans_params[7];	/* xshift, ..., xscale, ..., zrot */

    /* columns */
    unsigned int i;
    int idx, out3d;
    char **tokens;
    char *columns_name[7];	/* xshift, yshift, zshift, xscale, yscale, zscale, zrot */
    int field;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("transformation"));
    G_add_keyword("GCP");
    module->description =
	_("Performs an affine transformation (shift, scale and rotate) "
	  "on vector map.");
    
    tozero_flag = G_define_flag();
    tozero_flag->key = 't';
    tozero_flag->description = _("Shift all z values to bottom=0");

    swap_flag = G_define_flag();
    swap_flag->key = 'w';
    swap_flag->description =
	_("Swap coordinates x, y and then apply other parameters");
    
    no_topo = G_define_flag();
    no_topo->key = 'b';
    no_topo->description = _("Do not build topology for output");

    vold = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->guisection = _("Custom");

    vnew = G_define_standard_option(G_OPT_V_OUTPUT);

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

    Vect_check_input_output_name(vold->answer, vnew->answer, G_FATAL_EXIT);

    /* open input vector */
    Vect_open_old2(&Old, vold->answer, "", field_opt->answer);

    field = Vect_get_field_number(&Old, field_opt->answer);
    if (field < 1 && columns->answer) {
	G_fatal_error(_("Columns require a valid layer. Please use '%s' parameter."),
		      field_opt->key);
    }

    out3d = Vect_is_3d(&Old);

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
		else {
		    G_warning(_("Unknown column parameter '%s'"),
		              tokens[0]);
		    idx = -1;
		}

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

    /* should output be 3D ? 
     * note that z-scale and ztozero have no effect with input 2D */
    if (trans_params[IDX_ZSHIFT] != 0. || columns_name[IDX_ZSHIFT])
	out3d = WITH_Z;

    /* open output vector */
    Vect_open_new(&New, vnew->answer, out3d);

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

    Vect_get_map_box(&Old, &box);

    /* z to zero */
    if (tozero_flag->answer)
	ztozero = 0 - box.B;
    else
	ztozero = 0;

    /* do the transformation */
    G_important_message(_("Tranforming features..."));
    transform_digit_file(&Old, &New,
			 ztozero, swap_flag->answer, trans_params,
			 columns_name, field);

    G_important_message(_("Copying attributes..."));
    if (Vect_copy_tables(&Old, &New, 0))
        G_warning(_("Failed to copy attribute table to output map"));
    Vect_close(&Old);
    if (!no_topo->answer)
	Vect_build(&New);

    Vect_get_map_box(&New, &box);
    G_verbose_message(_("New vector map <%s> boundary coordinates:"),
		      vnew->answer);
    G_verbose_message(_(" N: %-10.3f    S: %-10.3f"), box.N, box.S);
    G_verbose_message(_(" E: %-10.3f    W: %-10.3f"), box.E, box.W);
    G_verbose_message(_(" B: %6.3f    T: %6.3f"), box.B, box.T);

    Vect_close(&New);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
