
/****************************************************************
 *
 * MODULE:       v.external
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Create a new vector as a link to OGR layer (read-only)
 *               
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "ogr_api.h"

int main(int argc, char *argv[])
{
    int i;
    struct GModule *module;
    struct Option *dsn_opt, *layer_opt, *out_opt;
    char buf[2000];
    FILE *fd;
    struct Map_info Map;
    OGRDataSourceH Ogr_ds;
    OGRSFDriverH Ogr_driver;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    int nlayers;
    int layer;
    char *layer_name;

    G_gisinit(argv[0]);

    OGRRegisterAll();

    /* Module options */
    sprintf(buf, "Available drivers: ");
    for (i = 0; i < OGRGetDriverCount(); i++) {
	Ogr_driver = OGRGetDriver(i);
	if (i == 0)
	    sprintf(buf, "%s%s", buf, OGR_Dr_GetName(Ogr_driver));
	else
	    sprintf(buf, "%s,%s", buf, OGR_Dr_GetName(Ogr_driver));
    }
    module = G_define_module();
    module->keywords = _("vector");
    module->label =
	_("Creates a new vector as a read-only link to OGR layer.");
    module->description = G_store(buf);

    dsn_opt = G_define_option();
    dsn_opt->key = "dsn";
    dsn_opt->type = TYPE_STRING;
    dsn_opt->required = YES;
    dsn_opt->gisprompt = "old_file,file,dsn";
    dsn_opt->description = "OGR datasource name. Examples:\n"
	"\t\tESRI Shapefile: directory containing shapefiles\n"
	"\t\tMapInfo File: directory containing mapinfo files";

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = NO;
    out_opt->description =
	_("Output vector. If not given, available layers are printed only.");

    layer_opt = G_define_option();
    layer_opt->key = "layer";
    layer_opt->type = TYPE_STRING;
    layer_opt->required = NO;
    layer_opt->multiple = NO;
    layer_opt->description =
	_("OGR layer name. If not given, available layers are printed only. Examples:\n"
	 "\t\tESRI Shapefile: shapefile name\n"
	 "\t\tMapInfo File: mapinfo file name");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!out_opt->answer && layer_opt->answer)
	G_fatal_error(_("Output vector name was not specified"));

    /* Open OGR DSN */
    Ogr_ds = OGROpen(dsn_opt->answer, FALSE, NULL);
    if (Ogr_ds == NULL)
	G_fatal_error(_("Cannot open data source"));

    /* Make a list of available layers */
    nlayers = OGR_DS_GetLayerCount(Ogr_ds);

    if (!layer_opt->answer)
	fprintf(stdout, "Data source contains %d layers:\n", nlayers);

    layer = -1;
    for (i = 0; i < nlayers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	layer_name = (char *)OGR_FD_GetName(Ogr_featuredefn);

	if (!layer_opt->answer) {
	    if (i > 0)
		fprintf(stdout, ", ");
	    fprintf(stdout, "%s", layer_name);
	}
	else {
	    if (strcmp(layer_name, layer_opt->answer) == 0) {
		layer = i;
	    }
	}
    }

    if (!layer_opt->answer) {
	fprintf(stdout, "\n");
	exit(0);
    }

    if (layer == -1) {
	G_fatal_error(_("Layer <%s> not available"), layer_opt->answer);
    }

    G_debug(2, "layer '%s' was found", layer_opt->answer);

    OGR_DS_Destroy(Ogr_ds);

    Vect_open_new(&Map, out_opt->answer, 0);
    Vect_hist_command(&Map);
    Vect_close(&Map);

    /* Vect_open_new created 'head', 'coor', 'hist' -> delete 'coor' and create 'frmt' */
    sprintf(buf, "%s/%s/vector/%s/coor", G_location_path(), G_mapset(),
	    out_opt->answer);
    G_debug(2, "Delete '%s'", buf);
    if (unlink(buf) == -1) {
	G_fatal_error("Cannot delete '%s'", buf);
    }

    /* Create frmt */
    sprintf(buf, "%s/%s", GRASS_VECT_DIRECTORY, out_opt->answer);
    fd = G_fopen_new(buf, GRASS_VECT_FRMT_ELEMENT);
    if (fd == NULL) {
	G_fatal_error("Cannot open 'frmt' file.");
    }

    fprintf(fd, "FORMAT: ogr\n");
    fprintf(fd, "DSN: %s\n", dsn_opt->answer);
    fprintf(fd, "LAYER: %s\n", layer_opt->answer);

    fclose(fd);

    Vect_open_old(&Map, out_opt->answer, G_mapset());
    Vect_build(&Map);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
