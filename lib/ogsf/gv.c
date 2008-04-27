/*
* $Id$
*/

/*  gv.c
    Bill Brown, USACERL  
    November 1993
*/

#include <stdio.h>
#include <stdlib.h>

#include <grass/gstypes.h>
#include "gsget.h"

#define FIRST_VECT_ID 20656

/*
#define TRACE_FUNCS
*/

static geovect *Vect_top = NULL;

/***********************************************************************/
geovect *gv_get_vect(int id)
{
    geovect *gv;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_get_vect");
    }
#endif

    for (gv = Vect_top; gv; gv = gv->next) {
	if (gv->gvect_id == id) {
	    return (gv);
	}
    }

    return (NULL);
}

/***********************************************************************/
geovect *gv_get_prev_vect(int id)
{
    geovect *pv;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_get_prev_vect");
    }
#endif

    for (pv = Vect_top; pv; pv = pv->next) {
	if (pv->gvect_id == id - 1) {
	    return (pv);
	}
    }

    return (NULL);
}

/***********************************************************************/
int gv_num_vects(void)
{
    geovect *gv;
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_num_vects");
    }
#endif

    for (i = 0, gv = Vect_top; gv; gv = gv->next, i++);

    return (i);
}

/***********************************************************************/
geovect *gv_get_last_vect(void)
{
    geovect *lv;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_get_last_vect");
    }
#endif

    if (!Vect_top) {
	return (NULL);
    }

    for (lv = Vect_top; lv->next; lv = lv->next);

#ifdef DEBUG
    {
	fprintf(stderr, "last vect id: %d\n", lv->gvect_id);
    }
#endif

    return (lv);
}

/***********************************************************************/
geovect *gv_get_new_vect(void)
{
    geovect *nv, *lv;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_get_new_vect");
    }
#endif

    if (NULL == (nv = (geovect *) malloc(sizeof(geovect)))) {
	gs_err("gv_get_new_vect");

	return (NULL);
    }

    if ((lv = gv_get_last_vect())) {
	lv->next = nv;
	nv->gvect_id = lv->gvect_id + 1;
    }
    else {
	Vect_top = nv;
	nv->gvect_id = FIRST_VECT_ID;
    }

    nv->next = NULL;

    return (nv);
}

/***********************************************************************/
/* call after surface is deleted */
void gv_update_drapesurfs(void)
{
    geovect *gv;
    int i, j;

    for (gv = Vect_top; gv; gv = gv->next) {
	if (gv->n_surfs) {
	    for (i = 0; i < gv->n_surfs; i++) {
		if (gv->drape_surf_id[i]) {
		    if (NULL == gs_get_surf(gv->drape_surf_id[i])) {
			for (j = i; j < gv->n_surfs - 1; j++) {
			    gv->drape_surf_id[j] = gv->drape_surf_id[j + 1];
			}

			gv->n_surfs = gv->n_surfs - 1;
		    }
		}
	    }
	}
    }
}

/***********************************************************************/
int gv_set_defaults(geovect * gv)
{
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_set_defaults");
    }
#endif

    if (!gv) {
	return (-1);
    }

    gv->n_lines = gv->n_surfs = gv->use_mem = 0;
    gv->x_trans = gv->y_trans = gv->z_trans = 0.0;
    gv->lines = NULL;
    gv->fastlines = NULL;
    gv->width = 1;
    gv->color = 0xFFFFFF;
    gv->flat_val = 0;

    for (i = 0; i < MAX_SURFS; i++) {
	gv->drape_surf_id[i] = 0;
    }

    return (0);
}

/***********************************************************************/
int gv_init_vect(geovect * gv)
{
#ifdef TRACE_FUNCS
    {
	Gs_status("gv_init_vect");
    }
#endif

    if (!gv) {
	return (-1);
    }

    return (0);
}

/***********************************************************************/
void gv_delete_vect(int id)
{
    geovect *fv;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_delete_vect");
    }
#endif

    fv = gv_get_vect(id);

    if (fv) {
	gv_free_vect(fv);
    }

    return;
}

/***********************************************************************/
int gv_free_vect(geovect * fv)
{
    geovect *gv;
    int found = 0;

#ifdef TRACE_FUNCS
    {
	Gs_status("gv_free_vect");
    }
#endif

    if (Vect_top) {
	if (fv == Vect_top) {
	    if (Vect_top->next) {
		/* can't free top if last */
		found = 1;
		Vect_top = fv->next;
	    }
	    else {
		gv_free_vectmem(fv);
		free(fv);
		Vect_top = NULL;
	    }
	}
	else {
	    for (gv = Vect_top; gv && !found; gv = gv->next) {
		/* can't free top */
		if (gv->next) {
		    if (gv->next == fv) {
			found = 1;
			gv->next = fv->next;
		    }
		}
	    }
	}

	if (found) {
	    gv_free_vectmem(fv);
	    free(fv);
	    fv = NULL;
	}

	return (1);
    }

    return (-1);
}

/***********************************************************************/
void gv_free_vectmem(geovect * fv)
{
    geoline *gln, *tmpln;

    if (fv->lines) {
	for (gln = fv->lines; gln;) {
	    if (gln->dims == 2) {
		sub_Vectmem(gln->npts * sizeof(Point2));
		free(gln->p2);
	    }

	    if (gln->dims == 3) {
		free(gln->p3);
	    }

	    tmpln = gln;
	    gln = gln->next;
	    sub_Vectmem(sizeof(geoline));
	    free(tmpln);
	}

	fv->n_lines = 0;
	fv->lines = NULL;
    }

    show_Vectmem();

    return;
}

/***********************************************************************/
void gv_set_drapesurfs(geovect * gv, int *hsurfs, int nsurfs)
{
    int i;

    for (i = 0; i < nsurfs && i < MAX_SURFS; i++) {
	gv->drape_surf_id[i] = hsurfs[i];
    }

    return;
}
