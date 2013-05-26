
/****************************************************************************
 *
 * MODULE:       i.landsat.acca
 *
 * AUTHOR(S):    E. Jorge Tizado - ej.tizado@unileon.es
 *
 * PURPOSE:      Landsat TM/ETM+ Automatic Cloud Cover Assessment
 *               Adopted for GRASS 7 by Martin Landa <landa.martin gmail.com>
 *
 * COPYRIGHT:    (C) 2008, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

extern int hist_n;

/*----------------------------------------------*
 * Constant threshold of ACCA algorithm
 *----------------------------------------------*/

extern double th_1;
extern double th_1_b;
extern double th_2[];
extern double th_2_b;
extern double th_3;
extern double th_4;
extern double th_4_b;
extern double th_5;
extern double th_6;
extern double th_7;
extern double th_8;

/*----------------------------------------------*
 *
 * Check a raster name y return fd of open file
 *
 *----------------------------------------------*/
int check_raster(char *raster_name)
{
    RASTER_MAP_TYPE map_type;
    int raster_fd;
    
    if ((raster_fd = Rast_open_old(raster_name, "")) < 0) {
	G_fatal_error(_("Unable to open raster map <%s>"), raster_name);
    }
    /* Uncomment to work in full raster map
       if (G_get_cellhd(raster_name, mapset, &cellhd) < 0) {
       G_warning(_("Unable to read header of raster map <%s>"), raster_name);
       return -1;
       }
       if (G_set_window(&cellhd) < 0) {
       G_warning(_("Cannot reset current region"));
       return -1;
       }
     */
    if ((map_type = Rast_get_map_type(raster_fd)) != DCELL_TYPE) {
	G_fatal_error(_("Input raster map <%s> is not floating point "
			"(process DN using i.landsat.toar to radiance first)"), raster_name);
    }

    return raster_fd;
}

/*----------------------------------------------*
 *
 *      MAIN FUNCTION
 *
 *----------------------------------------------*/
int main(int argc, char *argv[])
{
    struct History history;
    struct GModule *module;

    int i;
    struct Option *band_prefix, *output, *hist, *b56c, *b45r;
    struct Flag *shadow, *filter, *sat5, *pass2, *csig;
    char *in_name, *out_name;
    struct Categories cats;

    CELL cell_shadow = IS_SHADOW, cell_cold_cloud = IS_COLD_CLOUD, cell_warm_cloud = IS_WARM_CLOUD;
    Gfile band[5], out;

    char title[1024];
    
    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    module->description =
	_("Performs Landsat TM/ETM+ Automatic Cloud Cover Assessment (ACCA).");
    G_add_keyword(_("imagery"));
    G_add_keyword(_("Landsat"));
    G_add_keyword("ACCA");
    
    band_prefix = G_define_option();
    band_prefix->key = "input_prefix";
    band_prefix->label = _("Base name of input raster bands");
    band_prefix->description = _("Example: 'B.' for B.1, B.2, ...");
    band_prefix->type = TYPE_STRING;
    band_prefix->required = YES;
    
    output = G_define_standard_option(G_OPT_R_OUTPUT);

    b56c = G_define_option();
    b56c->key = "b56composite";
    b56c->type = TYPE_DOUBLE;
    b56c->required = NO;
    b56c->description = _("B56composite (step 6)");
    b56c->answer = "225.";

    b45r = G_define_option();
    b45r->key = "b45ratio";
    b45r->type = TYPE_DOUBLE;
    b45r->required = NO;
    b45r->description = _("B45ratio: Desert detection (step 10)");
    b45r->answer = "1.";

    hist = G_define_option();
    hist->key = "histogram";
    hist->type = TYPE_INTEGER;
    hist->required = NO;
    hist->description =
	_("Number of classes in the cloud temperature histogram");
    hist->answer = "100";
    hist->guisection = _("Cloud settings");
    
    sat5 = G_define_flag();
    sat5->key = '5';
    sat5->label = _("Data is Landsat-5 TM");
    sat5->description = _("I.e. Thermal band is '.6' not '.61')");

    filter = G_define_flag();
    filter->key = 'f';
    filter->description =
	_("Apply post-processing filter to remove small holes");

    csig = G_define_flag();
    csig->key = 'x';
    csig->description = _("Always use cloud signature (step 14)");
    csig->guisection = _("Cloud settings");

    pass2 = G_define_flag();
    pass2->key = '2';
    pass2->description =
	_("Bypass second-pass processing, and merge warm (not ambiguous) and cold clouds");
    pass2->guisection = _("Cloud settings");

    shadow = G_define_flag();
    shadow->key = 's';
    shadow->description = _("Include a category for cloud shadows");
    shadow->guisection = _("Cloud settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* stores OPTIONS and FLAGS to variables */

    hist_n = atoi(hist->answer);
    if (hist_n < 10)
	hist_n = 10;

    in_name = band_prefix->answer;

    for (i = BAND2; i <= BAND6; i++) {
	sprintf(band[i].name, "%s%d%c", in_name, i + 2,
		 (i == BAND6 && !sat5->answer ? '1' : '\0'));
	band[i].fd = check_raster(band[i].name);
	band[i].rast = Rast_allocate_buf(DCELL_TYPE);
    }

    out_name = output->answer;

    sprintf(out.name, "%s", out_name);
    if (G_legal_filename(out_name) < 0)
	G_fatal_error(_("<%s> is an illegal file name"), out.name);

    /* --------------------------------------- */
    th_4 = atof(b56c->answer);
    th_7 = atof(b45r->answer);
    acca_algorithm(&out, band, pass2->answer, shadow->answer,
		   csig->answer);

    if (filter->answer)
	filter_holes(&out);
    /* --------------------------------------- */

    for (i = BAND2; i <= BAND6; i++) {
	G_free(band[i].rast);
	Rast_close(band[i].fd);
    }

    /* write out map title and category labels */
    Rast_init_cats("", &cats);
    sprintf(title, "LANDSAT-%s Automatic Cloud Cover Assessment",
	    sat5->answer ? "5 TM" : "7 ETM+");
    Rast_set_cats_title(title, &cats);

    Rast_set_c_cat(&cell_shadow, &cell_shadow,
		   "Shadow", &cats);
    Rast_set_c_cat(&cell_cold_cloud, &cell_cold_cloud,
		   "Cold cloud", &cats);
    Rast_set_c_cat(&cell_warm_cloud, &cell_warm_cloud,
		   "Warm cloud", &cats);

    Rast_write_cats(out.name, &cats);
    Rast_free_cats(&cats);

    /* write out command line opts */
    Rast_short_history(out.name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out.name, &history);

    exit(EXIT_SUCCESS);
}
