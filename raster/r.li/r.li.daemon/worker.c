
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
static struct area_entry * ad;
static double result;
static struct Cell_head hd;
static cell_manager cm;
static dcell_manager dm;
static fcell_manager fm;
static char *raster;
static char **parameters;
static rli_func *func;

void worker_init(char *r, rli_func *f, char **p)
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

    /* get current window */
    Rast_get_window(&hd);

    /* read data type to allocate cache */
    data_type = Rast_map_type(raster, "");

    /* calculate rows in cache */
    switch (data_type) {
    case CELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(CELL));
	    if (cache_rows < 4)
		cache_rows = 4;
	    cm->cache = G_malloc(cache_rows * sizeof(CELL *));
	    cm->contents = G_malloc(cache_rows * sizeof(int));
	    cm->used = 0;
	    cm->contents[0] = -1;
	} break;
    case DCELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(DCELL));
	    if (cache_rows < 4)
		cache_rows = 4;
	    dm->cache = G_malloc(cache_rows * sizeof(DCELL *));
	    dm->contents = G_malloc(cache_rows * sizeof(int));
	    dm->used = 0;
	    dm->contents[0] = -1;
	} break;
    case FCELL_TYPE:{
	    cache_rows = CACHESIZE / (hd.cols * sizeof(FCELL));
	    if (cache_rows < 4)
		cache_rows = 4;
	    fm->cache = G_malloc(cache_rows * sizeof(FCELL *));
	    fm->contents = G_malloc(cache_rows * sizeof(int));
	    fm->used = 0;
	    fm->contents[0] = -1;
	} break;
    }
    ad->data_type = data_type;
    ad->rc = cache_rows;
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
	ad->mask_name = mask_preprocessing(m->f.f_ma.mask, raster, ad);

	if (ad->mask_name == NULL) {
	    G_message(_("unable to open <%s> mask ... continuing without!"),
		      m->f.f_ma.mask);
	    ad->mask = -1;
	}
	else {
	    if (strcmp(m->f.f_ma.mask, ad->mask_name) != 0)
		/* temporary mask created */
		erease_mask = 1;
	    ad->mask = 1;
	}
	break;
    default:
	G_fatal_error("Program error, worker() type=%d", m->type);
	break;
    }
    
    /* sanity check on the sample area ? */
    /* 0 <= ad->x < hd.cols */
    /* 0 <= ad->y < hd.rows */
    /* ad->rl + ad->y <= hd.rows */
    /* ad->cl + ad->x <= hd.cols */

    /* memory menagement */
    if (ad->rc > used) {
	/* allocate cache */
	int i;

	switch (data_type) {
	case CELL_TYPE:{
		for (i = 0; i < (ad->rc - used); i++) {
		    cm->cache[used + i] = Rast_allocate_c_buf();
		    cm->contents[used + i] = -1;
		}
	    }
	    break;
	case DCELL_TYPE:{
		for (i = 0; i < ad->rc - used; i++) {
		    dm->cache[used + i] = Rast_allocate_d_buf();
		    dm->contents[used + i] = -1;
		}
	    }
	    break;
	case FCELL_TYPE:{
		for (i = 0; i < ad->rc - used; i++) {
		    fm->cache[used + i] = Rast_allocate_f_buf();
		    fm->contents[used + i] = -1;
		}
	    }
	    break;
	}
	cm->used = ad->rc;
	dm->used = ad->rc;
	fm->used = ad->rc;
	used = ad->rc;
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

char *mask_preprocessing(char *mask, char *raster, struct area_entry *ad)
{
    const char *tmp_file;
    int mask_fd, old_fd, *buf, i, j;
    CELL *old;

    buf = G_malloc(ad->cl * sizeof(int));

    G_debug(3, "daemon mask preproc: raster=[%s] mask=[%s]  rl=%d cl=%d",
	    raster, mask, ad->rl, ad->cl);

    tmp_file = G_tempfile();
    mask_fd = open(tmp_file, O_RDWR | O_CREAT, 0755);
    old_fd = Rast_open_old(mask, "");
    old = Rast_allocate_c_buf();

    /* write out sample area size: ad->rl rows and ad->cl columns */

    for (i = 0; i < ad->rl; i++) {

	Rast_get_c_row_nomask(old_fd, old, i + ad->y);
	for (j = 0; j < ad->cl; j++) {

	    /* NULL -> 0, else 1 */
	    buf[j] = !Rast_is_c_null_value(&old[j + ad->x]);
	}
	if (write(mask_fd, buf, ad->cl * sizeof(int)) < 0)
	    return NULL;
    }

    close(mask_fd);
    Rast_close(old_fd);
    
    G_free(buf);
    G_free(old);
    
    return G_store(tmp_file);
}

CELL *RLI_get_cell_raster_row(int fd, int row, struct area_entry *ad)
{
    int hash;

    hash = row % ad->rc;
    if (ad->cm->contents[hash] == row)
	return ad->cm->cache[hash];
    else {
	Rast_get_row(fd, ad->cm->cache[hash], row, CELL_TYPE);
	ad->cm->contents[hash] = row;
	return ad->cm->cache[hash];
    }

}

DCELL *RLI_get_dcell_raster_row(int fd, int row, struct area_entry *ad)
{
    int hash;

    hash = row % ad->rc;
    if (ad->dm->contents[hash] == row)
	return ad->dm->cache[hash];
    else {
	Rast_get_row(fd, ad->dm->cache[hash], row, DCELL_TYPE);
	ad->dm->contents[hash] = row;
	return ad->dm->cache[hash];
    }

}

FCELL *RLI_get_fcell_raster_row(int fd, int row, struct area_entry *ad)
{
    int hash;

    hash = row % ad->rc;
    if (ad->fm->contents[hash] == row)
	return ad->fm->cache[hash];
    else {
	Rast_get_row(fd, ad->fm->cache[hash], row, FCELL_TYPE);
	ad->fm->contents[hash] = row;
	return ad->fm->cache[hash];
    }

}
