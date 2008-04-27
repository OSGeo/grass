/*
* $Id$
*/

/*  GV.c 
    Bill Brown, USACERL  
    October 1993
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gstypes.h>
#include "gsget.h"

#ifdef TRACE_FUNCS
#define TRACE_GV_FUNCS
#endif

static int Vect_ID[MAX_VECTS];
static int Next_vect = 0;

/***********************************************************************/
int GV_vect_exists(int id)
{
    int i, found = 0;

#ifdef TRACE_GV_FUNCS
    {
	Gs_status("GV_vect_exists");
    }
#endif

    if (NULL == gv_get_vect(id)) {
	return (0);
    }

    for (i = 0; i < Next_vect && !found; i++) {
	if (Vect_ID[i] == id) {
	    found = 1;
	}
    }

    return (found);
}

/***********************************************************************/
int GV_new_vector(void)
{
    geovect *nv;

#ifdef TRACE_GV_FUNCS
    {
	Gs_status("GV_new_vector");
    }
#endif

    if (Next_vect < MAX_VECTS) {
	nv = gv_get_new_vect();
	gv_set_defaults(nv);
	Vect_ID[Next_vect] = nv->gvect_id;
	++Next_vect;

	return (nv->gvect_id);
    }

    return (-1);
}

/***********************************************************************/
int GV_num_vects(void)
{
    return (gv_num_vects());
}

/***********************************************************************/
/* USER must free!! */
int *GV_get_vect_list(int *numvects)
{
    int i, *ret;

    *numvects = Next_vect;

    if (Next_vect) {
	if (NULL == (ret = (int *) malloc(Next_vect * sizeof(int)))) {
	    fprintf(stderr, "can't malloc\n");

	    return (NULL);
	}

	for (i = 0; i < Next_vect; i++) {
	    ret[i] = Vect_ID[i];
	}

	return (ret);
    }

    return (NULL);
}

/***********************************************************************/
int GV_delete_vector(int id)
{
    int i, j, found = 0;

#ifdef TRACE_GV_FUNCS
    {
	Gs_status("GV_delete_vect");
    }
#endif

    if (GV_vect_exists(id)) {
	gv_delete_vect(id);

	for (i = 0; i < Next_vect && !found; i++) {
	    if (Vect_ID[i] == id) {
		found = 1;

		for (j = i; j < Next_vect; j++) {
		    Vect_ID[j] = Vect_ID[j + 1];
		}
	    }
	}

	if (found) {
	    --Next_vect;
	    return (1);
	}
    }

    return (-1);
}

/***********************************************************************/
int GV_load_vector(int id, char *filename)
{
    geovect *gv;

    /* check to see if handle already loaded, if so - free before loading */
    /* new for now, always load to memory */
    /* TODO SOON: load file handle & ready for reading instead of using */
    /* memory */
    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    if (gv->lines) {
	gv_free_vectmem(gv);
    }

    if (NAME_SIZ > strlen(filename)) {
	strcpy(gv->filename, filename);
    }

    if ((gv->lines = Gv_load_vect(filename, &(gv->n_lines)))) {
	return (1);
    }

    return (-1);
}

/***********************************************************************/
int GV_get_vectname(int id, char *filename)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    strcpy(filename, gv->filename);

    return (1);
}

/***********************************************************************/
int GV_set_vectmode(int id, int mem, int color, int width, int flat)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    gv->use_mem = mem;
    gv->color = color;
    gv->width = width;
    gv->flat_val = flat;

    return (1);
}

/***********************************************************************/
int GV_get_vectmode(int id, int *mem, int *color, int *width, int *flat)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    *mem = gv->use_mem;
    *color = gv->color;
    *width = gv->width;
    *flat = gv->flat_val;

    return (1);
}

/***********************************************************************/
void GV_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geovect *gv;

#ifdef TRACE_GV_FUNCS
    {
	Gs_status("GV_set_trans");
    }
#endif

    gv = gv_get_vect(id);

    if (gv) {
	gv->x_trans = xtrans;
	gv->y_trans = ytrans;
	gv->z_trans = ztrans;
    }

    return;
}

/***********************************************************************/
int GV_get_trans(int id, float *xtrans, float *ytrans, float *ztrans)
{
    geovect *gv;

    gv = gv_get_vect(id);

    if (gv) {
	*xtrans = gv->x_trans;
	*ytrans = gv->y_trans;
	*ztrans = gv->z_trans;

	return (1);
    }

    return (-1);
}

/***********************************************************************/
int GV_select_surf(int hv, int hs)
{
    geovect *gv;

    if (GV_surf_is_selected(hv, hs)) {
	return (1);
    }

    gv = gv_get_vect(hv);

    if (gv && GS_surf_exists(hs)) {
	gv->drape_surf_id[gv->n_surfs] = hs;
	gv->n_surfs += 1;

	return (1);
    }

    return (-1);
}

/***********************************************************************/
int GV_unselect_surf(int hv, int hs)
{
    geovect *gv;
    int i, j;

    if (!GV_surf_is_selected(hv, hs)) {
	return (1);
    }

    gv = gv_get_vect(hv);

    if (gv) {
	for (i = 0; i < gv->n_surfs; i++) {
	    if (gv->drape_surf_id[i] == hs) {
		for (j = i; j < gv->n_surfs - 1; j++) {
		    gv->drape_surf_id[j] = gv->drape_surf_id[j + 1];
		}

		gv->n_surfs -= 1;

		return (1);
	    }
	}
    }

    return (-1);
}

/***********************************************************************/
int GV_surf_is_selected(int hv, int hs)
{
    int i;
    geovect *gv;

    gv = gv_get_vect(hv);

    if (gv) {
	for (i = 0; i < gv->n_surfs; i++) {
	    if (hs == gv->drape_surf_id[i]) {
		return (1);
	    }
	}
    }

    return (0);
}

/***********************************************************************/
void GV_draw_vect(int vid)
{
    geosurf *gs;
    geovect *gv;
    int i;

    gv = gv_get_vect(vid);

    if (gv) {
	for (i = 0; i < gv->n_surfs; i++) {
	    gs = gs_get_surf(gv->drape_surf_id[i]);

	    if (gs) {
		gvd_vect(gv, gs, 0);
	    }
	}
    }

    return;
}

/***********************************************************************/
void GV_alldraw_vect(void)
{
    int id;

    for (id = 0; id < Next_vect; id++) {
	GV_draw_vect(Vect_ID[id]);
    }

    return;
}

/***********************************************************************/
void GV_draw_fastvect(int vid)
{
    geosurf *gs;
    geovect *gv;
    int i;

    gv = gv_get_vect(vid);

    if (gv) {
	for (i = 0; i < gv->n_surfs; i++) {
	    gs = gs_get_surf(gv->drape_surf_id[i]);

	    if (gs) {
		gvd_vect(gv, gs, 1);
	    }
	}
    }

    return;
}

/***********************************************************************/
int GV_Set_ClientData(int id, void *clientd)
{
    geovect *gv;

    gv = gv_get_vect(id);
    if (gv) {
	gv->clientdata = clientd;

	return (1);
    }

    return (-1);
}

/***********************************************************************/
void *GV_Get_ClientData(int id)
{
    geovect *gv;

    gv = gv_get_vect(id);

    if (gv) {
	return (gv->clientdata);
    }

    return (NULL);
}
