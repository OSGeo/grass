
/****************************************************************************
 *
 * MODULE:       r.li.patchdensity
 * AUTHOR(S):    Claudio Porta and Lucio Davide Spano (original contributors)
 *                students of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Fixes: Serena Pallecchi, Markus Neteler <neteler itc.it>
 * PURPOSE:      calculates patch density index
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
#include "../r.li.daemon/defs.h"

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates patch density index on a raster map, using a 4 neighbour algorithm");
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

	/**add other options for index parameters here */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return calculateIndex(conf->answer, patch_density, NULL, raster->answer,
			  output->answer);

}


int patch_density(int fd, char **par, struct area_entry *ad, double *result)
{
    CELL *buf, *sup;
    int count = 0, i, j, connected = 0, complete_line = 1, other_above = 0;
    double area;
    struct Cell_head hd;
    CELL complete_value;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
    int mask_fd = -1, *mask_buf, *mask_sup, null_count = 0;

    G_debug(1, "begin patch_density() index");

    Rast_set_c_null_value(&complete_value, 1);
    Rast_get_cellhd(ad->raster, "", &hd);

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return 0;

	mask_buf = malloc(ad->cl * sizeof(int));
	mask_sup = malloc(ad->cl * sizeof(int));
    }

    sup = Rast_allocate_c_buf();

    /* calculate distance */
    G_begin_distance_calculations();
    /* EW Dist at North edge */
    EW_DIST1 = G_distance(hd.east, hd.north, hd.west, hd.north);
    /* EW Dist at South Edge */
    EW_DIST2 = G_distance(hd.east, hd.south, hd.west, hd.south);
    /* NS Dist at East edge */
    NS_DIST1 = G_distance(hd.east, hd.north, hd.east, hd.south);
    /* NS Dist at West edge */
    NS_DIST2 = G_distance(hd.west, hd.north, hd.west, hd.south);


    /* calculate number of patch */

    for (i = 0; i < ad->rl; i++) {
	buf = RLI_get_cell_raster_row(fd, i + ad->y, ad);

	if (i > 0) {
	    sup = RLI_get_cell_raster_row(fd, i - 1 + ad->y, ad);
	}

	/* mask values */
	if (ad->mask == 1) {
	    int k;

	    if (i > 0) {
		int *tmp;

		tmp = mask_sup;
		mask_buf = mask_sup;
	    }
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	    for (k = 0; k < ad->cl; k++) {
		if (mask_buf[k] == 0) {
		    Rast_set_c_null_value(mask_buf + k, 1);
		    null_count++;
		}
	    }

	}


	if (complete_line) {
	    if (!Rast_is_null_value(&(buf[ad->x]), CELL_TYPE) &&
		buf[ad->x] != complete_value)
		count++;

	    for (j = 0; j < ad->cl - 1; j++) {

		if (buf[j + ad->x] != buf[j + 1 + ad->x]) {
		    complete_line = 0;
		    if (!Rast_is_null_value(&(buf[j + 1 + ad->x]), CELL_TYPE)
			&& buf[j + 1 + ad->x] != complete_value)
			count++;
		}

	    }
	    if (complete_line) {
		complete_value = buf[ad->x];
	    }
	}
	else {
	    complete_line = 1;
	    connected = 0;
	    other_above = 0;
	    for (j = 0; j < ad->cl; j++) {
		if (sup[j + ad->x] == buf[j + ad->x]) {
		    connected = 1;
		    if (other_above) {
			other_above = 0;
			count--;
		    }
		}
		else {
		    if (connected &&
			!Rast_is_null_value(&(buf[j + ad->x]), CELL_TYPE))
			other_above = 1;
		}
		if (j < ad->cl - 1 && buf[j + ad->x] != buf[j + 1 + ad->x]) {
		    complete_line = 0;
		    if (!connected &&
			!Rast_is_null_value(&(buf[j + ad->x]), CELL_TYPE)) {

			count++;
			connected = 0;
			other_above = 0;
		    }
		    else {
			connected = 0;
			other_above = 0;
		    }
		}
	    }
	    if (!connected &&
		sup[ad->cl - 1 + ad->x] != buf[ad->cl - 1 + ad->x]) {
		if (!Rast_is_null_value
		    (&(buf[ad->cl - 1 + ad->x]), CELL_TYPE)) {
		    count++;
		    complete_line = 0;
		}
	    }

	    if (complete_line)
		complete_value = buf[ad->x];

	}

    }

    area = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	(((NS_DIST1 + NS_DIST2) / 2) / hd.rows) *
	(ad->rl *ad->cl - null_count);

    if (area != 0)
	*result = (count / area) * 1000000;
    else
	*result = -1;

    G_free(sup);

    return RLI_OK;
}
