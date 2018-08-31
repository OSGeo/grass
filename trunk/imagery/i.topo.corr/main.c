
/****************************************************************************
 *
 * MODULE:       i.topo.corr
 *
 * AUTHOR(S):    E. Jorge Tizado - ej.tizado@unileon.es
 *
 * PURPOSE:      Topographic corrections
 *
 * COPYRIGHT:    (C) 2002, 2005, 2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct History history;
    struct GModule *module;
    struct Cell_head hd_band, hd_dem, window;

    int i;
    struct Option *base, *output, *input, *zeni, *azim, *metho;
    struct Flag *ilum, *scl;

    Gfile dem, out, band;
    double zenith, azimuth;
    int method = COSINE;
    int do_scale;

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("terrain"));
    G_add_keyword(_("topographic correction"));
    module->description = _("Computes topographic correction of reflectance.");

    /* It defines the different parameters */

    input = G_define_standard_option(G_OPT_R_INPUTS);
    input->required = NO;
    input->multiple = YES;
    input->description =
	_("Name of reflectance raster maps to be corrected topographically");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description =
	_("Name (flag -i) or prefix for output raster maps");

    base = G_define_standard_option(G_OPT_R_MAP);
    base->key = "basemap";
    base->description = _("Name of input base raster map (elevation or illumination)");

    zeni = G_define_option();
    zeni->key = "zenith";
    zeni->type = TYPE_DOUBLE;
    zeni->required = YES;
    zeni->description = _("Solar zenith in degrees");

    azim = G_define_option();
    azim->key = "azimuth";
    azim->type = TYPE_DOUBLE;
    azim->required = NO;
    azim->description = _("Solar azimuth in degrees (only if flag -i)");

    metho = G_define_option();
    metho->key = "method";
    metho->type = TYPE_STRING;
    metho->required = NO;
    metho->options = "cosine,minnaert,c-factor,percent";
    metho->description = _("Topographic correction method");
    metho->answer = "c-factor";

    ilum = G_define_flag();
    ilum->key = 'i';
    ilum->description = _("Output sun illumination terrain model");
    
    scl = G_define_flag();
    scl->key = 's';
    scl->description = _("Scale output to input and copy color rules");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (ilum->answer && azim->answer == NULL)
	G_fatal_error(_("Solar azimuth is necessary to calculate illumination terrain model"));

    if (!ilum->answer && input->answer == NULL)
	G_fatal_error
	    (_("Reflectance maps are necessary to make topographic correction"));

    zenith = atof(zeni->answer);
    out.type = DCELL_TYPE;
    do_scale = scl->answer;

    /* Evaluate only cos_i raster file */
    /* i.topo.corr -i out=cosi.on07 base=SRTM_v2 zenith=33.3631 azimuth=59.8897 */
    if (ilum->answer) {
	Rast_get_window(&window);
	azimuth = atof(azim->answer);
	/* Warning: make buffers and output after set window */
	strcpy(dem.name, base->answer);
	/* Set window to DEM file */
	Rast_get_window(&window);
	Rast_get_cellhd(dem.name, "", &hd_dem);
	Rast_align_window(&window, &hd_dem);
	dem.fd = Rast_open_old(dem.name, "");
	dem.type = Rast_get_map_type(dem.fd);
	/* Open and buffer of the output file */
	strcpy(out.name, output->answer);
	out.fd = Rast_open_new(output->answer, DCELL_TYPE);
	out.rast = Rast_allocate_buf(out.type);
	/* Open and buffer of the elevation file */
	dem.rast = Rast_allocate_buf(dem.type);
	eval_cosi(&out, &dem, zenith, azimuth);
	/* Close files, buffers, and write history */
	G_free(dem.rast);
	Rast_close(dem.fd);
	G_free(out.rast);
	Rast_close(out.fd);
	Rast_short_history(out.name, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out.name, &history);
    }
    /* Evaluate topographic correction for all bands */
    /* i.topo.corr input=on07.toar.1 out=tcor base=cosi.on07 zenith=33.3631 method=c-factor */
    else {
	/*              if (G_strcasecmp(metho->answer, "cosine") == 0)        method = COSINE;
	 *               else if (G_strcasecmp(metho->answer, "percent") == 0)  method = PERCENT;
	 *               else if (G_strcasecmp(metho->answer, "minnaert") == 0) method = MINNAERT;
	 *               else if (G_strcasecmp(metho->answer, "c-factor") == 0) method = C_CORRECT;
	 *               else G_fatal_error(_("Invalid method: %s"), metho->answer);
	 */

	if (metho->answer[1] == 'o')
	    method = COSINE;
	else if (metho->answer[1] == 'e')
	    method = PERCENT;
	else if (metho->answer[1] == 'i')
	    method = MINNAERT;
	else if (metho->answer[1] == '-')
	    method = C_CORRECT;
	else
	    G_fatal_error(_("Invalid method: %s"), metho->answer);

	dem.fd = Rast_open_old(base->answer, "");
	dem.type = Rast_get_map_type(dem.fd);
	Rast_close(dem.fd);
	if (dem.type == CELL_TYPE)
	    G_fatal_error(_("Illumination model is of CELL type"));

	for (i = 0; input->answers[i] != NULL; i++) {
	    G_message(_("Band %s: "), input->answers[i]);
	    /* Abre fichero de bandas y el de salida */
	    strcpy(band.name, input->answers[i]);
	    Rast_get_cellhd(band.name, "", &hd_band);
	    Rast_set_window(&hd_band);	/* Antes de out_open y allocate para mismo size */
	    band.fd = Rast_open_old(band.name, "");
	    band.type = Rast_get_map_type(band.fd);
	    if (band.type != DCELL_TYPE) {
		G_warning(_("Reflectance of <%s> is not of DCELL type - ignored."),
			  input->answers[i]);
		Rast_close(band.fd);
		continue;
	    }
	    /* ----- */
	    dem.fd = Rast_open_old(base->answer, "");
	    G_snprintf(out.name, GNAME_MAX - 1, "%s.%s", output->answer,
                       input->answers[i]);
	    out.fd = Rast_open_new(out.name, DCELL_TYPE);
	    out.rast = Rast_allocate_buf(out.type);
	    band.rast = Rast_allocate_buf(band.type);
	    dem.rast = Rast_allocate_buf(dem.type);
	    /* ----- */
	    eval_tcor(method, &out, &dem, &band, zenith, do_scale);
	    /* ----- */
	    G_free(dem.rast);
	    Rast_close(dem.fd);
	    G_free(band.rast);
	    Rast_close(band.fd);
	    G_free(out.rast);
	    Rast_close(out.fd);
	    Rast_short_history(out.name, "raster", &history);
	    Rast_command_history(&history);
	    Rast_write_history(out.name, &history);

	    {
		struct FPRange range;
		DCELL min, max;
		struct Colors grey;
		int make_colors = 1;

		if (do_scale) {
		    if (Rast_read_colors(band.name, "", &grey) >= 0)
			make_colors = 0;
		}
		
		if (make_colors) {
		    Rast_read_fp_range(out.name, G_mapset(), &range);
		    Rast_get_fp_range_min_max(&range, &min, &max);
		    Rast_make_grey_scale_colors(&grey, min, max);
		}
		Rast_write_colors(out.name, G_mapset(), &grey);
	    }
	}
    }

    exit(EXIT_SUCCESS);
}
