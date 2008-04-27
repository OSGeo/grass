/****************************************************************************
 *
 * MODULE:       r.cats
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *		 Hamish Bowman, Dunedin, New Zealand  (label creation opts)
 *
 * PURPOSE:      Prints category values and labels associated with
 *		 user-specified raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

static char fs = '\t';
static struct Categories cats;


int main(int argc, char *argv[])
{
    char *name;
    char *mapset;
    long x, y;
    double dx;
    RASTER_MAP_TYPE map_type;
    int i;
    int from_stdin = FALSE;
    struct GModule *module;

    struct
    {
	struct Option *map, *fs, *cats, *vals, *raster, *file, *fmt_str, *fmt_coeff;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Manages category values and labels associated "
	  "with user-specified raster map layers.");

    parm.map = G_define_standard_option(G_OPT_R_MAP);

    parm.cats = G_define_standard_option(G_OPT_V_CATS);
    parm.cats->multiple = YES;

    parm.vals = G_define_option();
    parm.vals->key         = "vals";
    parm.vals->type        = TYPE_DOUBLE;
    parm.vals->multiple    = YES;
    parm.vals->required    = NO;
    parm.vals->label       = _("Comma separated value list");
    parm.vals->description = _("Example: 1.4,3.8,13");

    parm.fs = G_define_standard_option(G_OPT_F_SEP);
    parm.fs->key_desc = "character|space|tab";
    parm.fs->answer   = "tab";
    parm.fs->description = _("Output field separator");

    parm.raster = G_define_standard_option(G_OPT_R_INPUT);
    parm.raster->key         = "raster";
    parm.raster->required    = NO;
    parm.raster->description = _("Raster map from which to copy category table");

    parm.file = G_define_standard_option(G_OPT_F_INPUT);
    parm.file->key         = "rules";
    parm.file->required    = NO;
    parm.file->description =
	_("File containing category label rules (or \"-\" to read from stdin)");

    parm.fmt_str = G_define_option();
    parm.fmt_str->key         = "format";
    parm.fmt_str->type        = TYPE_STRING;
    parm.fmt_str->required    = NO;
    parm.fmt_str->label       = _("Default label or format string for dynamic labeling");
    parm.fmt_str->description = _("Used when no explicit label exists for the category");

    parm.fmt_coeff = G_define_option();
    parm.fmt_coeff->key      = "coefficients";
    parm.fmt_coeff->type     = TYPE_DOUBLE;
    parm.fmt_coeff->required = NO;
    parm.fmt_coeff->key_desc = "mult1,offset1,mult2,offset2";
/*    parm.fmt_coeff->answer   = "0.0,0.0,0.0,0.0"; */
    parm.fmt_coeff->label = _("Dynamic label coefficients");
    parm.fmt_coeff->description =
	_("Two pairs of category multiplier and offsets, for $1 and $2");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    name = parm.map->answer;

    /* see v.in.ascii for a better solution */
    if (parm.fs->answer != NULL) {
	if (strcmp(parm.fs->answer, "space") == 0)
	    fs = ' ';
	else if (strcmp(parm.fs->answer, "tab") == 0)
	    fs = '\t';
	else if (strcmp(parm.fs->answer, "\\t") == 0)
	    fs = '\t';
	else
	    fs = parm.fs->answer[0];
    }

    mapset = G_find_cell2(name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    map_type = G_raster_map_type(name, mapset);


    /* create category labels */
    if (parm.raster->answer || parm.file->answer ||
	parm.fmt_str->answer || parm.fmt_coeff->answer) {

	/* restrict editing to current mapset */
	if( strcmp(mapset, G_mapset()) != 0 )
	    G_fatal_error(_("Raster map <%s> not found in current mapset"), name);

	/* use cats from another map */
	if(parm.raster->answer) {
	    int fd;
	    char *cmapset;

	    cmapset = G_find_cell2(parm.raster->answer, "");
	    if (cmapset == NULL)
		G_fatal_error(_("Raster map <%s> not found"), parm.raster->answer);

	    if((fd = G_open_cell_old(name, mapset)) < 0)
		G_fatal_error(_("Unable to open raster map <%s>"), name);

	    G_init_raster_cats("", &cats);

	    if (0 > G_read_cats(parm.raster->answer, cmapset, &cats))
		G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		    parm.raster->answer, cmapset);

	    if (G_write_cats(name, &cats) >= 0)
		G_message(_("Category table for <%s> set from <%s>"), name,
		    parm.raster->answer);

	    G_close_cell(fd);
	}

	/* load cats from rules file */
/*  TODO: respect fs= */
	if(parm.file->answer) {
	    FILE *fp;

	    if (strcmp ("-", parm.file->answer) == 0) {
		from_stdin = TRUE;
		fp = stdin;
	    }
	    else {
		fp = fopen(parm.file->answer, "r");
		if (!fp)
		    G_fatal_error(_("Unable to open file <%s>"), parm.file->answer);
	    }

	    G_init_raster_cats("", &cats);

	    for (;;) {
		char buf[1024], label[1024];
		DCELL d1, d2;

		if (!G_getl2(buf, sizeof(buf), fp))
			break;

		if (sscanf(buf, "%lf:%lf:%[^\n]", &d1, &d2, label) == 3)
			G_set_d_raster_cat(&d1, &d2, label, &cats);
		else if (sscanf(buf, "%lf:%[^\n]", &d1, label) == 2)
			G_set_d_raster_cat(&d1, &d1, label, &cats);
	    }

	    if (G_write_cats(name, &cats) < 0)
		G_fatal_error(_("Cannot create category file for <%s>"), name);

	    if(!from_stdin)
		fclose(fp);
	}

	/* set dynamic cat rules for cats without explicit labels */
	if(parm.fmt_str->answer || parm.fmt_coeff->answer) {
	    char *fmt_str;
	    double m1,a1,m2,a2;

	    /* read existing values */
	    G_init_raster_cats("", &cats);

	    if (0 > G_read_cats(name, G_mapset(), &cats))
		G_warning(_("Unable to read category file of raster map <%s@%s>"), name, G_mapset());

	    if(parm.fmt_str->answer) {
		fmt_str = G_malloc(strlen(parm.fmt_str->answer) > strlen(cats.fmt)
			    ? strlen(parm.fmt_str->answer)+1 : strlen(cats.fmt)+1 );
		strcpy(fmt_str, parm.fmt_str->answer);
	    }
	    else {
	   	fmt_str = G_malloc(strlen(cats.fmt)+1);
		strcpy(fmt_str, cats.fmt);
	    }
 
	    m1 = cats.m1;
	    a1 = cats.a1;
	    m2 = cats.m2;
	    a2 = cats.a2;

	    if(parm.fmt_coeff->answer) {
		m1 = atof(parm.fmt_coeff->answers[0]);
		a1 = atof(parm.fmt_coeff->answers[1]);
		m2 = atof(parm.fmt_coeff->answers[2]);
		a2 = atof(parm.fmt_coeff->answers[3]);
	    }

	    G_set_cats_fmt(fmt_str, m1, a1, m2, a2, &cats);

	    if(G_write_cats(name, &cats) != 1)
		G_fatal_error(_("Cannot create category file for <%s>"), name);
	}

	G_free_cats(&cats);
	exit(EXIT_SUCCESS);
    }
    else {
	if (G_read_cats(name, mapset, &cats) < 0) 	 
	    G_fatal_error(_("Unable to read category file of raster map <%s> in <%s>"),
			     name, mapset);
    }

    /* describe the category labels */
    /* if no cats requested, use r.describe to get the cats */
    if (parm.cats->answer == NULL) {
	if (map_type == CELL_TYPE) {
	    get_cats(name, mapset);
	    while (next_cat(&x))
		print_label(x);
	    exit(EXIT_SUCCESS);
	}
    }
    else {
	if (map_type != CELL_TYPE)
	    G_warning(_("The map is floating point! Ignoring cats list, using vals list"));
	else {	/* integer map */

	    for (i = 0; parm.cats->answers[i]; i++)
		if (!scan_cats(parm.cats->answers[i], &x, &y)) {
		    G_usage();
		    exit(EXIT_FAILURE);
		}
	    for (i = 0; parm.cats->answers[i]; i++) {
		scan_cats(parm.cats->answers[i], &x, &y);
		while (x <= y)
		    print_label(x++);
	    }
	    exit(EXIT_SUCCESS);
	}
    }
    if (parm.vals->answer == NULL)
	G_fatal_error(_("vals argument is required for floating point map!"));
    for (i = 0; parm.vals->answers[i]; i++)
	if (!scan_vals(parm.vals->answers[i], &dx)) {
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    for (i = 0; parm.vals->answers[i]; i++) {
	scan_vals(parm.vals->answers[i], &dx);
	print_d_label(dx);
    }
    exit(EXIT_SUCCESS);
}



int print_label(long x)
{
    char *label;

    G_squeeze(label = G_get_cat((CELL) x, &cats));
    fprintf(stdout, "%ld%c%s\n", x, fs, label);

    return 0;
}

int print_d_label(double x)
{
    char *label, tmp[40];
    DCELL dtmp;

    dtmp = x;
    G_squeeze(label = G_get_d_raster_cat(&dtmp, &cats));
    sprintf(tmp, "%.10f", x);
    G_trim_decimal(tmp);
    fprintf(stdout, "%s%c%s\n", tmp, fs, label);

    return 0;
}

int scan_cats(char *s, long *x, long *y)
{
    char dummy[2];

    *dummy = 0;
    if (sscanf(s, "%ld-%ld%1s", x, y, dummy) == 2)
	return (*dummy == 0 && *x <= *y);
    *dummy = 0;
    if (sscanf(s, "%ld%1s", x, dummy) == 1 && *dummy == 0) {
	*y = *x;
	return 1;
    }
    return 0;
}

int scan_vals(char *s, double *x)
{
    char dummy[10];
    *dummy = 0;
    if (sscanf(s, "%lf%1s", x, dummy) == 1 && *dummy == 0)
	return 1;
    return 0;
}
