/*
 * \brief calculates mean patch size index
 *
 *  \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "../r.li.daemon/defs.h"
#include "../r.li.daemon/avlDefs.h"
#include "../r.li.daemon/avlID.h"
#include "../r.li.daemon/daemon.h"


int calculate(int fd, struct area_entry *ad, struct Cell_head hd, double *result);
int calculateD(int fd, struct area_entry *ad, struct Cell_head hd, double *result);
int calculateF(int fd, struct area_entry *ad, struct Cell_head hd, double *result);


int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates mean patch size index on a raster map, using a 4 neighbour algorithm");
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

    return calculateIndex(conf->answer, meanPatchSize, NULL, raster->answer,
			  output->answer);

}



int meanPatchSize(int fd, char **par, struct area_entry *ad, double *result)
{

    int ris = 0;
    double indice = 0;
    struct Cell_head hd;

    Rast_get_cellhd(ad->raster, "", &hd);

    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    ris = calculate(fd, ad, hd, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    ris = calculateD(fd, ad, hd, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    ris = calculateF(fd, ad, hd, &indice);
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


int calculate(int fd, struct area_entry *ad, struct Cell_head hd, double *result)
{
    CELL *buf;
    CELL *buf_sup;

    CELL corrCell;
    CELL precCell;
    CELL supCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;


    long npatch = 0;
    long tot = 0;
    long zero = 0;
    long uno = 1;
    long idCorr = 0;
    long lastId = 0;
    long doppi = 0;
    long *mask_patch_sup;
    long *mask_patch_corr;

    double indice = 0;
    double area = 0;		/*if all cells are null area=0 */
    double areaCorrect = 0;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;

    avlID_tree albero = NULL;

    avlID_table *array;



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

    mask_patch_sup = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_sup == NULL) {
	G_fatal_error("malloc mask_patch_sup failed");
	return RLI_ERRORE;
    }

    mask_patch_corr = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_corr == NULL) {
	G_fatal_error("malloc mask_patch_corr failed");
	return RLI_ERRORE;
    }

    buf_sup = Rast_allocate_c_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }

    buf = Rast_allocate_c_buf();
    if (buf == NULL) {
	G_fatal_error("malloc buf failed");
	return RLI_ERRORE;
    }

    Rast_set_c_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */

    for (i = 0; i < ad->cl; i++) {
	mask_patch_sup[i] = 0;
	mask_patch_corr[i] = 0;
    }

    /*for each raster row */

    for (j = 0; j < ad->rl; j++) {
	if (j > 0) {
	    buf_sup = RLI_get_cell_raster_row(fd, j - 1 + ad->y, ad);
	}
	buf = RLI_get_cell_raster_row(fd, j + ad->y, ad);
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}
	Rast_set_c_null_value(&precCell, 1);
	for (i = 0; i < ad->cl; i++) {	/*for each cell in the row */

	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i + ad->x] == 0)) {
		Rast_set_c_null_value(&corrCell, 1);
	    }

	    /*valid cell */
	    if (!(Rast_is_null_value(&corrCell, CELL_TYPE))) {
		area++;
		if (i > 0)
		    precCell = buf[i - 1 + ad->x];

		if (j == 0)
		    Rast_set_c_null_value(&supCell, 1);
		else
		    supCell = buf_sup[i + ad->x];


		if (corrCell != precCell) {
		    if (corrCell != supCell) {
			/*new patch */
			if (idCorr == 0) {	/*first patch */
			    lastId = 1;
			    idCorr = 1;
			    mask_patch_corr[i] = idCorr;
			}
			else {	/*not first patch */
			    /* put in the tree previous values */
			    if (albero == NULL) {
				albero = avlID_make(idCorr, uno);
				if (albero == NULL) {
				    G_fatal_error("avlID_make error");
				    return RLI_ERRORE;
				}
				npatch++;

			    }
			    else {	/* tree not null */

				ris = avlID_add(&albero, idCorr, uno);
				switch (ris) {
				case AVL_ERR:
				    {
					G_fatal_error("avlID_add error");
					return RLI_ERRORE;
				    }
				case AVL_ADD:
				    {
					npatch++;
					break;
				    }
				case AVL_PRES:
				    {
					break;
				    }
				default:
				    {
					G_fatal_error
					    ("avlID_add unknown error");
					return RLI_ERRORE;
				    }
				}
			    }

			    lastId++;
			    idCorr = lastId;
			    mask_patch_corr[i] = idCorr;
			}
		    }
		    else {	/*current cell and upper cell are equal */

			if ((corrCell == precCell) &&
			    (mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			    long r = 0;
			    long del = mask_patch_sup[i];

			    r = avlID_sub(&albero, del);
			    if (r == 0) {
				G_fatal_error("avlID_sub error");
				return RLI_ERRORE;
			    }
			    /*Remove one patch because it makes part of a patch already found */
			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			    r = i;
			    while (i < ad->cl) {
				if (mask_patch_sup[r] == del) {
				    mask_patch_sup[r] = idCorr;
				}
				else {
				    r = ad->cl + 1;
				}
			    }
			}

			if (albero == NULL) {
			    albero = avlID_make(idCorr, uno);
			    if (albero == NULL) {
				G_fatal_error("avlID_make error");
				return RLI_ERRORE;
			    }
			    npatch++;
			}
			else {	/* tree not null */

			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			}

			idCorr = mask_patch_sup[i];
			mask_patch_corr[i] = idCorr;
		    }
		}
		else {		/*current cell and previous cell are equal */


		    if ((corrCell == supCell) &&
			(mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			int l;

			mask_patch_corr[i] = mask_patch_sup[i];
			l = i - 1;
			while (l >= 0) {
			    if (mask_patch_corr[l] == idCorr) {
				mask_patch_corr[l] = mask_patch_sup[i];
				l--;
			    }
			    else {
				l = (-1);
			    }
			}
			lastId--;
			idCorr = mask_patch_sup[i];
		    }
		    else {
			mask_patch_corr[i] = idCorr;
		    }

		}
	    }
	    else {		/*cell not to consider or cell is null */

		mask_patch_corr[i] = 0;
	    }
	}
	mask_patch_sup = mask_patch_corr;
    }



    if (area != 0) {
	if (albero == NULL) {
	    albero = avlID_make(idCorr, uno);
	    if (albero == NULL) {
		G_fatal_error("avlID_make error");
		return RLI_ERRORE;
	    }
	    npatch++;
	}
	else {
	    ris = avlID_add(&albero, idCorr, uno);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avlID_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    npatch++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avlID_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}


	array = G_malloc(npatch * sizeof(avlID_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avlID_to_array(albero, zero, array);

	if (tot != npatch) {
	    G_warning
		("avlID_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	for (i = 0; i < npatch; i++) {
	    if (array[i]->tot == 0)
		doppi++;
	}
	npatch = npatch - doppi;

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

	areaCorrect = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	    (((NS_DIST1 + NS_DIST2) / 2) / hd.rows) * (area);
	indice = areaCorrect / npatch;
	G_free(array);
    }
    else
	indice = (double)(0);


    *result = indice;


    if (masked)
	G_free(mask_buf);

    G_free(mask_patch_corr);

    G_free(buf_sup);
    return RLI_OK;
}


int calculateD(int fd, struct area_entry *ad, struct Cell_head hd, double *result)
{
    DCELL *buf;
    DCELL *buf_sup;

    DCELL corrCell;
    DCELL precCell;
    DCELL supCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;

    long npatch = 0;
    long tot = 0;
    long zero = 0;
    long uno = 1;
    long idCorr = 0;
    long lastId = 0;
    long doppi = 0;
    long *mask_patch_sup;
    long *mask_patch_corr;

    double indice = 0;
    double area = 0;		/*if all cells are null area=0 */
    double areaCorrect = 0;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;

    avlID_tree albero = NULL;

    avlID_table *array;



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

    mask_patch_sup = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_sup == NULL) {
	G_fatal_error("malloc mask_patch_sup failed");
	return RLI_ERRORE;
    }

    mask_patch_corr = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_corr == NULL) {
	G_fatal_error("malloc mask_patch_corr failed");
	return RLI_ERRORE;
    }

    buf_sup = Rast_allocate_d_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }

    buf = Rast_allocate_d_buf();
    if (buf == NULL) {
	G_fatal_error("malloc buf failed");
	return RLI_ERRORE;
    }

    Rast_set_d_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */

    for (i = 0; i < ad->cl; i++) {
	mask_patch_sup[i] = 0;
	mask_patch_corr[i] = 0;
    }

    /*read each row and put in an avlId tree cell value with the number of cells which have that value */
    for (j = 0; j < ad->rl; j++) {
	if (j > 0) {
	    buf_sup = RLI_get_dcell_raster_row(fd, j - 1 + ad->y, ad);
	}
	buf = RLI_get_dcell_raster_row(fd, j + ad->y, ad);
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}
	Rast_set_d_null_value(&precCell, 1);
	for (i = 0; i < ad->cl; i++) {	/*for each cell in the row */

	    corrCell = buf[i + ad->x];

	    if ((masked) && (mask_buf[i + ad->x] == 0)) {
		Rast_set_d_null_value(&corrCell, 1);
	    }

	    if (!(Rast_is_null_value(&corrCell, DCELL_TYPE))) {
		area++;
		if (i > 0)
		    precCell = buf[i - 1 + ad->x];

		if (j == 0)
		    Rast_set_d_null_value(&supCell, 1);
		else
		    supCell = buf_sup[i + ad->x];

		if (corrCell != precCell) {
		    if (corrCell != supCell) {
			/*new patch */
			if (idCorr == 0) {	/*first patch */
			    lastId = 1;
			    idCorr = 1;
			    mask_patch_corr[i] = idCorr;
			}
			else {	/*not first patch */
			    /* put in the tree previous value */
			    if (albero == NULL) {
				albero = avlID_make(idCorr, uno);
				if (albero == NULL) {
				    G_fatal_error("avlID_make error");
				    return RLI_ERRORE;
				}
				npatch++;

			    }
			    else {	/* tree not null */

				ris = avlID_add(&albero, idCorr, uno);
				switch (ris) {
				case AVL_ERR:
				    {
					G_fatal_error("avlID_add error");
					return RLI_ERRORE;
				    }
				case AVL_ADD:
				    {
					npatch++;
					break;
				    }
				case AVL_PRES:
				    {
					break;
				    }
				default:
				    {
					G_fatal_error
					    ("avlID_add unknown error");
					return RLI_ERRORE;
				    }
				}
			    }

			    lastId++;
			    idCorr = lastId;
			    mask_patch_corr[i] = idCorr;
			}
		    }
		    else {	/*current cell and upper cell are equal */

			if ((corrCell == precCell) &&
			    (mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			    long r = 0;
			    long del = mask_patch_sup[i];

			    /*Remove one patch because it makes part of a patch already found */
			    r = avlID_sub(&albero, del);
			    if (r == 0) {
				G_fatal_error("avlID_sub error");
				return RLI_ERRORE;
			    }

			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			    r = i;
			    while (i < ad->cl) {
				if (mask_patch_sup[r] == del) {
				    mask_patch_sup[r] = idCorr;
				}
				else {
				    r = ad->cl + 1;
				}
			    }
			}

			if (albero == NULL) {
			    albero = avlID_make(idCorr, uno);
			    if (albero == NULL) {
				G_fatal_error("avlID_make error");
				return RLI_ERRORE;
			    }
			    npatch++;
			}
			else {	/*tree not null */

			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			}

			idCorr = mask_patch_sup[i];
			mask_patch_corr[i] = idCorr;
		    }
		}
		else {		/*current cell and previous cell are equals */


		    if ((corrCell == supCell) &&
			(mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			int l;

			mask_patch_corr[i] = mask_patch_sup[i];
			l = i - 1;
			while (l >= 0) {
			    if (mask_patch_corr[l] == idCorr) {
				mask_patch_corr[l] = mask_patch_sup[i];
				l--;
			    }
			    else {
				l = (-1);
			    }
			}
			lastId--;
			idCorr = mask_patch_sup[i];
		    }
		    else {
			mask_patch_corr[i] = idCorr;
		    }

		}
	    }
	    else {		/*cell null or not to consider */

		mask_patch_corr[i] = 0;
	    }
	}
	mask_patch_sup = mask_patch_corr;
    }



    if (area != 0) {
	if (albero == NULL) {
	    albero = avlID_make(idCorr, uno);
	    if (albero == NULL) {
		G_fatal_error("avlID_make error");
		return RLI_ERRORE;
	    }
	    npatch++;
	}
	else {
	    ris = avlID_add(&albero, idCorr, uno);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avlID_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    npatch++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avlID_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}


	array = G_malloc(npatch * sizeof(avlID_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avlID_to_array(albero, zero, array);

	if (tot != npatch) {
	    G_warning
		("avlID_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	for (i = 0; i < npatch; i++) {
	    if (array[i]->tot == 0)
		doppi++;

	}
	npatch = npatch - doppi;

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

	areaCorrect = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	    (((NS_DIST1 + NS_DIST2) / 2) / hd.rows) * (area);
	indice = areaCorrect / npatch;
	G_free(array);
    }
    else
	indice = (double)(0);


    *result = indice;


    if (masked)
	G_free(mask_buf);

    G_free(mask_patch_corr);

    return RLI_OK;
}


int calculateF(int fd, struct area_entry *ad, struct Cell_head hd, double *result)
{
    FCELL *buf;
    FCELL *buf_sup;

    FCELL corrCell;
    FCELL precCell;
    FCELL supCell;

    int i, j;
    int mask_fd = -1, *mask_buf;
    int ris = 0;
    int masked = FALSE;


    long npatch = 0;
    long tot = 0;
    long zero = 0;
    long uno = 1;
    long idCorr = 0;
    long lastId = 0;
    long doppi = 0;
    long *mask_patch_sup;
    long *mask_patch_corr;

    double indice = 0;
    double area = 0;		/*if all cells are null area=0 */
    double areaCorrect = 0;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;

    avlID_tree albero = NULL;

    avlID_table *array;



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

    mask_patch_sup = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_sup == NULL) {
	G_fatal_error("malloc mask_patch_sup failed");
	return RLI_ERRORE;
    }

    mask_patch_corr = G_malloc(ad->cl * sizeof(long));
    if (mask_patch_corr == NULL) {
	G_fatal_error("malloc mask_patch_corr failed");
	return RLI_ERRORE;
    }

    buf_sup = Rast_allocate_f_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }


    buf = Rast_allocate_f_buf();
    if (buf == NULL) {
	G_fatal_error("malloc buf failed");
	return RLI_ERRORE;
    }

    Rast_set_f_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */

    for (i = 0; i < ad->cl; i++) {
	mask_patch_sup[i] = 0;
	mask_patch_corr[i] = 0;
    }
    /*for each raster row */

    for (j = 0; j < ad->rl; j++) {
	if (j > 0) {
	    buf_sup = RLI_get_fcell_raster_row(fd, j - 1 + ad->y, ad);
	}
	buf = RLI_get_fcell_raster_row(fd, j + ad->y, ad);
	if (masked) {
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("mask read failed");
		return RLI_ERRORE;
	    }
	}
	Rast_set_f_null_value(&precCell, 1);
	for (i = 0; i < ad->cl; i++) {	/*for each cell in the row */


	    corrCell = buf[i + ad->x];
	    if (((masked) && (mask_buf[i + ad->x] == 0))) {
		Rast_set_f_null_value(&corrCell, 1);
	    }
	    if (!(Rast_is_null_value(&corrCell, FCELL_TYPE))) {
		area++;
		if (i > 0)
		    precCell = buf[i - 1 + ad->x];
		if (j == 0)
		    Rast_set_f_null_value(&supCell, 1);
		else
		    supCell = buf_sup[i + ad->x];


		if (corrCell != precCell) {
		    if (corrCell != supCell) {
			/*new patch */
			if (idCorr == 0) {	/*first patch */
			    lastId = 1;
			    idCorr = 1;
			    mask_patch_corr[i] = idCorr;
			}
			else {	/*not first patch */
			    /* put in the tree previous value */
			    if (albero == NULL) {
				albero = avlID_make(idCorr, uno);
				if (albero == NULL) {
				    G_fatal_error("avlID_make error");
				    return RLI_ERRORE;
				}
				npatch++;

			    }
			    else {	/*tree not empty */

				ris = avlID_add(&albero, idCorr, uno);
				switch (ris) {
				case AVL_ERR:
				    {
					G_fatal_error("avlID_add error");
					return RLI_ERRORE;
				    }
				case AVL_ADD:
				    {
					npatch++;
					break;
				    }
				case AVL_PRES:
				    {
					break;
				    }
				default:
				    {
					G_fatal_error
					    ("avlID_add unknown error");
					return RLI_ERRORE;
				    }
				}
			    }

			    lastId++;
			    idCorr = lastId;
			    mask_patch_corr[i] = idCorr;
			}
		    }
		    else {	/*current cell and upper cell are equal */

			if ((corrCell == precCell) &&
			    (mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			    long r = 0;
			    long del = mask_patch_sup[i];

			    r = avlID_sub(&albero, del);
			    if (r == 0) {
				G_fatal_error("avlID_sub error");
				return RLI_ERRORE;
			    }
			    /*Remove one patch because it makes part of a patch already found */
			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			    r = i;
			    while (i < ad->cl) {
				if (mask_patch_sup[r] == del) {
				    mask_patch_sup[r] = idCorr;
				}
				else {
				    r = ad->cl + 1;
				}
			    }
			}

			if (albero == NULL) {
			    albero = avlID_make(idCorr, uno);
			    if (albero == NULL) {
				G_fatal_error("avlID_make error");
				return RLI_ERRORE;
			    }
			    npatch++;
			}
			else {	/*the tree (albero) isn't null */

			    ris = avlID_add(&albero, idCorr, uno);
			    switch (ris) {
			    case AVL_ERR:
				{
				    G_fatal_error("avlID_add error");
				    return RLI_ERRORE;
				}
			    case AVL_ADD:
				{
				    npatch++;
				    break;
				}
			    case AVL_PRES:
				{
				    break;
				}
			    default:
				{
				    G_fatal_error("avlID_add unknown error");
				    return RLI_ERRORE;
				}
			    }
			}

			idCorr = mask_patch_sup[i];
			mask_patch_corr[i] = idCorr;
		    }
		}
		else {		/*current cell and previous cell are equal */

		    if ((corrCell == supCell) &&
			(mask_patch_sup[i] != mask_patch_corr[i - 1])) {
			int l;

			mask_patch_corr[i] = mask_patch_sup[i];
			l = i - 1;
			while (l >= 0) {
			    if (mask_patch_corr[l] == idCorr) {
				mask_patch_corr[l] = mask_patch_sup[i];
				l--;
			    }
			    else {
				l = (-1);
			    }
			}
			lastId--;
			idCorr = mask_patch_sup[i];
		    }
		    else {
			mask_patch_corr[i] = idCorr;
		    }

		}
	    }
	    else {		/*null cell or cell not to consider */

		mask_patch_corr[i] = 0;
	    }
	}
	mask_patch_sup = mask_patch_corr;
    }



    if (area != 0) {
	if (albero == NULL) {
	    albero = avlID_make(idCorr, uno);
	    if (albero == NULL) {
		G_fatal_error("avlID_make error");
		return RLI_ERRORE;
	    }
	    npatch++;
	}
	else {
	    ris = avlID_add(&albero, idCorr, uno);
	    switch (ris) {
	    case AVL_ERR:
		{
		    G_fatal_error("avlID_add error");
		    return RLI_ERRORE;
		}
	    case AVL_ADD:
		{
		    npatch++;
		    break;
		}
	    case AVL_PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("avlID_add unknown error");
		    return RLI_ERRORE;
		}
	    }
	}


	array = G_malloc(npatch * sizeof(avlID_tableRow));
	if (array == NULL) {
	    G_fatal_error("malloc array failed");
	    return RLI_ERRORE;
	}
	tot = avlID_to_array(albero, zero, array);

	if (tot != npatch) {
	    G_warning
		("avlID_to_array unaspected value. the result could be wrong");
	    return RLI_ERRORE;
	}

	for (i = 0; i < npatch; i++) {
	    if (array[i]->tot == 0)
		doppi++;

	}
	npatch = npatch - doppi;

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

	areaCorrect = (((EW_DIST1 + EW_DIST2) / 2) / hd.cols) *
	    (((NS_DIST1 + NS_DIST2) / 2) / hd.rows) * (area);
	indice = areaCorrect / npatch;
	G_free(array);
    }
    else
	indice = (double)(0);


    *result = indice;


    if (masked)
	G_free(mask_buf);

    G_free(mask_patch_corr);

    return RLI_OK;
}
