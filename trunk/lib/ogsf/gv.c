/*!
   \file lib/ogsf/gv.c

   \brief OGSF library - loading and manipulating vector sets (lower level functions)

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL (November 1993)
   \author Doxygenized by Martin Landa (June 2008)
 */

#include <stdio.h>
#include <stdlib.h>

#include <grass/ogsf.h>
#include "gsget.h"

#define FIRST_VECT_ID 20656

static geovect *Vect_top = NULL;

/*!
   \brief Get vector set

   \param id vector set id

   \return pointer to geovect struct
   \return NULL on failure
 */
geovect *gv_get_vect(int id)
{
    geovect *gv;

    G_debug(5, "gv_get_vect() id=%d", id);

    for (gv = Vect_top; gv; gv = gv->next) {
	if (gv->gvect_id == id) {
	    return gv;
	}
    }

    return NULL;
}

/*!
   \brief Get previous vector set

   \param id vector set id

   \return pointer to geovect struct
   \return NULL on failure
 */
geovect *gv_get_prev_vect(int id)
{
    geovect *pv;

    G_debug(5, "gv_get_prev_vect(): id=%d", id);

    for (pv = Vect_top; pv; pv = pv->next) {
	if (pv->gvect_id == id - 1) {
	    return pv;
	}
    }

    return NULL;
}

/*!
   \brief Get number of loaded vector sets

   \return number of vector sets
 */
int gv_num_vects(void)
{
    geovect *gv;
    int i;

    for (i = 0, gv = Vect_top; gv; gv = gv->next, i++) ;

    G_debug(5, "gv_num_vects(): num=%d", i);

    return i;
}

/*!
   \brief Get last loaded vector set

   \return pointer to geovect struct
   \return NULL on failure (no vector set available)
 */
geovect *gv_get_last_vect(void)
{
    geovect *lv;

    if (!Vect_top) {
	return NULL;
    }

    for (lv = Vect_top; lv->next; lv = lv->next) ;

    G_debug(5, "gv_get_last_vect(): id=%d", lv->gvect_id);

    return lv;
}

/*!
   \brief Allocate memory for new vector set

   \return pointer to geovect struct
   \return NULL on failure
 */
geovect *gv_get_new_vect(void)
{
    geovect *nv, *lv;

    nv = (geovect *) G_malloc(sizeof(geovect));
    if (!nv) {
	/* G_fatal_error */
	return NULL;
    }
    G_zero(nv, sizeof(geovect));

    if ((lv = gv_get_last_vect())) {
	lv->next = nv;
	nv->gvect_id = lv->gvect_id + 1;
    }
    else {
	Vect_top = nv;
	nv->gvect_id = FIRST_VECT_ID;
    }
    
    nv->style = (gvstyle *) G_malloc(sizeof(gvstyle));
    if (NULL == nv->style)
	return NULL;
    G_zero(nv->style, sizeof (gvstyle));
    nv->hstyle = (gvstyle *) G_malloc(sizeof(gvstyle));
    if (NULL == nv->hstyle)
	return NULL;
    G_zero(nv->hstyle, sizeof (gvstyle));

    G_debug(5, "gv_get_new_vect() id=%d", nv->gvect_id);

    return nv;
}

/*!
   \brief Update drape surfaces

   Call after surface is deleted
 */
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

/*!
   \brief Set attributes of vector set to default values

   \param gv pointer to geovect struct

   \return -1 on error
   \return 0 on success
 */
int gv_set_defaults(geovect * gv)
{
    int i;

    if (!gv) {
	return (-1);
    }
    G_debug(5, "gv_set_defaults() id=%d", gv->gvect_id);

    gv->filename = NULL;
    gv->n_lines = gv->n_surfs = gv->use_mem = 0;
    gv->x_trans = gv->y_trans = gv->z_trans = 0.0;
    gv->lines = NULL;
    gv->fastlines = NULL;
    gv->use_z = 0;
    gv->style->color = 0xF0F0F0;
    gv->style->width = 1;
    gv->style->next = NULL;
    gv->hstyle->color = 0xFF0000;
    gv->hstyle->width = 2;
    gv->hstyle->next = NULL;
    gv->tstyle = NULL;
    gv->next = NULL;

    for (i = 0; i < MAX_SURFS; i++) {
	gv->drape_surf_id[i] = 0;
    }

    return 0;
}

/*!
   \brief Initialize geovect struct

   \param gv pointer to geovect struct

   \return -1 on failure
   \return 0 on succcess
 */
int gv_init_vect(geovect * gv)
{
    if (!gv) {
	return -1;
    }

    G_debug(5, "gv_init_vect() id=%d", gv->gvect_id);

    return 0;
}

/*!
   \brief Delete vector set (unload)

   \param id vector set id
 */
void gv_delete_vect(int id)
{
    geovect *fv;

    G_debug(5, "gv_delete_vect(): id=%d", id);

    fv = gv_get_vect(id);

    if (fv) {
	gv_free_vect(fv);
    }

    return;
}

/*!
   \brief Free allocated memory for geovect struct

   \param fv pointer to geovect struct

   \return -1 on failure
   \return 1 on success
 */
int gv_free_vect(geovect * fv)
{
    geovect *gv;
    int found = 0;

    if (Vect_top) {
	if (fv == Vect_top) {
	    if (Vect_top->next) {
		/* can't free top if last */
		found = 1;
		Vect_top = fv->next;
	    }
	    else {
		gv_free_vectmem(fv);
		G_free(fv);
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
	    G_debug(5, "gv_free_vect(): id=%d", fv->gvect_id);
	    gv_free_vectmem(fv);
	    G_free(fv);
	    fv = NULL;
	}

	return 1;
    }

    return -1;
}

/*!
   \brief Free allocated memory

   \param fv pointer to geovect struct
 */
void gv_free_vectmem(geovect * fv)
{
    geoline *gln, *tmpln;
    
    G_free((void *)fv->filename);
    fv->filename = NULL;
    if (fv->style)
	G_free(fv->style);
    if (fv->hstyle)
	G_free(fv->hstyle);

    if (fv->lines) {
	for (gln = fv->lines; gln;) {
	    if (gln->dims == 2) {
		sub_Vectmem(gln->npts * sizeof(Point2));
		G_free(gln->p2);
	    }

	    if (gln->dims == 3) {
		G_free(gln->p3);
	    }

	    G_free(gln->cats);
	    
	    tmpln = gln;
	    gln = gln->next;
	    sub_Vectmem(sizeof(geoline));
	    G_free(tmpln);
	}

	fv->n_lines = 0;
	fv->lines = NULL;
    }

    if (fv->tstyle) {
	G_free(fv->tstyle->color_column);
	G_free(fv->tstyle->symbol_column);
	G_free(fv->tstyle->size_column);
	G_free(fv->tstyle->width_column);
    }

    return;
}

/*!
   \brief Set drape surfaces for vector set

   \param gv pointer to geovect struct
   \param hsurfs array of surfaces (id)
   \param nsurfs number of surfaces
 */
void gv_set_drapesurfs(geovect * gv, int *hsurfs, int nsurfs)
{
    int i;

    for (i = 0; i < nsurfs && i < MAX_SURFS; i++) {
	gv->drape_surf_id[i] = hsurfs[i];
    }

    return;
}
