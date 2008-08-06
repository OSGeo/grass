
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
#include <grass/glocale.h>
#include "daemon.h"
#include "defs.h"


#define CACHESIZE 4194304

void worker(char *raster, int f(int, char **, area_des, double *),
	    char *server_channel, char *mychannel, char **parameters)
{
    char *mapset;
    int fd, aid;
    int rec_ch, send_ch, erease_mask = 0, data_type = 0;
    int cache_rows, used = 0;
    msg toReceive, toSend;
    area_des ad;
    double result;
    int pid;
    struct Cell_head hd;
    cell_manager cm;
    dcell_manager dm;
    fcell_manager fm;

    cm = G_malloc(sizeof(struct cell_memory_entry));
    fm = G_malloc(sizeof(struct fcell_memory_entry));
    dm = G_malloc(sizeof(struct dcell_memory_entry));
    pid = getpid();
    ad = malloc(sizeof(struct area_entry));
    /* open raster map */
    mapset = G_find_cell(raster, "");
    fd = G_open_cell_old(raster, mapset);
    if (G_get_cellhd(raster, mapset, &hd) == -1) {
	G_message(_("CHILD[pid = %i] cannot open raster map"), pid);
	exit(EXIT_FAILURE);
    }
    /* read data type to allocate cache */
    data_type = G_raster_map_type(raster, mapset);
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

    /* open receive channel */
    rec_ch = open(mychannel, O_RDONLY, 0755);
    if (rec_ch == -1) {
	G_message(_("CHILD[pid = %i] cannot open receive channel"), pid);
	exit(0);
    }

    /* open send channel */
    send_ch = open(server_channel, O_WRONLY, 0755);
    if (send_ch == -1) {
	G_message(_("CHILD[pid = %i] cannot open receive channel"), pid);
	exit(0);
    }

    /* receive loop */
    receive(rec_ch, &toReceive);
    while (toReceive.type != TERM) {
	if (toReceive.type == AREA) {
	    aid = toReceive.f.f_ma.aid;
	    ad->x = toReceive.f.f_a.x;
	    ad->y = toReceive.f.f_a.y;
	    ad->rl = toReceive.f.f_a.rl;
	    ad->cl = toReceive.f.f_a.cl;
	    ad->raster = raster;
	    ad->mask = -1;
	}
	else {
	    /* toReceive.type == MASKEDAREA */

	    aid = toReceive.f.f_ma.aid;
	    ad->x = toReceive.f.f_ma.x;
	    ad->y = toReceive.f.f_ma.y;
	    ad->rl = toReceive.f.f_ma.rl;
	    ad->cl = toReceive.f.f_ma.cl;
	    ad->raster = raster;

	    /* mask preprocessing */
	    ad->mask_name = mask_preprocessing(toReceive.f.f_ma.mask,
					       raster, ad->rl, ad->cl);
	    if (ad->mask_name == NULL) {
		G_message(_("CHILD[pid = %i]: unable to open %s mask ... continue without!!!"),
			  pid, toReceive.f.f_ma.mask);
		ad->mask = -1;
	    }
	    else {
		if (strcmp(toReceive.f.f_ma.mask, ad->mask_name) != 0)
		    /* temporary mask created */
		    erease_mask = 1;
		ad->mask = open(ad->mask_name, O_WRONLY, 0755);
		if (ad->mask == -1) {
		    G_message(_("CHILD[pid = %i]: unable to open %s mask ... continue without!!!"),
			      pid, toReceive.f.f_ma.mask);
		}

	    }


	}
	/* memory menagement */
	if (ad->rl > used) {
	    /* allocate cache */
	    int i;

	    switch (data_type) {
	    case CELL_TYPE:{
		    for (i = 0; i < (ad->rl - used); i++) {
			cm->cache[used + i] = G_allocate_cell_buf();
		    }
		}
		break;
	    case DCELL_TYPE:{
		    for (i = 0; i < ad->rl - used; i++) {
			dm->cache[used + i] = G_allocate_d_raster_buf();
		    }
		}
		break;
	    case FCELL_TYPE:{
		    for (i = 0; i < ad->rl - used; i++) {
			fm->cache[used + i] = G_allocate_f_raster_buf();
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

	if (f(fd, parameters, ad, &result) == RLI_OK) {
	    /* success */
	    toSend.type = DONE;
	    toSend.f.f_d.aid = aid;
	    toSend.f.f_d.pid = getpid();
	    toSend.f.f_d.res = result;
	}
	else {
	    /* fail */
	    toSend.type = ERROR;
	    toSend.f.f_e.aid = aid;
	    toSend.f.f_e.pid = getpid();
	}

	send(send_ch, &toSend);

	if (erease_mask == 1) {
	    erease_mask = 0;
	    unlink(ad->mask_name);
	}

	receive(rec_ch, &toReceive);
    }
    /* close raster map */
    G_close_cell(fd);

    /* close channels */
    close(rec_ch);
    close(send_ch);

    return;
}

char *mask_preprocessing(char *mask, char *raster, int rl, int cl)
{
    char *mapset, *tmp_file;
    struct Cell_head cell, oldcell;
    int mask_fd, old_fd, *buf, i, j;
    CELL *old;
    double add_row, add_col;

    buf = malloc(cl * sizeof(int));
    mapset = G_find_cell(raster, "");
    /* open raster */
    if (G_get_cellhd(raster, mapset, &cell) == -1)
	return NULL;
    mapset = G_find_cell(mask, "");
    /* open raster */
    if (G_get_cellhd(mask, mapset, &oldcell) == -1)
	return NULL;

    add_row = 1.0 * oldcell.rows / rl;
    add_col = 1.0 * oldcell.cols / cl;
    tmp_file = G_tempfile();
    mask_fd = open(tmp_file, O_RDWR | O_CREAT, 0755);
    old_fd = G_open_cell_old(mask, mapset);
    old = G_allocate_cell_buf();
    for (i = 0; i < rl; i++) {
	int riga;

	riga = (int)rint(i * add_row);
	G_get_map_row_nomask(old_fd, old, riga);
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
	G_get_raster_row(fd, ad->cm->cache[hash], row, CELL_TYPE);
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
	G_get_raster_row(fd, ad->dm->cache[hash], row, DCELL_TYPE);
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
	G_get_raster_row(fd, ad->fm->cache[hash], row, FCELL_TYPE);
	ad->fm->contents[hash] = row;
	return ad->fm->cache[hash];
    }

}
