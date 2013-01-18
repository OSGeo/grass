/*
 * \brief calculates dominance's diversity index
 *
 *   \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 *       \BUGS: please send bugs reports to pallecch@cli.di.unipi.it
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
	_("Calculates dominance's diversity index on a raster map");
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

    return calculateIndex(conf->answer, dominance, NULL, raster->answer,
			  output->answer);

}


int dominance(int fd, char **par, struct area_entry *ad, double *result)
{

    int ris = RLI_OK;
    double indice = 0;
    struct Cell_head hd;


    Rast_get_cellhd(ad->raster, "", &hd);

    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    calculate(fd, ad, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    calculateD(fd, ad, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    calculateF(fd, ad, &indice);
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
    double percentuale = 0;
    double area = 0;
    double t;
    double logaritmo;

    generic_cell cc;

    avl_tree albero = NULL;

    AVL_table *array;


    cc.t = CELL_TYPE;

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


    /*for each row */
    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_cell_raster_row(fd, j + ad->y, ad);


	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}


	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    area++;
	    corrCell = buf[i + ad->x];

	    if (masked && mask_buf[i + ad->x] == 0) {
		Rast_set_c_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, CELL_TYPE))) {
		a = 1;
		if (Rast_is_null_value(&precCell, cc.t)) {
		    precCell = corrCell;
		}

		if (corrCell != precCell) {
		    if (albero == NULL) {
			cc.val.c = precCell;
			albero = avl_make(cc, totCorr);

			if (albero == NULL) {
			    G_fatal_error("avl_make error");
			    return RLI_ERRORE;
			}
			m++;
		    }
		    else {
			cc.val.c = precCell;
			ris = avl_add(&albero, cc, totCorr);
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
		    totCorr = 1;
		}
		else {
		    totCorr++;
		}
		precCell = corrCell;
	    }

	}			/* end for */
    }				/* end for */



    if (a != 0) {
	/*last closing */
	if (albero == NULL) {
	    cc.val.c = precCell;
	    albero = avl_make(cc, totCorr);

	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    cc.val.c = precCell;
	    ris = avl_add(&albero, cc, totCorr);
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


	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);

	if (tot != m) {
	    G_warning
		("avl_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate summary */
	for (i = 0; i < m; i++) {
	    t = (double)array[i]->tot;
	    percentuale = (double)(t / area);
	    logaritmo = (double)log(percentuale);
	    somma = somma + (percentuale * logaritmo);
	}

	if (m != 0)
	    indice = log((double)m) + somma;
	else
	    indice = 0;
	G_free(array);
    }
    else			/*if a is 0, that is all cell are null, i put index=-1 */
	indice = (double)(-1);

    *result = indice;


    if (masked) {
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
    double percentuale = 0;
    double area = 0;
    double t;
    double logaritmo;

    avl_tree albero = NULL;

    AVL_table *array;

    generic_cell cc;

    cc.t = DCELL_TYPE;

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

    /*for each row */
    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_dcell_raster_row(fd, j + ad->y, ad);


	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}


	for (i = 0; i < ad->cl; i++) {	/* for each dcell in the row */
	    area++;
	    corrCell = buf[i + ad->x];

	    if (masked && mask_buf[i + ad->x] == 0) {
		Rast_set_d_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, DCELL_TYPE))) {
		a = 1;
		if (Rast_is_null_value(&precCell, DCELL_TYPE)) {
		    precCell = corrCell;
		}
		if (corrCell != precCell) {
		    if (albero == NULL) {
			cc.val.dc = precCell;
			albero = avl_make(cc, totCorr);
			if (albero == NULL) {
			    G_fatal_error("avl_make error");
			    return RLI_ERRORE;
			}
			m++;
		    }
		    else {
			cc.val.dc = precCell;
			ris = avl_add(&albero, cc, totCorr);
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
		    totCorr = 1;
		}
		else {
		    totCorr++;
		}
		precCell = corrCell;
	    }

	}			/*close for */
    }				/*close for */

    if (masked) {
	G_free(mask_buf);
    }


    if (a != 0) {
	/*last closing */
	if (albero == NULL) {
	    cc.val.dc = precCell;
	    albero = avl_make(cc, totCorr);
	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    cc.val.fc = precCell;
	    ris = avl_add(&albero, cc, totCorr);
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


	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);

	if (tot != m) {
	    G_warning
		("avl_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate summary */
	for (i = 0; i < m; i++) {
	    t = (double)array[i]->tot;
	    percentuale = (double)(t / area);
	    logaritmo = (double)log(percentuale);
	    somma = somma + (percentuale * logaritmo);
	}

	G_free(array);

	if (m != 0)
	    indice = log((double)m) + somma;
	else
	    indice = 0;
    }
    else
	/*if a is 0, that is all cell are null, i put index=-1 */
	indice = (double)(-1);


    *result = indice;


    return RLI_OK;
}


int calculateF(int fd, struct area_entry *ad, double *result)
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
    double percentuale = 0;
    double area = 0;
    double t;
    double logaritmo;

    avl_tree albero = NULL;

    AVL_table *array;

    generic_cell cc;

    cc.t = FCELL_TYPE;

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


    /*for each row */
    for (j = 0; j < ad->rl; j++) {
	buf = RLI_get_fcell_raster_row(fd, j + ad->y, ad);


	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}


	for (i = 0; i < ad->cl; i++) {	/* for each fcell in the row */
	    area++;
	    corrCell = buf[i + ad->x];
	    if (masked && mask_buf[i + ad->x] == 0) {
		Rast_set_f_null_value(&corrCell, 1);
		area--;
	    }

	    if (!(Rast_is_null_value(&corrCell, FCELL_TYPE))) {
		a = 1;
		if (Rast_is_null_value(&precCell, FCELL_TYPE)) {
		    precCell = corrCell;
		}
		if (corrCell != precCell) {
		    if (albero == NULL) {
			cc.val.fc = precCell;
			albero = avl_make(cc, totCorr);
			if (albero == NULL) {
			    G_fatal_error("avl_make error");
			    return RLI_ERRORE;
			}
			m++;
		    }
		    else {
			cc.val.fc = precCell;
			ris = avl_add(&albero, cc, totCorr);
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
		    totCorr = 1;
		}
		else {
		    totCorr++;
		}

		precCell = corrCell;
	    }

	}
    }


    /*last closing */
    if (a != 0) {
	if (albero == NULL) {
	    cc.val.fc = precCell;
	    albero = avl_make(cc, totCorr);
	    if (albero == NULL) {
		G_fatal_error("avl_make error");
		return RLI_ERRORE;
	    }
	    m++;
	}
	else {
	    cc.val.fc = precCell;
	    ris = avl_add(&albero, cc, totCorr);
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


	array = G_malloc(m * sizeof(AVL_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avl_to_array(albero, zero, array);

	if (tot != m) {
	    G_warning
		("avl_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	/* calculate summary */
	for (i = 0; i < m; i++) {
	    t = (double)array[i]->tot;
	    percentuale = (double)(t / area);
	    logaritmo = (double)log(percentuale);
	    somma = somma + (percentuale * logaritmo);
	}

	if (m != 0)
	    indice = log((double)m) + somma;
	else
	    indice = 0;

	G_free(array);
    }


    else			/*if a is 0, that is all cell are null, i put index=-1 */
	indice = (double)(-1);


    *result = indice;



    if (masked) {
	G_free(mask_buf);
    }
    return RLI_OK;
}
