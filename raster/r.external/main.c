
/****************************************************************************
 *
 * MODULE:       r.external
 *               
 * AUTHOR(S):    Glynn Clements, based on r.in.gdal
 *               List GDAL layers by Martin Landa <landa.martin gmail.com> 8/2011
 *
 * PURPOSE:      Link raster map into GRASS utilizing the GDAL library.
 *
 * COPYRIGHT:    (C) 2008, 2010-2011 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include <gdal.h>
#include <ogr_srs_api.h>

#include "proto.h"

int main(int argc, char *argv[])
{
    const char *input, *source, *output;
    char *title;
    struct Cell_head cellhd;
    GDALDatasetH hDS;
    GDALRasterBandH hBand;
    struct GModule *module;
    struct {
	struct Option *input, *source, *output, *band, *title;
    } parm;
    struct {
	struct Flag *o, *f, *e, *h, *v;
    } flag;
    int min_band, max_band, band;
    struct band_info info;
    int flip;
    struct Ref reference;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("external"));
    module->description =
	_("Links GDAL supported raster data as a pseudo GRASS raster map.");

    parm.input = G_define_standard_option(G_OPT_F_INPUT);
    parm.input->description = _("Name of raster file to be linked");
    parm.input->required = NO;
    parm.input->guisection = _("Input");

    parm.source = G_define_option();
    parm.source->key = "source";
    parm.source->description = _("Name of non-file GDAL data source");
    parm.source->required = NO;
    parm.source->type = TYPE_STRING;
    parm.source->key_desc = "name";
    parm.source->guisection = _("Input");
    
    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    
    parm.band = G_define_option();
    parm.band->key = "band";
    parm.band->type = TYPE_INTEGER;
    parm.band->required = NO;
    parm.band->description = _("Band to select (default: all)");
    parm.band->guisection = _("Input");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");
    parm.title->guisection = _("Metadata");

    flag.f = G_define_flag();
    flag.f->key = 'f';
    flag.f->description = _("List supported formats and exit");
    flag.f->guisection = _("Print");
    flag.f->suppress_required = YES;

    flag.o = G_define_flag();
    flag.o->key = 'o';
    flag.o->description =
	_("Override projection (use location's projection)");

    flag.e = G_define_flag();
    flag.e->key = 'e';
    flag.e->label = _("Extend region extents based on new dataset");
    flag.e->description = _("Also updates the default region if in the PERMANENT mapset");

    flag.h = G_define_flag();
    flag.h->key = 'h';
    flag.h->description = _("Flip horizontally");

    flag.v = G_define_flag();
    flag.v->key = 'v';
    flag.v->description = _("Flip vertically");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    GDALAllRegister();

    if (flag.f->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    input = parm.input->answer;
    source = parm.source->answer;
    output = parm.output->answer;

    flip = 0;
    if (flag.h->answer)
	flip |= FLIP_H;
    if (flag.v->answer)
	flip |= FLIP_V;

    if (parm.title->answer) {
	title = G_store(parm.title->answer);
	G_strip(title);
    }
    else
	title = NULL;

    if (!input && !source)
	G_fatal_error(_("One of options <%s> or <%s> must be given"),
		      parm.input->key, parm.source->key);

    if (input && source)
	G_fatal_error(_("Option <%s> and <%s> are mutually exclusive"),
		      parm.input->key, parm.source->key);
    
    if (input && !G_is_absolute_path(input)) {
	char path[GPATH_MAX];
	getcwd(path, sizeof(path));
	strcat(path, "/");
	strcat(path, input);
	input = G_store(path);
    }

    if (!input)
	input = source;

    hDS = GDALOpen(input, GA_ReadOnly);
    if (hDS == NULL)
	return 1;

    setup_window(&cellhd, hDS, &flip);

    check_projection(&cellhd, hDS, flag.o->answer);

    Rast_set_window(&cellhd);

    if (parm.band->answer)
	min_band = max_band = atoi(parm.band->answer);
    else
	min_band = 1, max_band = GDALGetRasterCount(hDS);

    G_verbose_message(_("Proceeding with import..."));

    if (max_band > min_band) {
	if (I_find_group(output) == 1)
	    G_warning(_("Imagery group <%s> already exists and will be overwritten."), output);
	I_init_group_ref(&reference);
    }

    for (band = min_band; band <= max_band; band++) {
	char *output2, *title2 = NULL;

	G_message(_("Reading band %d of %d..."),
		  band, GDALGetRasterCount( hDS ));

	hBand = GDALGetRasterBand(hDS, band);
	if (!hBand)
	    G_fatal_error(_("Selected band (%d) does not exist"), band);

	if (max_band > min_band) {
	    G_asprintf(&output2, "%s.%d", output, band);
	    if (title)
		G_asprintf(&title2, "%s (band %d)", title, band);
	    G_debug(1, "Adding raster map <%s> to group <%s>", output2, output);
	    I_add_file_to_group_ref(output2, G_mapset(), &reference);
	}
	else {
	    output2 = G_store(output);
	    if (title)
		title2 = G_store(title);
	}

	query_band(hBand, output2, &cellhd, &info);
	create_map(input, band, output2, &cellhd, &info, title, flip);

	G_free(output2);
	G_free(title2);
    }

    /* close the GDALDataset to avoid segfault in libgdal */
    GDALClose(hDS);

    if (flag.e->answer)
	update_default_window(&cellhd);

    /* Create the imagery group if multiple bands are imported */
    if (max_band > min_band) {
    	I_put_group_ref(output, &reference);
	I_put_group(output);
	G_message(_("Imagery group <%s> created"), output);
    }

    exit(EXIT_SUCCESS);
}
