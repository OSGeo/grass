
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

	/** add other options for index parameters here */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return calculateIndex(conf->answer, patch_density, NULL, raster->answer,
			  output->answer);

}


int patch_density(int fd, char **par, struct area_entry *ad, double *result)
{
    CELL *buf, *sup, *cnull;
    CELL pid, old_pid, *pid_curr, *pid_sup, *ctmp;
    int count, i, j, k, connected, other_above;
    struct Cell_head hd;
    int mask_fd, *mask_buf, null_count;
    double area;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;

    Rast_get_cellhd(ad->raster, "", &hd);

    cnull = Rast_allocate_c_buf();
    Rast_set_c_null_value(cnull, Rast_window_cols());
    sup = cnull;

    /* initialize patch ids */
    pid_curr = Rast_allocate_c_buf();
    Rast_set_c_null_value(pid_curr, Rast_window_cols());
    pid_sup = Rast_allocate_c_buf();
    Rast_set_c_null_value(pid_sup, Rast_window_cols());

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = NULL;
    null_count = 0;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return 0;
	mask_buf = G_malloc(ad->cl * sizeof(int));
    }

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

    /* calculate number of patches */
    count = 0;
    connected = 0;
    other_above = 0;
    pid = 0;

    for (i = 0; i < ad->rl; i++) {
	buf = RLI_get_cell_raster_row(fd, i + ad->y, ad);
	if (i > 0) {
	    sup = RLI_get_cell_raster_row(fd, i - 1 + ad->y, ad);
	}
	/* mask values */
	if (ad->mask == 1) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	    for (k = 0; k < ad->cl; k++) {
		if (mask_buf[k] == 0) {
		    Rast_set_c_null_value(&buf[k + ad->x], 1);
		    null_count++;
		}
	    }
	}
	
	ctmp = pid_sup;
	pid_sup = pid_curr;
	pid_curr = ctmp;
	Rast_set_c_null_value(pid_curr, Rast_window_cols());

	connected = 0;
	other_above = 1;
	for (j = 0; j < ad->cl; j++) {
	    
	    if (Rast_is_null_value(&(buf[j + ad->x]), CELL_TYPE)) {
		connected = 0;
		other_above = 1;
		continue;
	    }

	    if (sup[j + ad->x] == buf[j + ad->x]) {
		
		if (!connected) {
		    pid_curr[j + ad->x] = pid_sup[j + ad->x];
		}
		
		if (pid_curr[j + ad->x] != pid_sup[j + ad->x]) {
		    if (connected) {
			count--;
		    }
		    if (other_above) {
			pid_curr[j + ad->x] = pid_sup[j + ad->x];
			for (k = j + ad->x - 1; k >= ad->x; k--) {
			    if (buf[k] != buf[j + ad->x])
				break;
			    pid_curr[k] = pid_sup[j + ad->x];
			}
		    }
		    else {
			old_pid = pid_sup[j + ad->x];
			pid_sup[j + ad->x] = pid_curr[j + ad->x];
			
			for (k = j + 1; k < ad->cl; k++) {
			    if (pid_sup[k + ad->x] == old_pid) {
				pid_sup[k + ad->x] = pid_curr[j + ad->x];
			    }
			}
		    }
		}

		other_above = 0;
		connected = 1;
	    }

	    if (!connected) {
		count++;
		pid++;
		pid_curr[j + ad->x] = pid;
	    }

	    if (j < ad->cl - 1) {
		if (buf[j + ad->x] == buf[j + 1 + ad->x]) {
		    
		    connected = 1;
		    pid_curr[j + 1 + ad->x] = pid_curr[j + ad->x];
		}
		else {
		    other_above = 1;
		    connected = 0;
		}
	    }
	}
    }

    area = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	(((NS_DIST1 + NS_DIST2) / 2) / hd.rows) *
	(ad->rl *ad->cl - null_count);

    if (area != 0)
	*result = (count / area) * 1000000;
    else
	Rast_set_d_null_value(result, 1);

    if (ad->mask == 1) {
	G_free(mask_buf);
    }

    G_free(cnull);
    G_free(pid_curr);
    G_free(pid_sup);

    return RLI_OK;
}
