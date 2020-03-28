/*!
  \file lib/raster/vrt.c
  
  \brief Raster Library - virtual GRASS raster maps.
  
  (C) 2010 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Markus Metz
*/

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "R.h"

int cmp_wnd(const void *a, const void *b)
{
    struct Cell_head *cellhda = &((struct tileinfo *) a)->cellhd;
    struct Cell_head *cellhdb = &((struct tileinfo *) b)->cellhd;

    /* sort from descending N to S, then ascending from W to E */
    if (cellhda->south > cellhdb->south)
	return -1;
    if (cellhda->south < cellhdb->south)
	return 1;
    if (cellhda->north > cellhdb->north)
	return -1;
    if (cellhda->north < cellhdb->north)
	return 1;
    if (cellhda->west < cellhdb->west)
	return -1;
    if (cellhda->west > cellhdb->west)
	return 1;
    if (cellhda->east < cellhdb->east)
	return -1;
    if (cellhda->east > cellhdb->east)
	return 1;

    return 0;
}

struct R_vrt *Rast_get_vrt(const char *vname, const char *vmapset)
{
    FILE *fp;
    int talloc, tilecount;
    struct tileinfo *ti;
    struct R_vrt *vrt;
    struct Cell_head *rd_window = &R__.rd_window;
    struct ilist *tlist;

    tilecount = 0;
    ti = NULL;

    if (!G_find_raster2(vname, vmapset))
	return NULL;

    fp = G_fopen_old_misc("cell_misc", "vrt", vname, vmapset);
    if (!fp)
	return NULL;

    tlist = G_new_ilist();
    talloc = 0;
    while (1) {
	char buf[GNAME_MAX];
	char *name;
	const char *mapset;
	struct tileinfo *p;

	if (!G_getl2(buf, sizeof(buf), fp))
	    break;

	/* Ignore empty lines */
	if (!*buf)
	    continue;

	name = buf;
	if ((mapset = G_find_raster(name, "")) == NULL)
	    G_fatal_error(_("Tile raster map <%s> not found"), name);

	if (strcmp(name, vname) == 0)
	    G_fatal_error(_("A virtual raster can not contain itself"));

	if (tilecount >= talloc) {
	    talloc += 100;
	    ti = G_realloc(ti, talloc * sizeof(struct tileinfo));
	}
	p = &ti[tilecount];

	p->name = G_store(name);
	p->mapset = G_store(mapset);
	Rast_get_cellhd(p->name, p->mapset, &(p->cellhd));
	p->clist = NULL;

	if (rd_window->proj == PROJECTION_LL) {
	    while (p->cellhd.west >= rd_window->east) {
		p->cellhd.west -= 360.0;
		p->cellhd.east -= 360.0;
	    }
	    while (p->cellhd.east <= rd_window->west) {
		p->cellhd.west += 360.0;
		p->cellhd.east += 360.0;
	    }
	}

	if (p->cellhd.north > rd_window->south && 
	    p->cellhd.south <= rd_window->north &&
	    p->cellhd.west < rd_window->east && 
	    p->cellhd.east >= rd_window->west) {
	    
	    int col;
	    double east;
	    
	    G_ilist_add(tlist, tilecount);
	    
	    p->clist = G_new_ilist();
	    for (col = 0; col < rd_window->cols; col++) {
		east = rd_window->west + rd_window->ew_res * (col + 0.5);
		
		if (rd_window->proj == PROJECTION_LL) {
		    while (east > p->cellhd.east)
			east -= 360;
		    while (east < p->cellhd.west)
			east += 360;
		}
		if (east >= p->cellhd.west && east < p->cellhd.east)
		    G_ilist_add(p->clist, col);
	    }
	}
	tilecount++;
    }

    if (tilecount > 1)
	qsort(ti, tilecount, sizeof(struct tileinfo), cmp_wnd);

    fclose(fp);
    
    vrt = G_calloc(1, sizeof(struct R_vrt));
    vrt->tilecount = tilecount;
    vrt->tileinfo = ti;
    vrt->tlist = tlist;

    return vrt;
}

void Rast_close_vrt(struct R_vrt *vrt)
{
    int i;
    
    for (i = 0; i < vrt->tilecount; i++) {
	struct tileinfo *p;

	p = &(vrt->tileinfo[i]);
	
	G_free(p->name);
	G_free(p->mapset);
	if (p->clist)
	    G_free_ilist(p->clist);
    }
    G_free(vrt->tileinfo);
    G_free_ilist(vrt->tlist);
    G_free(vrt);
}

/* must only be called by get_map_row_nomask() 
 * move to get_row.c as read_data_vrt() ? */
int Rast_get_vrt_row(int fd, void *buf, int row, RASTER_MAP_TYPE data_type)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    struct R_vrt *vrt = fcb->vrt;
    struct tileinfo *ti = vrt->tileinfo;
    struct Cell_head *rd_window = &R__.rd_window;
    double rown, rows;
    int i, j;
    int have_tile;
    void *tmpbuf;
    size_t size = Rast_cell_size(data_type);

    rown = rd_window->north - rd_window->ns_res * row;
    rows = rd_window->north - rd_window->ns_res * (row + 1);

    Rast_set_null_value(buf, rd_window->cols, data_type);
    tmpbuf = Rast_allocate_input_buf(data_type);
    have_tile = 0;

    for (i = 0; i < vrt->tlist->n_values; i++) {
	struct tileinfo *p = &ti[vrt->tlist->value[i]];

	if (p->cellhd.north > rows && p->cellhd.south <= rown) {
	    int tfd;
	    void *p1, *p2;

	    /* recurse into get_map_row(), collect data for all tiles 
	     * a mask is applied to the collected data 
	     * after this function returns */
	    Rast_set_null_value(tmpbuf, rd_window->cols, data_type);
	    /* avoid Rast__check_for_auto_masking() */
	    tfd = Rast__open_old(p->name, p->mapset);
	    Rast_get_row_nomask(tfd, tmpbuf, row, data_type);
	    Rast_unopen(tfd);
	    
	    p1 = buf;
	    p2 = tmpbuf;
	    /* restrict to start and end col ? */
	    for (j = 0; j < p->clist->n_values; j++) {
		p1 = (unsigned char *)buf + size * p->clist->value[j];
		p2 = (unsigned char *)tmpbuf + size * p->clist->value[j];
		
		if (!Rast_is_null_value(p2, data_type)) {
		    switch (data_type) {
		    case CELL_TYPE:
			*(CELL *) p1 = *(CELL *) p2;
			break;
		    case FCELL_TYPE:
			*(FCELL *) p1 = *(FCELL *) p2;
			break;
		    case DCELL_TYPE:
			*(DCELL *) p1 = *(DCELL *) p2;
			break;
		    default:
			break;
		    }
		}
	    }
	    have_tile = 1;
	}
    }
    G_free(tmpbuf);

    return have_tile;
}
