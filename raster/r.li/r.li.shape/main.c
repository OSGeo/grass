
/****************************************************************************
 *
 * MODULE:       r.li.shape
 * AUTHOR(S):    Claudio Porta and Lucio Davide Spano (original contributors)
 *                students of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Fixes: Markus Neteler <neteler itc.it>
 *               Rewrite: Markus Metz
 *
 * PURPOSE:      calculates patch number index
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
#include "../r.li.daemon/GenericCell.h"

/* type, cell count and edge count of each patch */
struct pst {
    generic_cell type;
    long cells;
    long edges;
};

rli_func shape_index;
int calculate(int fd, struct area_entry *ad, double *result);
int calculateD(int fd, struct area_entry *ad, double *result);
int calculateF(int fd, struct area_entry *ad, double *result);

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
    CELL *buf, *buf_sup, *buf_null;
    CELL corrCell, precCell, supCell;
    long npatch; 
    long pid, old_pid, new_pid, *pid_corr, *pid_sup, *ltmp;
    struct pst *pst;
    long nalloc, incr;
    int i, j, k;
    int connected;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;

    buf_null = Rast_allocate_c_buf();
    Rast_set_c_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    /* initialize patch ids */
    pid_corr = G_malloc(ad->cl * sizeof(long));
    pid_sup = G_malloc(ad->cl * sizeof(long));

    for (j = 0; j < ad->cl; j++) {
	pid_corr[j] = 0;
	pid_sup[j] = 0;
    }

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    /* calculate number of patches */
    npatch = 0;
    pid = 0;

    /* patch size and type */
    incr = 1024;
    if (incr > ad->rl)
	incr = ad->rl;
    if (incr > ad->cl)
	incr = ad->cl;
    if (incr < 2)
	incr = 2;
    nalloc = incr;
    pst = G_malloc(nalloc * sizeof(struct pst));
    for (k = 0; k < nalloc; k++) {
	pst[k].cells = 0;
	pst[k].edges = 0;
    }

    for (i = 0; i < ad->rl; i++) {
	buf = RLI_get_cell_raster_row(fd, i + ad->y, ad);
	if (i > 0) {
	    buf_sup = RLI_get_cell_raster_row(fd, i - 1 + ad->y, ad);
	}

	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	}

	ltmp = pid_sup;
	pid_sup = pid_corr;
	pid_corr = ltmp;

	Rast_set_c_null_value(&precCell, 1);

	connected = 0;
	for (j = 0; j < ad->cl; j++) {
	    pid_corr[j] = 0;
	    
	    corrCell = buf[j + ad->x];
	    if (masked && (mask_buf[j] == 0)) {
		Rast_set_c_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_c_null_value(&supCell, 1);
	    }

	    if (Rast_is_c_null_value(&corrCell)) {
		if (!Rast_is_c_null_value(&precCell))
		    pst[pid_corr[j - 1]].edges++;
		if (!Rast_is_c_null_value(&supCell))
		    pst[pid_sup[j]].edges++;
		connected = 0;
		precCell = corrCell;
		continue;
	    }

	    if (!Rast_is_c_null_value(&precCell) && corrCell == precCell) {
		pid_corr[j] = pid_corr[j - 1];
		connected = 1;
		pst[pid_corr[j]].cells++;
	    }
	    else {
		connected = 0;
	    }

	    if (!Rast_is_c_null_value(&supCell) && corrCell == supCell) {

		if (pid_corr[j] != pid_sup[j]) {
		    /* connect or merge */
		    /* after r.clump */
		    if (connected) {
			npatch--;

			if (npatch == 0) {
			    G_fatal_error("npatch == 0 at row %d, col %d", i, j);
			}
		    }

		    old_pid = pid_corr[j];
		    new_pid = pid_sup[j];
		    pid_corr[j] = new_pid;
		    if (old_pid > 0) {
			/* merge */
			/* update left side of the current row */
			for (k = 0; k < j; k++) {
			    if (pid_corr[k] == old_pid)
				pid_corr[k] = new_pid;
			}
			/* update right side of the previous row */
			for (k = j + 1; k < ad->cl; k++) {
			    if (pid_sup[k] == old_pid)
				pid_sup[k] = new_pid;
			}
			pst[new_pid].cells += pst[old_pid].cells;
			pst[old_pid].cells = 0;
			pst[new_pid].edges += pst[old_pid].edges;
			pst[old_pid].edges = 0;
			
			if (old_pid == pid)
			    pid--;
		    }
		    else {
			pst[new_pid].cells++;
		    }
		}
		connected = 1;
	    }

	    if (!connected) {
		/* start new patch */
		npatch++;
		pid++;
		pid_corr[j] = pid;

		if (pid >= nalloc) {
		    pst = (struct pst *)G_realloc(pst, (pid + incr) * sizeof(struct pst));

		    for (k = nalloc; k < pid + incr; k++) {
			pst[k].cells = 0;
			pst[k].edges = 0;
		    }
			
		    nalloc = pid + incr;
		}

		pst[pid].cells = 1;
		pst[pid].type.t = CELL_TYPE;
		pst[pid].type.val.c = corrCell;
	    }
	    /* update edge count for corr */
	    if (Rast_is_c_null_value(&precCell) || precCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (Rast_is_c_null_value(&supCell) || supCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (i == ad->rl - 1)
		pst[pid_corr[j]].edges++;
	    if (j == ad->cl - 1)
		pst[pid_corr[j]].edges++;
	    /* update edge count for prec */
	    if (!Rast_is_c_null_value(&precCell) && precCell != corrCell)
		pst[pid_corr[j - 1]].edges++;
	    /* update edge count for sup */
	    if (!Rast_is_c_null_value(&supCell) && supCell != corrCell)
		pst[pid_sup[j]].edges++;

	    precCell = corrCell;
	}
    }

    if (npatch > 0) {
	double edges, cells;

	edges = cells = 0;
	
	for (i = 1; i <= pid; i++) {
	    cells += pst[i].cells;
	    edges += pst[i].edges;
	}

	*result = 0.25 * edges / sqrt(cells);
    }
    else {
	Rast_set_d_null_value(result, 1);
    }

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);
    G_free(pid_corr);
    G_free(pid_sup);
    G_free(pst);

    return RLI_OK;
}


int calculateD(int fd, struct area_entry *ad, double *result)
{
    DCELL *buf, *buf_sup, *buf_null;
    DCELL corrCell, precCell, supCell;
    long npatch; 
    long pid, old_pid, new_pid, *pid_corr, *pid_sup, *ltmp;
    struct pst *pst;
    long nalloc, incr;
    int i, j, k;
    int connected;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;

    buf_null = Rast_allocate_d_buf();
    Rast_set_d_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    /* initialize patch ids */
    pid_corr = G_malloc(ad->cl * sizeof(long));
    pid_sup = G_malloc(ad->cl * sizeof(long));

    for (j = 0; j < ad->cl; j++) {
	pid_corr[j] = 0;
	pid_sup[j] = 0;
    }

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    /* calculate number of patches */
    npatch = 0;
    pid = 0;

    /* patch size and type */
    incr = 1024;
    if (incr > ad->rl)
	incr = ad->rl;
    if (incr > ad->cl)
	incr = ad->cl;
    if (incr < 2)
	incr = 2;
    nalloc = incr;
    pst = G_malloc(nalloc * sizeof(struct pst));
    for (k = 0; k < nalloc; k++) {
	pst[k].cells = 0;
	pst[k].edges = 0;
    }

    for (i = 0; i < ad->rl; i++) {
	buf = RLI_get_dcell_raster_row(fd, i + ad->y, ad);
	if (i > 0) {
	    buf_sup = RLI_get_dcell_raster_row(fd, i - 1 + ad->y, ad);
	}

	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	}

	ltmp = pid_sup;
	pid_sup = pid_corr;
	pid_corr = ltmp;

	Rast_set_d_null_value(&precCell, 1);

	connected = 0;
	for (j = 0; j < ad->cl; j++) {
	    pid_corr[j] = 0;
	    
	    corrCell = buf[j + ad->x];
	    if (masked && (mask_buf[j] == 0)) {
		Rast_set_d_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_d_null_value(&supCell, 1);
	    }

	    if (Rast_is_d_null_value(&corrCell)) {
		if (!Rast_is_d_null_value(&precCell))
		    pst[pid_corr[j - 1]].edges++;
		if (!Rast_is_d_null_value(&supCell))
		    pst[pid_sup[j]].edges++;
		connected = 0;
		precCell = corrCell;
		continue;
	    }

	    if (!Rast_is_d_null_value(&precCell) && corrCell == precCell) {
		pid_corr[j] = pid_corr[j - 1];
		connected = 1;
		pst[pid_corr[j]].cells++;
	    }
	    else {
		connected = 0;
	    }

	    if (!Rast_is_d_null_value(&supCell) && corrCell == supCell) {

		if (pid_corr[j] != pid_sup[j]) {
		    /* connect or merge */
		    /* after r.clump */
		    if (connected) {
			npatch--;

			if (npatch == 0) {
			    G_fatal_error("npatch == 0 at row %d, col %d", i, j);
			}
		    }

		    old_pid = pid_corr[j];
		    new_pid = pid_sup[j];
		    pid_corr[j] = new_pid;
		    if (old_pid > 0) {
			/* merge */
			/* update left side of the current row */
			for (k = 0; k < j; k++) {
			    if (pid_corr[k] == old_pid)
				pid_corr[k] = new_pid;
			}
			/* update right side of the previous row */
			for (k = j + 1; k < ad->cl; k++) {
			    if (pid_sup[k] == old_pid)
				pid_sup[k] = new_pid;
			}
			pst[new_pid].cells += pst[old_pid].cells;
			pst[old_pid].cells = 0;
			pst[new_pid].edges += pst[old_pid].edges;
			pst[old_pid].edges = 0;
			
			if (old_pid == pid)
			    pid--;
		    }
		    else {
			pst[new_pid].cells++;
		    }
		}
		connected = 1;
	    }

	    if (!connected) {
		/* start new patch */
		npatch++;
		pid++;
		pid_corr[j] = pid;

		if (pid >= nalloc) {
		    pst = (struct pst *)G_realloc(pst, (pid + incr) * sizeof(struct pst));

		    for (k = nalloc; k < pid + incr; k++) {
			pst[k].cells = 0;
			pst[k].edges = 0;
		    }
			
		    nalloc = pid + incr;
		}

		pst[pid].cells = 1;
		pst[pid].type.t = CELL_TYPE;
		pst[pid].type.val.c = corrCell;
	    }
	    /* update edge count for corr */
	    if (Rast_is_d_null_value(&precCell) || precCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (Rast_is_d_null_value(&supCell) || supCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (i == ad->rl - 1)
		pst[pid_corr[j]].edges++;
	    if (j == ad->cl - 1)
		pst[pid_corr[j]].edges++;
	    /* update edge count for prec */
	    if (!Rast_is_d_null_value(&precCell) && precCell != corrCell)
		pst[pid_corr[j - 1]].edges++;
	    /* update edge count for sup */
	    if (!Rast_is_d_null_value(&supCell) && supCell != corrCell)
		pst[pid_sup[j]].edges++;

	    precCell = corrCell;
	}
    }

    if (npatch > 0) {
	double edges, cells;

	edges = cells = 0;
	
	for (i = 1; i <= pid; i++) {
	    cells += pst[i].cells;
	    edges += pst[i].edges;
	}

	*result = 0.25 * edges / sqrt(cells);
    }
    else {
	Rast_set_d_null_value(result, 1);
    }

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);
    G_free(pid_corr);
    G_free(pid_sup);
    G_free(pst);

    return RLI_OK;
}


int calculateF(int fd, struct area_entry *ad, double *result)
{
    FCELL *buf, *buf_sup, *buf_null;
    FCELL corrCell, precCell, supCell;
    long npatch; 
    long pid, old_pid, new_pid, *pid_corr, *pid_sup, *ltmp;
    struct pst *pst;
    long nalloc, incr;
    int i, j, k;
    int connected;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;

    buf_null = Rast_allocate_f_buf();
    Rast_set_f_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    /* initialize patch ids */
    pid_corr = G_malloc(ad->cl * sizeof(long));
    pid_sup = G_malloc(ad->cl * sizeof(long));

    for (j = 0; j < ad->cl; j++) {
	pid_corr[j] = 0;
	pid_sup[j] = 0;
    }

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    /* calculate number of patches */
    npatch = 0;
    pid = 0;

    /* patch size and type */
    incr = 1024;
    if (incr > ad->rl)
	incr = ad->rl;
    if (incr > ad->cl)
	incr = ad->cl;
    if (incr < 2)
	incr = 2;
    nalloc = incr;
    pst = G_malloc(nalloc * sizeof(struct pst));
    for (k = 0; k < nalloc; k++) {
	pst[k].cells = 0;
	pst[k].edges = 0;
    }

    for (i = 0; i < ad->rl; i++) {
	buf = RLI_get_fcell_raster_row(fd, i + ad->y, ad);
	if (i > 0) {
	    buf_sup = RLI_get_fcell_raster_row(fd, i - 1 + ad->y, ad);
	}

	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return 0;
	}

	ltmp = pid_sup;
	pid_sup = pid_corr;
	pid_corr = ltmp;

	Rast_set_f_null_value(&precCell, 1);

	connected = 0;
	for (j = 0; j < ad->cl; j++) {
	    pid_corr[j] = 0;
	    
	    corrCell = buf[j + ad->x];
	    if (masked && (mask_buf[j] == 0)) {
		Rast_set_f_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_f_null_value(&supCell, 1);
	    }

	    if (Rast_is_f_null_value(&corrCell)) {
		if (!Rast_is_f_null_value(&precCell))
		    pst[pid_corr[j - 1]].edges++;
		if (!Rast_is_f_null_value(&supCell))
		    pst[pid_sup[j]].edges++;
		connected = 0;
		precCell = corrCell;
		continue;
	    }

	    if (!Rast_is_f_null_value(&precCell) && corrCell == precCell) {
		pid_corr[j] = pid_corr[j - 1];
		connected = 1;
		pst[pid_corr[j]].cells++;
	    }
	    else {
		connected = 0;
	    }

	    if (!Rast_is_f_null_value(&supCell) && corrCell == supCell) {

		if (pid_corr[j] != pid_sup[j]) {
		    /* connect or merge */
		    /* after r.clump */
		    if (connected) {
			npatch--;

			if (npatch == 0) {
			    G_fatal_error("npatch == 0 at row %d, col %d", i, j);
			}
		    }

		    old_pid = pid_corr[j];
		    new_pid = pid_sup[j];
		    pid_corr[j] = new_pid;
		    if (old_pid > 0) {
			/* merge */
			/* update left side of the current row */
			for (k = 0; k < j; k++) {
			    if (pid_corr[k] == old_pid)
				pid_corr[k] = new_pid;
			}
			/* update right side of the previous row */
			for (k = j + 1; k < ad->cl; k++) {
			    if (pid_sup[k] == old_pid)
				pid_sup[k] = new_pid;
			}
			pst[new_pid].cells += pst[old_pid].cells;
			pst[old_pid].cells = 0;
			pst[new_pid].edges += pst[old_pid].edges;
			pst[old_pid].edges = 0;
			
			if (old_pid == pid)
			    pid--;
		    }
		    else {
			pst[new_pid].cells++;
		    }
		}
		connected = 1;
	    }

	    if (!connected) {
		/* start new patch */
		npatch++;
		pid++;
		pid_corr[j] = pid;

		if (pid >= nalloc) {
		    pst = (struct pst *)G_realloc(pst, (pid + incr) * sizeof(struct pst));

		    for (k = nalloc; k < pid + incr; k++) {
			pst[k].cells = 0;
			pst[k].edges = 0;
		    }
			
		    nalloc = pid + incr;
		}

		pst[pid].cells = 1;
		pst[pid].type.t = CELL_TYPE;
		pst[pid].type.val.c = corrCell;
	    }
	    /* update edge count for corr */
	    if (Rast_is_f_null_value(&precCell) || precCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (Rast_is_f_null_value(&supCell) || supCell != corrCell)
		pst[pid_corr[j]].edges++;
	    if (i == ad->rl - 1)
		pst[pid_corr[j]].edges++;
	    if (j == ad->cl - 1)
		pst[pid_corr[j]].edges++;
	    /* update edge count for prec */
	    if (!Rast_is_f_null_value(&precCell) && precCell != corrCell)
		pst[pid_corr[j - 1]].edges++;
	    /* update edge count for sup */
	    if (!Rast_is_f_null_value(&supCell) && supCell != corrCell)
		pst[pid_sup[j]].edges++;

	    precCell = corrCell;
	}
    }

    if (npatch > 0) {
	double edges, cells;

	edges = cells = 0;
	
	for (i = 1; i <= pid; i++) {
	    cells += pst[i].cells;
	    edges += pst[i].edges;
	}

	*result = 0.25 * edges / sqrt(cells);
    }
    else {
	Rast_set_d_null_value(result, 1);
    }

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);
    G_free(pid_corr);
    G_free(pid_sup);
    G_free(pst);

    return RLI_OK;
}
