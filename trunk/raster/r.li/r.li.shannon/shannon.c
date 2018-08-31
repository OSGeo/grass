/****************************************************************************
 *
 * MODULE:       r.li.shannon
 * AUTHOR(S):    Serena Pallecchi (original contributor)
 *                student of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Rewrite: Markus Metz
 *
 * PURPOSE:      calculates Shannon's diversity index
 * COPYRIGHT:    (C) 2007-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "../r.li.daemon/daemon.h"
#include "../r.li.daemon/avlDefs.h"
#include "../r.li.daemon/avl.h"

/* template for dominance, renyi, pielou, simpson */

rli_func shannon;
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
	_("Calculates Shannon's diversity index on a raster map");
    G_add_keyword(_("raster"));
    G_add_keyword(_("landscape structure analysis"));
    G_add_keyword(_("diversity index"));

    /* define options */

    raster = G_define_standard_option(G_OPT_R_INPUT);

    conf = G_define_standard_option(G_OPT_F_INPUT);
    conf->key = "config";
    conf->description = _("Configuration file");
    conf->required = YES;

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return calculateIndex(conf->answer, shannon, NULL, raster->answer,
			  output->answer);
}


int shannon(int fd, char **par, struct area_entry *ad, double *result)
{
    int ris = RLI_OK;
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
    CELL corrCell;
    CELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int ris = 0;
    int masked = FALSE;

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 1;

    long area = 0;

    avl_tree albero = NULL;
    AVL_table array;
    generic_cell uc;

    uc.t = CELL_TYPE;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    Rast_set_c_null_value(&precCell, 1);
    for (j = 0; j < ad->rl; j++) {	/* for each row */
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	buf = RLI_get_cell_raster_row(fd, j + ad->y, ad);

	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i] == 0)) {
		Rast_set_c_null_value(&corrCell, 1);
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		/* total patch area */
		area++;
	    }

	    if (!(Rast_is_null_value(&precCell, uc.t)) &&
		corrCell == precCell) {

		totCorr++;
	    }
	    else if (!(Rast_is_null_value(&precCell, uc.t)) &&
		     corrCell != precCell) {
		
		/* add precCell to search tree */
		if (albero == NULL) {
		    uc.val.c = precCell;
		    albero = avl_make(uc, totCorr);
		    if (albero == NULL) {
			G_fatal_error("avl_make error");
			return RLI_ERRORE;
		    }
		    m++;
		}
		else {
		    uc.val.c = precCell;
		    ris = avl_add(&albero, uc, totCorr);
		    switch (ris) {
		    case AVL_ERR:
			{
			    G_fatal_error("avl_add error");
			    return RLI_ERRORE;
			}
		    case AVL_ADD:
			{
			    m++;
			    break;
			}
		    case AVL_PRES:
			{
			    break;
			}
		    default:
			{
			    G_fatal_error("avl_make unknown error");
			    return RLI_ERRORE;
			}
		    }
		}
		totCorr = 1;
	    }		/* endif not equal cells */
	    precCell = corrCell;
	}
    }

    /* last closing */
    if (area > 0 && !(Rast_is_null_value(&precCell, uc.t))) {
	if (albero == NULL) {
	    uc.val.c = precCell;
	    albero = avl_make(uc, totCorr);
	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    uc.val.c = precCell;
	    ris = avl_add(&albero, uc, totCorr);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avl_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    m++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avl_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}
    }

    if (area > 0) {
	double t;
	double shannon;
	double perc, logarithm;

	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);
	if (tot != m) {
	    G_warning("avl_to_array unexpected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate shannon */
	shannon = 0;
	for (i = 0; i < m; i++) {
	    t = array[i].tot;
	    perc = t / area;
	    logarithm = log(perc);
	    shannon += perc * logarithm;
	}
	G_free(array);

	*result = -shannon;
    }
    else
	Rast_set_d_null_value(result, 1);

    avl_destroy(albero);
    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}


int calculateD(int fd, struct area_entry *ad, double *result)
{
    DCELL *buf;
    DCELL corrCell;
    DCELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int ris = 0;
    int masked = FALSE;

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 1;

    long area = 0;

    avl_tree albero = NULL;
    AVL_table array;
    generic_cell uc;

    uc.t = DCELL_TYPE;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    Rast_set_d_null_value(&precCell, 1);
    for (j = 0; j < ad->rl; j++) {	/* for each row */
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	buf = RLI_get_dcell_raster_row(fd, j + ad->y, ad);

	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i] == 0)) {
		Rast_set_d_null_value(&corrCell, 1);
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		/* total patch area */
		area++;
	    }

	    if (!(Rast_is_null_value(&precCell, uc.t)) &&
		corrCell == precCell) {

		totCorr++;
	    }
	    else if (!(Rast_is_null_value(&precCell, uc.t)) &&
		     corrCell != precCell) {
		
		/* add precCell to search tree */
		if (albero == NULL) {
		    uc.val.dc = precCell;
		    albero = avl_make(uc, totCorr);
		    if (albero == NULL) {
			G_fatal_error("avl_make error");
			return RLI_ERRORE;
		    }
		    m++;
		}
		else {
		    uc.val.dc = precCell;
		    ris = avl_add(&albero, uc, totCorr);
		    switch (ris) {
		    case AVL_ERR:
			{
			    G_fatal_error("avl_add error");
			    return RLI_ERRORE;
			}
		    case AVL_ADD:
			{
			    m++;
			    break;
			}
		    case AVL_PRES:
			{
			    break;
			}
		    default:
			{
			    G_fatal_error("avl_make unknown error");
			    return RLI_ERRORE;
			}
		    }
		}
		totCorr = 1;
	    }		/* endif not equal cells */
	    precCell = corrCell;
	}
    }

    /* last closing */
    if (area > 0 && !(Rast_is_null_value(&precCell, uc.t))) {
	if (albero == NULL) {
	    uc.val.dc = precCell;
	    albero = avl_make(uc, totCorr);
	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    uc.val.dc = precCell;
	    ris = avl_add(&albero, uc, totCorr);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avl_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    m++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avl_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}
    }

    if (area > 0) {
	double t;
	double shannon;
	double perc, logarithm;

	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);
	if (tot != m) {
	    G_warning("avl_to_array unexpected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate shannon */
	shannon = 0;
	for (i = 0; i < m; i++) {
	    t = array[i].tot;
	    perc = t / area;
	    logarithm = log(perc);
	    shannon += perc * logarithm;
	}
	G_free(array);

	*result = -shannon;
    }
    else
	Rast_set_d_null_value(result, 1);

    avl_destroy(albero);
    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}


int calculateF(int fd, struct area_entry *ad, double *result)
{
    FCELL *buf;
    FCELL corrCell;
    FCELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf = NULL;
    int ris = 0;
    int masked = FALSE;

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 1;

    long area = 0;

    avl_tree albero = NULL;
    AVL_table array;
    generic_cell uc;

    uc.t = FCELL_TYPE;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	masked = TRUE;
    }

    Rast_set_f_null_value(&precCell, 1);
    for (j = 0; j < ad->rl; j++) {	/* for each row */
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}

	buf = RLI_get_fcell_raster_row(fd, j + ad->y, ad);

	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i] == 0)) {
		Rast_set_f_null_value(&corrCell, 1);
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		/* total patch area */
		area++;
	    }

	    if (!(Rast_is_null_value(&precCell, uc.t)) &&
		corrCell == precCell) {

		totCorr++;
	    }
	    else if (!(Rast_is_null_value(&precCell, uc.t)) &&
		     corrCell != precCell) {
		
		/* add precCell to search tree */
		if (albero == NULL) {
		    uc.val.fc = precCell;
		    albero = avl_make(uc, totCorr);
		    if (albero == NULL) {
			G_fatal_error("avl_make error");
			return RLI_ERRORE;
		    }
		    m++;
		}
		else {
		    uc.val.fc = precCell;
		    ris = avl_add(&albero, uc, totCorr);
		    switch (ris) {
		    case AVL_ERR:
			{
			    G_fatal_error("avl_add error");
			    return RLI_ERRORE;
			}
		    case AVL_ADD:
			{
			    m++;
			    break;
			}
		    case AVL_PRES:
			{
			    break;
			}
		    default:
			{
			    G_fatal_error("avl_make unknown error");
			    return RLI_ERRORE;
			}
		    }
		}
		totCorr = 1;
	    }		/* endif not equal cells */
	    precCell = corrCell;
	}
    }

    /* last closing */
    if (area > 0 && !(Rast_is_null_value(&precCell, uc.t))) {
	if (albero == NULL) {
	    uc.val.fc = precCell;
	    albero = avl_make(uc, totCorr);
	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    uc.val.fc = precCell;
	    ris = avl_add(&albero, uc, totCorr);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avl_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    m++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avl_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}
    }

    if (area > 0) {
	double t;
	double shannon;
	double perc, logarithm;

	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);
	if (tot != m) {
	    G_warning("avl_to_array unexpected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate shannon */
	shannon = 0;
	for (i = 0; i < m; i++) {
	    t = array[i].tot;
	    perc = t / area;
	    logarithm = log(perc);
	    shannon += perc * logarithm;
	}
	G_free(array);

	*result = -shannon;
    }
    else
	Rast_set_d_null_value(result, 1);

    avl_destroy(albero);
    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
    }

    return RLI_OK;
}
