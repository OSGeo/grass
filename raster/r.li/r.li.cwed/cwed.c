/*
 * \brief calculates contrast weighted edge density index
 *
 *   \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include <fcntl.h>		/* for O_RDONLY usage */

#include "../r.li.daemon/defs.h"
#include "../r.li.daemon/daemon.h"

#include "cellWeighted.h"
#include "utility.h"

#define NMAX 512
#define _ADD 0
#define _PRES 1
#define _ERR -1

int calculate(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	      double *result);
int calculateD(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	       double *result);
int calculateF(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	       double *result);

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *path, *output;
    struct GModule *module;
    char **par = NULL;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates contrast weighted edge density index on a raster map");
    G_add_keyword(_("raster"));
    G_add_keyword(_("landscape structure analysis"));
    G_add_keyword(_("patch index"));

    /* define options */

    raster = G_define_standard_option(G_OPT_R_INPUT);

    conf = G_define_standard_option(G_OPT_F_INPUT);
    conf->key = "config";
    conf->description = _("Configuration file");
    conf->required = YES;

    path = G_define_standard_option(G_OPT_F_INPUT);
    path->key = "path";
    path->description =
        _("Name of file that contains the weight to calculate the index");
    path->required = YES;

    output = G_define_standard_option(G_OPT_R_OUTPUT);


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (path->answer == NULL)
	par = NULL;
    else
	par = &path->answer;

    return calculateIndex(conf->answer, contrastWeightedEdgeDensity, par,
			  raster->answer, output->answer);

}


int contrastWeightedEdgeDensity(int fd, char **par, struct area_entry *ad,
				double *result)
{
    double indice = 0;		/* the result */

    struct Cell_head hd;

    int i = 0;
    int file_fd = -1;
    int l;			/*number of read byte */
    int ris = 0;

    char *file;
    char *strFile;

    char row[NMAX];		/* to read the file */

    char **bufRighe;		/* contains every valid file row */

    char separatore;		/* separator to split a string */

    long totCoppie = 0;		/* number of cells pair */
    long totRow = 0;		/* of the file */
    long tabSize = 10;		/* array length */

    Coppie *cc = NULL;		/* here store the pair of cell with the weight. these information are in the file */


    /* try to open file */
    file = par[0];
    file_fd = open(file, O_RDONLY);
    if (file_fd == -1) {
	G_fatal_error("can't  open file %s", file);
	return RLI_ERRORE;
    }



    strFile = concatena("", "");
    if (strFile == NULL) {
	G_fatal_error("can't  concat strFile");
	return RLI_ERRORE;
    }

    while ((l = read(file_fd, row, NMAX)) > 0) {
	strFile = concatena(strFile, row);
	if (strFile == NULL) {
	    G_fatal_error("can't  concat strFile 2");
	    return RLI_ERRORE;
	}
    }

    l = close(file_fd);

    if (l != 0) {
	G_warning("errore chiusura file %s", file);
    }

    /* 
     * every row of a rigth file has this layout
     * CELL1,CELL2,dissimilarity  
     */

    /* test if the file is ok and store every line in an array of CoppiaPesata */
    separatore = '\n';
    bufRighe = split_arg(strFile, separatore, &totRow);
    if (bufRighe == NULL) {
	G_fatal_error("can't  split buf_righe\n");
	return RLI_ERRORE;
    }

    cc = G_malloc(tabSize * sizeof(CoppiaPesata));
    if (cc == NULL) {
	G_fatal_error("malloc cc failed");
	return RLI_ERRORE;
    }

    Rast_get_cellhd(ad->raster, "", &hd);

    for (i = 0; i < totRow; i++) {
	long num = 0;
	char **b;
	generic_cell c1, c2;
	double p;
	int ris;

	separatore = ',';
	b = split_arg(bufRighe[i], separatore, &num);

	if (b == NULL) {
	    G_fatal_error("can't split bufRighe [%d]", i);
	    return RLI_ERRORE;
	}

	if (num != 1) {

	    if (num != 3) {
		G_fatal_error("wrong file format at line %d", i + 1);
		return RLI_ERRORE;
	    }

	    c1.t = ad->data_type;
	    c2.t = ad->data_type;
	    switch (ad->data_type) {
	    case CELL_TYPE:
		{
		    c1.val.c = atoi(b[0]);
		    c2.val.c = atoi(b[1]);
		    break;
		}
	    case DCELL_TYPE:
		{
		    c1.val.dc = atof(b[0]);
		    c2.val.dc = atof(b[1]);
		    break;
		}
	    case FCELL_TYPE:
		{
		    c1.val.fc = (float)atof(b[0]);
		    c2.val.fc = (float)atof(b[1]);
		    break;
		}
	    default:
		{
		    G_fatal_error("data type unknown");
		    return RLI_ERRORE;
		}
	    }
	    p = atof(b[2]);

	    if (tabSize == totCoppie) {
		tabSize += 10;
		cc = G_realloc(cc, tabSize * sizeof(CoppiaPesata));
		if (cc == NULL) {
		    G_fatal_error("realloc cc failed");
		    return _ERR;
		}
	    }


	    ris = addCoppia(cc, c1, c2, p, totCoppie, &tabSize);
	    switch (ris) {
	    case _ERR:
		{
		    G_fatal_error("add error");
		    return RLI_ERRORE;
		}
	    case _ADD:
		{
		    totCoppie++;
		    break;
		}
	    case _PRES:
		{
		    break;
		}
	    default:
		{
		    G_fatal_error("add unknown error");
		    return RLI_ERRORE;
		}
	    }

	}
	else;
	/* num = 1  ---> in the line there is only 1 token 
	 * I ignore this line
	 */

    }




    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    ris = calculate(fd, ad, cc, totCoppie, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    ris = calculateD(fd, ad, cc, totCoppie, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    ris = calculateF(fd, ad, cc, totCoppie, &indice);
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

    G_free(cc);

    return RLI_OK;
}




int calculate(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	      double *result)
{

    double indice = 0;
    double somma = 0;
    double area = 0;

    int i = 0, j;
    int mask_fd = -1;
    int masked = FALSE;
    int *mask_corr, *mask_sup;

    CELL *buf_corr, *buf_sup;
    CELL prevCell, corrCell, supCell;

    generic_cell c1;
    generic_cell c2;


    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't open mask");
	    return RLI_ERRORE;
	}

	mask_corr = G_malloc(ad->cl * sizeof(int));
	if (mask_corr == NULL) {
	    G_fatal_error("malloc mask_corr failed");
	    return RLI_ERRORE;
	}

	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_sup failed");
	    return RLI_ERRORE;
	}

	masked = TRUE;
    }


    buf_sup = Rast_allocate_c_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }

    c1.t = CELL_TYPE;
    c2.t = CELL_TYPE;

    buf_corr = Rast_allocate_c_buf();
    if (buf_corr == NULL) {
	G_fatal_error("error malloc buf_corr");
	return RLI_ERRORE;
    }

    Rast_set_c_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */
    for (j = 0; j < ad->rl; j++) {	/* for each row */
	buf_corr = RLI_get_cell_raster_row(fd, j + ad->y, ad);	/* read row of raster */
	if (j > 0) {		/* not first row */
	    buf_sup = RLI_get_cell_raster_row(fd, j - 1 + ad->y, ad);
	}
	/*read mask if needed */
	if (masked) {
	    if (read(mask_fd, mask_corr, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("reading mask_corr");
		return RLI_ERRORE;
	    }
	}

	Rast_set_c_null_value(&prevCell, 1);
	Rast_set_c_null_value(&corrCell, 1);
	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    area++;
	    corrCell = buf_corr[i + ad->x];
	    if (masked && mask_corr[i + ad->x] == 0) {
		area--;
		Rast_set_c_null_value(&corrCell, 1);
	    }
	    if (!(Rast_is_null_value(&corrCell, CELL_TYPE))) {
		supCell = buf_sup[i + ad->x];
		/* calculate how many edge the cell has */

		if (((!Rast_is_null_value(&prevCell, CELL_TYPE))) &&
		    (corrCell != prevCell)) {
		    int r = 0;

		    c1.val.c = corrCell;
		    c2.val.c = prevCell;

		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;

		}

		if ((!(Rast_is_null_value(&supCell, CELL_TYPE))) &&
		    (corrCell != supCell)) {
		    int r = 0;

		    c1.val.c = corrCell;
		    c2.val.c = supCell;
		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;
		}
	    }
	    prevCell = buf_corr[i + ad->x];
	}

	if (masked)
	    mask_sup = mask_corr;
    }


    /* calcolo dell'indice */
    if (area == 0)
	indice = -1;
    else {
	for (i = 0; i < totCoppie; i++) {
	    double ee = 0, dd = 0;

	    ee = (double)(cc[i]->e);	/*totedge */
	    dd = (cc[i]->d);	/*weight */
	    somma = somma + (ee * dd);
	}
	indice = somma * 10000 / area;
    }
    *result = indice;

    if (masked) {
	G_free(mask_corr);
	G_free(mask_sup);
    }

    G_free(buf_sup);
    return RLI_OK;
}


int calculateD(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	       double *result)
{

    double indice = 0;
    double somma = 0;
    double area = 0;

    int i = 0, j;
    int mask_fd = -1;
    int masked = FALSE;
    int *mask_corr, *mask_sup;

    DCELL *buf_corr, *buf_sup;
    DCELL prevCell, corrCell, supCell;

    generic_cell c1;
    generic_cell c2;



    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't  open mask");
	    return RLI_ERRORE;
	}

	mask_corr = G_malloc(ad->cl * sizeof(int));
	if (mask_corr == NULL) {
	    G_fatal_error("malloc mask_corr failed");
	    return RLI_ERRORE;
	}

	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_corr failed");
	    return RLI_ERRORE;
	}

	masked = TRUE;
    }


    buf_sup = Rast_allocate_d_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }

    c1.t = DCELL_TYPE;
    c2.t = DCELL_TYPE;

    buf_corr = Rast_allocate_d_buf();

    Rast_set_d_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */

    for (j = 0; j < ad->rl; j++) {	/* for each row */
	buf_corr = RLI_get_dcell_raster_row(fd, j + ad->y, ad);	/* read row of raster */
	if (j > 0) {		/* not first row */
	    buf_sup = RLI_get_dcell_raster_row(fd, j - 1 + ad->y, ad);
	}
	/*read mask if needed */
	if (masked) {
	    if (read(mask_fd, mask_corr, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("reading mask_corr");
		return RLI_ERRORE;
	    }
	}
	Rast_set_d_null_value(&prevCell, 1);
	Rast_set_d_null_value(&corrCell, 1);
	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    area++;
	    corrCell = buf_corr[i + ad->x];
	    if (masked && mask_corr[i + ad->x] == 0) {
		Rast_set_d_null_value(&corrCell, 1);
		area--;
	    }
	    if (!(Rast_is_null_value(&corrCell, DCELL_TYPE))) {
		supCell = buf_sup[i + ad->x];
		/* calculate how many edge the cell has */
		if (((!Rast_is_null_value(&prevCell, DCELL_TYPE))) &&
		    (corrCell != prevCell)) {
		    int r = 0;

		    c1.val.dc = corrCell;
		    c2.val.dc = prevCell;
		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;
		}


		if ((!(Rast_is_null_value(&supCell, DCELL_TYPE))) &&
		    (corrCell != supCell)) {
		    int r = 0;

		    c1.val.dc = corrCell;
		    c2.val.dc = supCell;
		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;
		}

	    }
	    prevCell = buf_corr[i + ad->x];
	}

	if (masked)
	    mask_sup = mask_corr;
    }


    /* calcolo dell'indice */
    if (area == 0)
	indice = -1;
    else {
	for (i = 0; i < totCoppie; i++) {
	    double ee, dd;

	    ee = (double)(cc[i]->e);
	    dd = (double)(cc[i]->d);
	    somma = somma + (ee * dd);
	}
	indice = somma * 10000 / area;
    }
    *result = indice;
    if (masked) {
	G_free(mask_corr);
	G_free(mask_sup);
    }
    return RLI_OK;
}



int calculateF(int fd, struct area_entry *ad, Coppie * cc, long totCoppie,
	       double *result)
{

    double indice = 0;
    double somma = 0;
    double area = 0;

    int i = 0, j;
    int mask_fd = -1;
    int masked = FALSE;
    int *mask_corr, *mask_sup;

    FCELL *buf_corr, *buf_sup;
    FCELL prevCell, corrCell, supCell;

    generic_cell c1;
    generic_cell c2;



    /* open mask if needed */
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0) {
	    G_fatal_error("can't  open mask");
	    return RLI_ERRORE;
	}

	mask_corr = G_malloc(ad->cl * sizeof(int));
	if (mask_corr == NULL) {
	    G_fatal_error("malloc mask_corr failed");
	    return RLI_ERRORE;
	}

	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_sup failed");
	    return RLI_ERRORE;
	}

	masked = TRUE;
    }

    /* allocate and inizialize buffers */
    buf_sup = Rast_allocate_f_buf();
    if (buf_sup == NULL) {
	G_fatal_error("malloc buf_sup failed");
	return RLI_ERRORE;
    }
    Rast_set_f_null_value(buf_sup + ad->x, ad->cl);	/*the first time buf_sup is all null */

    buf_corr = Rast_allocate_f_buf();
    if (buf_corr == NULL) {
	G_fatal_error("malloc buf_corr failed");
	return RLI_ERRORE;
    }

    c1.t = FCELL_TYPE;
    c2.t = FCELL_TYPE;


    for (j = 0; j < ad->rl; j++) {	/* for each row */
	buf_corr = RLI_get_fcell_raster_row(fd, j + ad->y, ad);	/* read row of raster */
	if (j > 0) {		/* not first row */
	    buf_sup = RLI_get_fcell_raster_row(fd, j - 1 + ad->y, ad);
	}
	/*read mask if needed */
	if (masked) {
	    if (read(mask_fd, mask_corr, (ad->cl * sizeof(int))) < 0) {
		G_fatal_error("reading mask_corr");
		return RLI_ERRORE;
	    }
	}
	Rast_set_f_null_value(&prevCell, 1);
	Rast_set_f_null_value(&corrCell, 1);
	for (i = 0; i < ad->cl; i++) {	/* for each cell in the row */
	    area++;
	    corrCell = buf_corr[i + ad->x];
	    if (masked && mask_corr[i + ad->x] == 0) {
		Rast_set_f_null_value(&corrCell, 1);
		area--;
	    }
	    if (!(Rast_is_null_value(&corrCell, FCELL_TYPE))) {
		supCell = buf_sup[i + ad->x];

		if (((!Rast_is_null_value(&prevCell, FCELL_TYPE))) &&
		    (corrCell != prevCell)) {
		    int r = 0;

		    c1.val.dc = corrCell;
		    c2.val.dc = prevCell;
		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;
		}


		if ((!(Rast_is_null_value(&supCell, FCELL_TYPE))) &&
		    (corrCell != supCell)) {
		    int r = 0;

		    c1.val.fc = corrCell;
		    c2.val.fc = supCell;
		    r = updateCoppia(cc, c1, c2, totCoppie);
		    if (r == RLI_ERRORE)
			return RLI_ERRORE;
		}

	    }
	    prevCell = buf_corr[i + ad->x];
	}

	if (masked)
	    mask_sup = mask_corr;
    }


    /* calcolo dell'indice */
    if (area == 0)
	indice = -1;
    else {
	for (i = 0; i < totCoppie; i++) {
	    double ee, dd;

	    ee = (double)(cc[i]->e);
	    dd = (double)(cc[i]->d);
	    somma = somma + (ee * dd);
	}
	indice = somma * 10000 / area;
    }
    *result = indice;
    if (masked) {
	G_free(mask_corr);
	G_free(mask_sup);
    }
    return RLI_OK;
}



int addCoppia(Coppie * cc, generic_cell ce1, generic_cell ce2, double pe,
	      long tc, long *siz)
{
    generic_cell cs;
    long it = 0;
    CoppiaPesata *cp = NULL;
    int ris;



    ris = equalsGenericCell(ce1, ce2);

    if (ris == GC_DIFFERENT_TYPE)
	return _ERR;
    if (ris == GC_HIGHER) {
	cs = ce2;
	ce2 = ce1;
	ce1 = cs;
    }


    switch (ce1.t) {
    case CELL_TYPE:
	{
	    if ((Rast_is_null_value(&ce1.val.c, CELL_TYPE)) ||
		(Rast_is_null_value(&ce2.val.c, CELL_TYPE)))
		return _ERR;
	    break;
	}
    case DCELL_TYPE:
	{
	    if ((Rast_is_null_value(&ce1.val.dc, DCELL_TYPE)) ||
		(Rast_is_null_value(&ce2.val.dc, DCELL_TYPE)))
		return _ERR;
	    break;
	}
    case FCELL_TYPE:
	{
	    if ((Rast_is_null_value(&ce1.val.fc, FCELL_TYPE)) ||
		(Rast_is_null_value(&ce2.val.fc, FCELL_TYPE)))
		return _ERR;
	    break;
	}
    default:
	{
	    G_fatal_error("data type unknown");
	    return _ERR;
	}
    }


    while (it < tc) {
	CoppiaPesata *cp_tmp = NULL;

	cp_tmp = cc[it];

	if ((equalsGenericCell((cp_tmp->c1), ce1) == GC_EQUAL) &&
	    (equalsGenericCell((cp_tmp->c2), ce2) == GC_EQUAL)) {

	    if ((cp_tmp->d) != pe) {
		G_warning
		    ("different weight for the same cell type. I consider right the first");
	    }

	    return _PRES;
	}

	it++;

    }
    /* if the pair of cell there isn't, add the pair */
    cp = G_malloc(sizeof(CoppiaPesata));
    if (cp == NULL) {
	G_fatal_error("malloc cp failed");
	return _ERR;
    }
    cp->c1 = ce1;
    cp->c2 = ce2;
    cp->d = pe;
    cp->e = 0;

    if (cc == NULL) {
	G_fatal_error("realloc cc failed");
	return _ERR;
    }

    cc[it] = G_malloc(sizeof(CoppiaPesata));
    if (cc[it] == NULL) {
	G_fatal_error("malloc cc[it] failed");
	return _ERR;
    }
    cc[it] = cp;


    return _ADD;
}



int updateCoppia(Coppie * cc, generic_cell c1, generic_cell c2, long tc)
{
    generic_cell cs;
    long k = 0;
    int ris;

    if (cc == NULL)
	return RLI_ERRORE;

    switch (c1.t) {
    case CELL_TYPE:
	{
	    if ((Rast_is_null_value(&(c1.val.c), CELL_TYPE)) ||
		(Rast_is_null_value(&(c2.val.c), CELL_TYPE)))
		return RLI_ERRORE;
	    break;
	}
    case DCELL_TYPE:
	{
	    if ((Rast_is_null_value(&(c1.val.dc), DCELL_TYPE)) ||
		(Rast_is_null_value(&(c2.val.dc), DCELL_TYPE)))
		return RLI_ERRORE;
	    break;
	}
    case FCELL_TYPE:
	{
	    if ((Rast_is_null_value(&(c1.val.fc), FCELL_TYPE)) ||
		(Rast_is_null_value(&(c2.val.fc), FCELL_TYPE)))
		return RLI_ERRORE;
	    break;
	}
    default:
	{
	    G_fatal_error("data type unknown");
	    return RLI_ERRORE;
	}
    }

    ris = equalsGenericCell(c1, c2);
    if (ris == GC_ERR_UNKNOWN || ris == GC_DIFFERENT_TYPE)
	return RLI_ERRORE;

    if (ris == GC_HIGHER) {
	cs = c2;
	c2 = c1;
	c1 = cs;
    }

    while (k < tc) {

	if ((equalsGenericCell((cc[k]->c1), c1) == GC_EQUAL) &&
	    (equalsGenericCell((cc[k]->c2), c2) == GC_EQUAL)) {
	    (cc[k]->e)++;
	    return RLI_OK;
	}
	else {
	    ;
	}
	k++;
    }

    return RLI_OK;
}
