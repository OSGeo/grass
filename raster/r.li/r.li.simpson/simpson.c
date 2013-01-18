
/*
 * \brief calculates Simpson's diversity index
 *
 *  \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

#include "../r.li.daemon/defs.h"
#include "../r.li.daemon/avlDefs.h"
#include "../r.li.daemon/avl.h"
#include "../r.li.daemon/daemon.h"

double calculate(struct area_entry *ad, int fd, double *result);
double calculateD(struct area_entry *ad, int fd, double *result);
double calculateF(struct area_entry *ad, int fd, double *result);

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates Simpson's diversity index on a raster map");
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

    return calculateIndex(conf->answer, simpson, NULL, raster->answer,
			  output->answer);

}

int simpson(int fd, char **par, struct area_entry *ad, double *result)
{
    int ris = RLI_OK;
    double indice = 0;
    struct Cell_head hd;

    Rast_get_cellhd(ad->raster, "", &hd);

    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    ris = calculate(ad, fd, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    ris = calculateD(ad, fd, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    ris = calculateF(ad, fd, &indice);
	    break;
	}
    default:
	{
	    G_fatal_error("data type unknown");
	    return RLI_ERRORE;
	}

    }

    if (ris != RLI_OK)
	return RLI_ERRORE;

    *result = indice;

    return RLI_OK;

}



double calculate(struct area_entry *ad, int fd, double *result)
{
    CELL *buf;
    CELL corrCell;
    CELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;
    int a = 0;			/* a=0 if all cells are null */

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 0;

    double indice = 0;
    double somma = 0;
    double p = 0;
    double area = 0;
    double t;

    avl_tree albero = NULL;

    AVL_table *array;

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
	    area++;
	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i + ad->x] == 0)) {
		Rast_set_c_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		a = 1;
		if (Rast_is_null_value(&precCell, uc.t)) {
		    precCell = corrCell;
		}
		if (corrCell != precCell) {
		    if (albero == NULL) {
			uc.val.c = precCell;
			albero = avl_make(uc, totCorr);
			if (albero == NULL) {
			    G_fatal_error("avl_make error");
			    return RLI_ERRORE;
			}
			else
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
		else {		/*equal cells */

		    totCorr++;
		}
		precCell = corrCell;
	    }

	}
    }

    /*last closing */
    if (a != 0) {
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
    array = G_malloc(m * sizeof(AVL_tableRow));
    if (array == NULL) {
	G_fatal_error("malloc array failed");
	return RLI_ERRORE;
    }
    tot = avl_to_array(albero, zero, array);
    if (tot != m) {
	G_warning("avl_to_array unaspected value. the result could be wrong");
	return RLI_ERRORE;
    }

    /* claculate index summary */
    for (i = 0; i < m; i++) {
	t = (double)(array[i]->tot);
	p = t / area;
	somma = somma + (p * p);
    }

    if (a != 0) {
	indice = 1 - somma;
    }
    else			/*if a is 0, that is all cell are null, i put index=-1 */
	indice = (double)(-1);


    G_free(array);
    if (masked)
	G_free(mask_buf);

    *result = indice;

    return RLI_OK;
}


double calculateD(struct area_entry *ad, int fd, double *result)
{
    DCELL *buf;
    DCELL corrCell;
    DCELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;
    int a = 0;			/* a=0 if all cells are null */

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 0;

    double indice = 0;
    double somma = 0;
    double p = 0;
    double area = 0;
    double t;

    avl_tree albero = NULL;

    AVL_table *array;

    generic_cell uc;

    uc.t = DCELL_TYPE;

    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buffailed");
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

	for (i = 0; i < ad->cl; i++) {	/* for each dcell in the row */
	    area++;
	    corrCell = buf[i + ad->x];

	    if (masked && mask_buf[i + ad->x] == 0) {
		Rast_set_d_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		a = 1;

		if (Rast_is_null_value(&precCell, uc.t)) {
		    precCell = corrCell;
		}
		if (corrCell != precCell) {
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
		}		/* endif not equal dcells */
		else {		/*equal dcells */

		    totCorr++;
		}
		precCell = corrCell;
	    }


	}
    }

    /*last closing */
    if (a != 0) {
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
    array = G_malloc(m * sizeof(AVL_tableRow));
    if (array == NULL) {
	G_fatal_error("malloc array failed");
	return RLI_ERRORE;
    }
    tot = avl_to_array(albero, zero, array);
    if (tot != m) {
	G_warning("avl_to_array unaspected value. the result could be wrong");
	return RLI_ERRORE;
    }

    /* claculate index summary */
    for (i = 0; i < m; i++) {
	t = (double)(array[i]->tot);
	p = t / area;
	somma = somma + (p * p);
    }

    if (a != 0) {		/*if a is 0, that is all cell are null, i put index=-1 */
	indice = 1 - somma;
    }
    else
	indice = (double)(-1);

    G_free(array);
    if (masked)
	G_free(mask_buf);

    *result = indice;
    return RLI_OK;
}



double calculateF(struct area_entry *ad, int fd, double *result)
{
    FCELL *buf;
    FCELL corrCell;
    FCELL precCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;
    int a = 0;			/* a=0 if all cells are null */

    long m = 0;
    long tot = 0;
    long zero = 0;
    long totCorr = 0;

    double indice = 0;
    double somma = 0;
    double p = 0;
    double area = 0;
    double t;

    avl_tree albero = NULL;

    AVL_table *array;

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


	for (i = 0; i < ad->cl; i++) {	/* for each fcell in the row */

	    area++;
	    corrCell = buf[i + ad->x];

	    if (masked && mask_buf[i + ad->x] == 0) {
		Rast_set_f_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, uc.t))) {
		a = 1;
		if (Rast_is_null_value(&precCell, uc.t)) {
		    precCell = corrCell;
		}
		if (corrCell != precCell) {
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
		}		/* endif not equal fcells */
		else {		/*equal fcells */

		    totCorr++;
		}
		precCell = corrCell;
	    }


	}
    }

    /*last closing */
    if (a != 0) {
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

    array = G_malloc(m * sizeof(AVL_tableRow));
    if (array == NULL) {
	G_fatal_error("malloc array failed");
	return RLI_ERRORE;
    }
    tot = avl_to_array(albero, zero, array);
    if (tot != m) {
	G_warning("avl_to_array unaspected value. the result could be wrong");
	return RLI_ERRORE;
    }

    /* calculate index summary */
    for (i = 0; i < m; i++) {
	t = (double)(array[i]->tot);
	p = t / area;
	somma = somma + (p * p);
    }

    if (a != 0) {		/*if a is 0, that is all cell are null, i put index=-1 */
	indice = 1 - somma;
    }
    else
	indice = (double)(-1);


    G_free(array);
    if (masked)
	G_free(mask_buf);


    *result = indice;
    return RLI_OK;
}
