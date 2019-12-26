
/****************************************************************************
 *
 * MODULE:       r.buildvrt
 *               
 * AUTHOR(S):    Markus Metz, based on r.external
 *
 * PURPOSE:      Build a VRT (Virtual Raster) that is a mosaic of the
 *               list of input raster maps.
 *
 * COPYRIGHT:    (C) 2018 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "proto.h"

int cmp_wnd(const void *a, const void *b)
{
    struct Cell_head *cellhda = &((struct input *) a)->cellhd;
    struct Cell_head *cellhdb = &((struct input *) b)->cellhd;

    /* sort from descending N to S, then ascending from W to E */
    if (cellhda->south > cellhdb->south)
	return -1;
    if (cellhda->south < cellhdb->south)
	return 1;
    if (cellhda->north > cellhdb->north)
	return -1;
    if (cellhda->north < cellhdb->north)
	return 1;
    if (cellhda->west < cellhdb->west)
	return -1;
    if (cellhda->west > cellhdb->west)
	return 1;
    if (cellhda->east < cellhdb->east)
	return -1;
    if (cellhda->east > cellhdb->east)
	return 1;

    return 0;
}

int main(int argc, char *argv[])
{
    const char *output;
    char *title;
    struct Cell_head cellhd;
    struct GModule *module;
    struct {
	struct Option *input, *file, *output, *title;
    } parm;
    int i, j, num_inputs;
    struct input *inputs = NULL;
    char nsresstr[1024], ewresstr[1024];
    int maptype;
    struct FPRange fprange;
    DCELL dmin, dmax;
    int have_stats;
    struct R_stats rstats, ostats;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("mosaic"));
    G_add_keyword(_("virtual raster"));
    module->description =
	_("Build a VRT (Virtual Raster) from the list of input raster maps.");

    parm.input = G_define_standard_option(G_OPT_R_INPUTS);
    parm.input->description = _("Name of input raster files");
    parm.input->required = NO;
    parm.input->guisection = _("Input");

    parm.file = G_define_standard_option(G_OPT_F_INPUT);
    parm.file->key = "file";
    parm.file->description = _("Input file with one raster map name per line");
    parm.file->required = NO;
    parm.file->guisection = _("Input");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->guisection = _("Output");
    
    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");
    parm.title->guisection = _("Output");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (parm.input->answer && parm.file->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.input->key, parm.file->key);
 
    if (!parm.input->answer && !parm.file->answer)
        G_fatal_error(_("Please specify %s= or %s="),
			parm.input->key, parm.file->key);

    /* read input maps from file */
    if (parm.file->answer) {
	FILE *in;
	int max_inputs;

	if (strcmp(parm.file->answer, "-") == 0)
	    in = stdin;
	else {
	    in = fopen(parm.file->answer, "r");
	    if (!in)
		G_fatal_error(_("Unable to open input file <%s>"), parm.file->answer);
	}
    
	num_inputs = 0;
	max_inputs = 0;

	for (;;) {
	    char buf[GNAME_MAX];
	    char *name;
	    const char *mapset;
	    struct input *p;

	    if (!G_getl2(buf, sizeof(buf), in))
		break;

	    /* Ignore empty lines */
	    if (!*buf)
		continue;

	    name = buf;
	    if ((mapset = G_find_raster(name, "")) == NULL)
		G_fatal_error(_("Input raster map <%s> not found"), name);

	    if (strcmp(name, parm.output->answer) == 0)
		G_fatal_error(_("Input and output raster map can not be identical"));

	    Rast_read_fp_range(name, mapset, &fprange);
	    dmin = fprange.min;
	    if (Rast_is_d_null_value(&dmin)) {
		G_verbose_message(_("Input map <%s@%s> is all NULL, skipping"),
		                  name, mapset);
		continue;
	    }

	    if (num_inputs >= max_inputs) {
		max_inputs += 100;
		inputs = G_realloc(inputs, max_inputs * sizeof(struct input));
	    }
	    p = &inputs[num_inputs++];

	    p->name = G_store(name);
            p->mapset = G_store(mapset);
	    p->maptype = Rast_map_type(p->name, p->mapset);
	    Rast_get_cellhd(p->name, p->mapset, &(p->cellhd));
	}
	fclose(in);

	if (num_inputs < 1)
	    G_fatal_error(_("No raster map name found in input file"));
        
	if (num_inputs == 1)
	    G_fatal_error(_("Only one raster map name found in input file"));
    }
    else {
    	for (i = 0; parm.input->answers[i]; i++)
	    ;
    	num_inputs = i;

    	if (num_inputs < 1)
	    G_fatal_error(_("Raster map not found"));

	if (num_inputs == 1)
	    G_fatal_error(_("Only one raster map name found"));

    	inputs = G_malloc(num_inputs * sizeof(struct input));

	j = 0;
    	for (i = 0; i < num_inputs; i++) {
	    char *name;
	    const char *mapset;
	    struct input *p = &inputs[i];

	    name = parm.input->answers[i];
	    if ((mapset = G_find_raster(name, "")) == NULL)
		G_fatal_error(_("Input raster map <%s> not found"), name);

	    if (strcmp(name, parm.output->answer) == 0)
		G_fatal_error(_("Input and output raster map can not be identical"));

	    Rast_read_fp_range(name, mapset, &fprange);
	    dmin = fprange.min;
	    if (Rast_is_d_null_value(&dmin)) {
		G_verbose_message(_("Input map <%s@%s> is all NULL, skipping"),
		                  name, mapset);
		continue;
	    }

	    p = &inputs[j++];

	    p->name = G_store(name);
            p->mapset = G_store(mapset);
	    p->maptype = Rast_map_type(p->name, p->mapset);
	    Rast_get_cellhd(p->name, p->mapset, &(p->cellhd));
    	}
	num_inputs = j;
    }

    qsort(inputs, num_inputs, sizeof(struct input), cmp_wnd);

    /* check resolution and maptype of input maps */
    cellhd = inputs[0].cellhd;
    cellhd.compressed = 0;
    G_format_resolution(cellhd.ns_res, nsresstr, G_projection());
    G_format_resolution(cellhd.ew_res, ewresstr, G_projection());
    maptype = inputs[0].maptype;

    Rast_set_d_null_value(&dmin, 1);
    Rast_set_d_null_value(&dmax, 1);
    if (Rast_read_fp_range(inputs[0].name, inputs[0].mapset, &fprange) == 1) {
	dmin = fprange.min;
	dmax = fprange.max;
    }
    Rast_set_d_null_value(&(ostats.sum), 1);
    Rast_set_d_null_value(&(ostats.sumsq), 1);
    ostats.count = 0;
    have_stats = 1;
    if (Rast_read_rstats(inputs[0].name, inputs[0].mapset, &rstats) == 1) {
	ostats.sum = rstats.sum;
	ostats.sumsq = rstats.sumsq;
	ostats.count = rstats.count;
    }
    else
	have_stats = 0;

    for (i = 1; i < num_inputs; i++) {
	char tnsresstr[1024], tewresstr[1024];
	int tmaptype;
	struct input *p = &inputs[i];

	G_format_resolution(p->cellhd.ns_res, tnsresstr, G_projection());
	G_format_resolution(p->cellhd.ew_res, tewresstr, G_projection());
	tmaptype = p->maptype;

	if (tmaptype != maptype)
	    G_warning(_("Input maptypes are different"));
	if (strcmp(nsresstr, tnsresstr) != 0)
	    G_warning(_("Input ns resolutions are different"));
	if (strcmp(ewresstr, tewresstr) != 0)
	    G_warning(_("Input ns resolutions are different"));

	if (cellhd.north < p->cellhd.north)
	    cellhd.north = p->cellhd.north;
	if (cellhd.south > p->cellhd.south)
	    cellhd.south = p->cellhd.south;
	if (cellhd.east < p->cellhd.east)
	    cellhd.east = p->cellhd.east;
	if (cellhd.west > p->cellhd.west)
	    cellhd.west = p->cellhd.west;

	if (Rast_read_fp_range(p->name, p->mapset, &fprange) == 1) {
	    if (Rast_is_d_null_value(&dmin)) {
		dmin = fprange.min;
		dmax = fprange.max;
	    }
	    else {
		if (dmin > fprange.min)
		    dmin = fprange.min;
		if (dmax < fprange.max)
		    dmax = fprange.max;
	    }
	}
	if (have_stats && 
	    Rast_read_rstats(p->name, p->mapset, &rstats) == 1) {
	    ostats.sum += rstats.sum;
	    ostats.sumsq += rstats.sumsq;
	    ostats.count += rstats.count;
	}
	else
	    have_stats = 0;
    }

    G_adjust_Cell_head(&cellhd, 0, 0);

    if (maptype == CELL_TYPE)
	cellhd.format = 3;
    else
	cellhd.format = -1;

    output = parm.output->answer;

    title = NULL;
    if (parm.title->answer) {
	title = G_store(parm.title->answer);
	G_strip(title);
    }

    create_map(inputs, num_inputs, output, &cellhd, maptype, dmin, dmax,
               have_stats, &ostats, title);

    exit(EXIT_SUCCESS);
}
