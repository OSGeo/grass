
/****************************************************************************
 *
 * MODULE:       r.li.shape
 * AUTHOR(S):    Claudio Porta and Lucio Davide Spano (original contributors)
 *                students of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Fixes: Markus Neteler <neteler itc.it>
 *               
 * PURPOSE:      calculates shape index
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "../r.li.daemon/daemon.h"

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description = _("Calculates shape index on a raster map");
    G_add_keyword(_("raster"));
    G_add_keyword(_("landscape structure analysis"));
    G_add_keyword(_("patch index"));

    /* define options */

    raster = G_define_standard_option(G_OPT_R_INPUT);

    conf = G_define_standard_option(G_OPT_F_INPUT);
    conf->key = "config";
    conf->description = _("Configuration file");
    conf->required = YES;

    output = G_define_standard_option(G_OPT_R_OUTPUT);

	/** add other options for index parameters here */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return calculateIndex(conf->answer, shape_index, NULL, raster->answer,
			  output->answer);

}

int shape_index(int fd, char **par, struct area_entry *ad, double *result)
{


    double area;
    struct Cell_head hd;
    CELL complete_value;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
    int mask_fd = -1, null_count = 0;
    int i = 0, k = 0;
    int *mask_buf;

    Rast_set_c_null_value(&complete_value, 1);
    Rast_get_cellhd(ad->raster, "", &hd);

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return 0;
	mask_buf = malloc(ad->cl * sizeof(int));
	for (i = 0; i < ad->rl; i++) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	    for (k = 0; k < ad->cl; k++) {
		if (mask_buf[k] == 0) {
		    null_count++;
		}
	    }
	}
    }

    /*calculate distance */
    G_begin_distance_calculations();
    /* EW Dist at North edge */
    EW_DIST1 = G_distance(hd.east, hd.north, hd.west, hd.north);
    /* EW Dist at South Edge */
    EW_DIST2 = G_distance(hd.east, hd.south, hd.west, hd.south);
    /* NS Dist at East edge */
    NS_DIST1 = G_distance(hd.east, hd.north, hd.east, hd.south);
    /* NS Dist at West edge */
    NS_DIST2 = G_distance(hd.west, hd.north, hd.west, hd.south);


    area = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	(((NS_DIST1 + NS_DIST2) / 2) / hd.rows) *
	(ad->rl * ad->cl - null_count);

    *result = area;
    return 1;
}
