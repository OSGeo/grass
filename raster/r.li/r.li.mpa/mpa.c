/****************************************************************************
 *
 * MODULE:       r.li.mpa
 * AUTHOR(S):    Serena Pallecchi (original contributor)
 *                student of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Rewrite: Markus Metz
 *
 * PURPOSE:      calculates mean pixel attribute index
 * COPYRIGHT:    (C) 2007-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>


#include "../r.li.daemon/defs.h"
#include "../r.li.daemon/daemon.h"

rli_func meanPixelAttribute;
int calculate(int fd, struct area_entry *ad, double *result);
int calculateD(int fd, struct area_entry *ad, double *result);
int calculateF(int fd, struct area_entry *ad, double *result);

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates mean pixel attribute index on a raster map");
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


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return calculateIndex(conf->answer, meanPixelAttribute, NULL,
			  raster->answer, output->answer);
}


int meanPixelAttribute(int fd, char **par, struct area_entry *ad, double *result)
{
    int ris = 0;
    double indice = 0;

    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    ris = calculate(fd, ad, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    ris = calculateD(fd, ad, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    ris = calculateF(fd, ad, &indice);
	    break;
	}
    default:
	{
	    G_fatal_error("data type unknown");
	    return RLI_ERRORE;
	}

    }
    if (ris != RLI_OK) {
	return RLI_ERRORE;
    }

    *result = indice;

    return RLI_OK;
}


int calculate(int fd, struct area_entry *ad, double *result)
{
    CELL *buf;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int masked = FALSE;

    double area = 0;
    double somma = 0;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't open mask");
	    return RLI_ERRORE;
	}
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_cell_raster_row(fd, j + ad->y, ad);

	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	for (i = 0; i < ad->cl; i++) {
	    if (masked && mask_buf[i] == 0) {
		Rast_set_c_null_value(&buf[i + ad->x], 1);
	    }
	    if (!(Rast_is_c_null_value(&buf[i + ad->x]))) {
		area++;
		somma = somma + buf[i + ad->x];
	    }
	}
    }

    if (area > 0)
	*result = somma / area;
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}


int calculateD(int fd, struct area_entry *ad, double *result)
{
    DCELL *buf;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int masked = FALSE;

    double area = 0;
    double somma = 0;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't open mask");
	    return RLI_ERRORE;
	}
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_dcell_raster_row(fd, j + ad->y, ad);

	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	for (i = 0; i < ad->cl; i++) {
	    if (masked && mask_buf[i] == 0) {
		Rast_set_d_null_value(&buf[i + ad->x], 1);
	    }
	    if (!(Rast_is_d_null_value(&buf[i + ad->x]))) {
		area++;
		somma = somma + buf[i + ad->x];
	    }
	}
    }

    if (area > 0)
	*result = somma / area;
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}

int calculateF(int fd, struct area_entry *ad, double *result)
{
    FCELL *buf;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int masked = FALSE;

    double area = 0;
    double somma = 0;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't open mask");
	    return RLI_ERRORE;
	}
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_fcell_raster_row(fd, j + ad->y, ad);

	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	for (i = 0; i < ad->cl; i++) {
	    if (masked && mask_buf[i] == 0) {
		Rast_set_f_null_value(&buf[i + ad->x], 1);
	    }
	    if (!(Rast_is_f_null_value(&buf[i + ad->x]))) {
		area++;
		somma = somma + buf[i + ad->x];
	    }
	}
    }

    if (area > 0)
	*result = somma / area;
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}
