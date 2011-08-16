
/**
 * \file worker.c
 *
 * \brief Implementation of the client for parallel
 * computing of r.li raster analysis
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \author Claudio Porta & Lucio Davide Spano
 * 
 * \version 1.0
 * 
 */

#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "daemon.h"
#include "defs.h"


#define CACHESIZE 4194304

static int fd, aid;
static int erease_mask = 0, data_type = 0;
static int cache_rows, used = 0;
static area_des ad;
static double result;
static struct Cell_head hd;
static cell_manager cm;
static dcell_manager dm;
static fcell_manager fm;
static char *raster;
static char **parameters;
static int (*func) (int, char **, area_des, double *);

void worker_init(char *r, int f(int, char **, area_des, double *), char **p)
{
    cm = G_malloc(sizeof(struct cell_memory_entry));
    fm = G_malloc(sizeof(struct fcell_memory_entry));
    dm = G_malloc(sizeof(struct dcell_memory_entry));
    ad = G_malloc(sizeof(struct area_entry));

    raster = r;
    parameters = p;
    func = f;

    /* open raster map */
    fd = Rast_open_old(raster, "");
    Rast_get_cellhd(raster, "", &hd);

    /* read data type to allocate cache */
    data_type = Rast_map_type(raster, "");
    /* calculate rows in cache */
    switch (data_type) {
    case CELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(CELL));
	    cm->cache = G_malloc(cache_rows * sizeof(CELL *));
	    cm->contents = G_malloc(cache_rows * sizeof(int));
	    cm->used = 0;
	    cm->contents[0] = -1;
	} break;
    case DCELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(DCELL));
	    dm->cache = G_malloc(cache_rows * sizeof(DCELL *));
	    dm->contents = G_malloc(cache_rows * sizeof(int));
	    dm->used = 0;
	    dm->contents[0] = -1;
	} break;
    case FCELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(FCELL));
	    fm->cache = G_malloc(cache_rows * sizeof(FCELL *));
	    fm->contents = G_malloc(cache_rows * sizeof(int));
	    fm->used = 0;
	    fm->contents[0] = -1;
	} break;
    }
    ad->data_type = data_type;
    ad->cm = cm;
    ad->fm = fm;
    ad->dm = dm;
}

void worker_process(msg * ret, msg * m)
{
    switch (m->type) {
    case AREA:
	aid = m->f.f_a.aid;
	ad->x = m->f.f_a.x;
	ad->y = m->f.f_a.y;
	ad->rl = m->f.f_a.rl;
	ad->cl = m->f.f_a.cl;
	ad->raster = raster;
	ad->mask = -1;
	break;
    case MASKEDAREA:
	aid = m->f.f_ma.aid;
	ad->x = m->f.f_ma.x;
	ad->y = m->f.f_ma.y;
	ad->rl = m->f.f_ma.rl;
	ad->cl = m->f.f_ma.cl;
	ad->raster = raster;

	/* mask preprocessing */
	ad->mask_name = mask_preprocessing(m->f.f_ma.mask,
					   raster, ad->rl, ad->cl);
	if (ad->mask_name == NULL) {
	    G_message(_("unable to open <%s> mask ... continuing without!"),
		      m->f.f_ma.mask);
	    ad->mask = -1;
	}
	else {
	    if (strcmp(m->f.f_ma.mask, ad->mask_name) != 0)
		/* temporary mask created */
		erease_mask = 1;
	    ad->mask = open(ad->mask_name, O_WRONLY, 0755);
	    if (ad->mask == -1) {
		G_message(_("unable to open <%s> mask ... continuing without!"),
			  m->f.f_ma.mask);
	    }
	}
	break;
    default:
	G_fatal_error("Program error, worker() type=%d", m->type);
	break;
    }

    /* memory menagement */
    if (ad->rl > used) {
	/* allocate cache */
	int i;

	switch (data_type) {
	case CELL_TYPE:{
		for (i = 0; i < (ad->rl - used); i++) {
		    cm->cache[used + i] = Rast_allocate_c_buf();
		}
	    }
	    break;
	case DCELL_TYPE:{
		for (i = 0; i < ad->rl - used; i++) {
		    dm->cache[used + i] = Rast_allocate_d_buf();
		}
	    }
	    break;
	case FCELL_TYPE:{
		for (i = 0; i < ad->rl - used; i++) {
		    fm->cache[used + i] = Rast_allocate_f_buf();
		}
	    }
	    break;
	}
	cm->used = ad->rl;
	dm->used = ad->rl;
	fm->used = ad->rl;
	used = ad->rl;
    }

    /* calculate function */

    if (func(fd, parameters, ad, &result) == RLI_OK) {
	/* success */
	ret->type = DONE;
	ret->f.f_d.aid = aid;
	ret->f.f_d.pid = 0;
	ret->f.f_d.res = result;
    }
    else {
	/* fail */
	ret->type = ERROR;
	ret->f.f_e.aid = aid;
	ret->f.f_e.pid = 0;
    }

    if (erease_mask == 1) {
	erease_mask = 0;
	unlink(ad->mask_name);
    }
}

void worker_end(void)
{
    /* close raster map */
    Rast_close(fd);
}

char *mask_preprocessing(char *mask, char *raster, int rl, int cl)
{
    const char *tmp_file;
    struct Cell_head cell, oldcell;
    int mask_fd, old_fd, *buf, i, j;
    CELL *old;
    double add_row, add_col;

    buf = G_malloc(cl * sizeof(int));

    G_debug(3, "daemon mask preproc: raster=[%s] mask=[%s]  rl=%d cl=%d",
	    raster, mask, rl, cl);

    /* open raster */
    Rast_get_cellhd(raster, "", &cell);

    /* open raster */
    Rast_get_cellhd(mask, "", &oldcell);

    add_row = 1.0 * oldcell.rows / rl;
    add_col = 1.0 * oldcell.cols / cl;

    tmp_file = G_tempfile();
    mask_fd = open(tmp_file, O_RDWR | O_CREAT, 0755);
    old_fd = Rast_open_old(mask, "");
    old = Rast_allocate_c_buf();

    for (i = 0; i < rl; i++) {
	int riga;

	riga = (int)rint(i * add_row);
	Rast_get_c_row_nomask(old_fd, old, riga);
	for (j = 0; j < cl; j++) {
	    int colonna;

	    colonna = (int)rint(j * add_col);
	    buf[j] = old[colonna];
	}
	if (write(mask_fd, buf, cl * sizeof(int)) < 0)
	    return NULL;
    }
    close(mask_fd);
    return G_store(tmp_file);
}

CELL *RLI_get_cell_raster_row(int fd, int row, area_des ad)
{
    int hash;

    hash = row % ad->rl;
    if (ad->cm->contents[hash] == row)
	return ad->cm->cache[hash];
    else {
	Rast_get_row(fd, ad->cm->cache[hash], row, CELL_TYPE);
	ad->cm->contents[hash] = row;
	return ad->cm->cache[hash];
    }

}

DCELL *RLI_get_dcell_raster_row(int fd, int row, area_des ad)
{
    int hash;

    hash = row % ad->rl;
    if (ad->dm->contents[hash] == row)
	return ad->dm->cache[hash];
    else {
	Rast_get_row(fd, ad->dm->cache[hash], row, DCELL_TYPE);
	ad->dm->contents[hash] = row;
	return ad->dm->cache[hash];
    }

}

FCELL *RLI_get_fcell_raster_row(int fd, int row, area_des ad)
{
    int hash;

    hash = row % ad->rl;
    if (ad->fm->contents[hash] == row)
	return ad->fm->cache[hash];
    else {
	Rast_get_row(fd, ad->fm->cache[hash], row, FCELL_TYPE);
	ad->fm->contents[hash] = row;
	return ad->fm->cache[hash];
    }

}
