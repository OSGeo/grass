/*!
   \file lib/ogsf/gp.c

   \brief OGSF library - loading and manipulating point sets (lower level functions)

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (January 1994)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#define FIRST_SITE_ID 21720

static geosite *Site_top = NULL;

/*!
   \brief Get geosite struct

   \param id point set id

   \return pointer to geosite struct
   \return NULL on failure
 */
geosite *gp_get_site(int id)
{
    geosite *gp;

    G_debug(5, "gp_get_site(%d)", id);

    for (gp = Site_top; gp; gp = gp->next) {
	if (gp->gsite_id == id) {
	    return gp;
	}
    }

    return NULL;
}

/*!
   \brief Get previous geosite struct from list

   \param id point set id

   \return pointer to geosite struct
   \return NULL on failure
 */
geosite *gp_get_prev_site(int id)
{
    geosite *pp;

    G_debug(5, "gp_get_prev_site(%d)", id);

    for (pp = Site_top; pp; pp = pp->next) {
	if (pp->gsite_id == id - 1) {
	    return (pp);
	}
    }

    return NULL;
}

/*!
   \brief Get number of loaded point sets

   \return number of point sets
 */
int gp_num_sites(void)
{
    geosite *gp;
    int i;

    for (i = 0, gp = Site_top; gp; gp = gp->next, i++) ;

    G_debug(5, "gp_num_sites(): n=%d", i);

    return i;
}

/*!
   \brief Get last point set

   \return pointer to geosite struct
   \return NULL if no point set is available
 */
geosite *gp_get_last_site(void)
{
    geosite *lp;

    G_debug(5, "gp_get_last_site");

    if (!Site_top) {
	return NULL;
    }

    for (lp = Site_top; lp->next; lp = lp->next) ;

    G_debug(5, " last site id: %d", lp->gsite_id);

    return lp;
}

/*!
   \brief Create new geosite instance and add it to list

   \return pointer to geosite struct
   \return NULL on error
 */
geosite *gp_get_new_site(void)
{
    geosite *np, *lp;
    
    np = (geosite *) G_malloc(sizeof(geosite));	/* G_fatal_error */
    if (!np) {
	return NULL;
    }
    G_zero(np, sizeof(geosite));
    
    lp = gp_get_last_site();
    if (lp) {
	lp->next = np;
	np->gsite_id = lp->gsite_id + 1;
    }
    else {
	Site_top = np;
	np->gsite_id = FIRST_SITE_ID;
    }
    np->style = (gvstyle *) G_malloc(sizeof(gvstyle));
    if (!np->style)
	return NULL;
    G_zero(np->style, sizeof (gvstyle));
    np->hstyle = (gvstyle *) G_malloc(sizeof(gvstyle));
    if (!np->hstyle)
	return NULL;
    G_zero(np->hstyle, sizeof (gvstyle));

    G_debug(5, "gp_get_new_site id=%d", np->gsite_id);
    
    return np;
}

/*!
   \brief Update drape surfaces

   Call after surface is deleted
 */
void gp_update_drapesurfs(void)
{
    geosite *gp;
    int i, j;

    for (gp = Site_top; gp; gp = gp->next) {
	if (gp->n_surfs) {
	    for (i = 0; i < gp->n_surfs; i++) {
		if (gp->drape_surf_id[i]) {
		    if (NULL == gs_get_surf(gp->drape_surf_id[i])) {
			for (j = i; j < gp->n_surfs - 1; j++) {
			    gp->drape_surf_id[j] = gp->drape_surf_id[j + 1];
			}

			gp->n_surfs = gp->n_surfs - 1;
		    }
		}
	    }
	}
    }

    return;
}

/*!
   \brief Set default value for geosite struct

   \param gp pointer to geosite struct

   \return 1 on success
   \return -1 on failure
 */
int gp_set_defaults(geosite * gp)
{
    float dim;

    if (!gp) {
	return -1;
    }
    G_debug(5, "gp_set_defaults() id=%d", gp->gsite_id);

    GS_get_longdim(&dim);

    gp->style->color = 0xF0F0F0;
    gp->style->size = dim / 100.;
    gp->style->width = 1;
    gp->style->symbol = ST_X;
    gp->hstyle->color = 0xFF0000;
    gp->hstyle->size = dim / 150.;
    gp->hstyle->symbol = ST_X;
    gp->tstyle = NULL;

    return 1;
}

/*!
   \brief Initialize geosite struct

   \todo Currently does nothing

   \param gp pointer to geosite struct

   \return -1 on failure
   \return 0 on success
 */
int gp_init_site(geosite * gp)
{
    G_debug(5, "gp_init_site");

    if (!gp) {
	return -1;
    }

    return 0;
}

/*!
   \brief Delete point set and remove from list

   \param id point set id
 */
void gp_delete_site(int id)
{
    geosite *fp;

    G_debug(5, "gp_delete_site");

    fp = gp_get_site(id);

    if (fp) {
	gp_free_site(fp);
    }

    return;
}

/*!
   \brief Free allocated geosite struct

   \param fp pointer to geosite struct

   \return 1 on success
   \return -1 on failure
 */
int gp_free_site(geosite * fp)
{
    geosite *gp;
    int found = 0;

    G_debug(5, "gp_free_site(id=%d)", fp->gsite_id);

    if (Site_top) {
	if (fp == Site_top) {
	    if (Site_top->next) {
		/* can't free top if last */
		found = 1;
		Site_top = fp->next;
	    }
	    else {
		gp_free_sitemem(fp);
		G_free(fp);
		Site_top = NULL;
	    }
	}
	else {
	    for (gp = Site_top; gp && !found; gp = gp->next) {
		/* can't free top */
		if (gp->next) {
		    if (gp->next == fp) {
			found = 1;
			gp->next = fp->next;
		    }
		}
	    }
	}

	if (found) {
	    gp_free_sitemem(fp);
	    G_free(fp);
	    fp = NULL;
	}

	return (1);
    }

    return -1;
}

/*!
   \brief Free geosite (lower level)

   \param fp pointer to geosite struct
 */
void gp_free_sitemem(geosite * fp)
{
    geopoint *gpt, *tmp;
    
    G_free((void *)fp->filename);
    fp->filename = NULL;
    if (fp->style) {
	G_free(fp->style);
    }
    if (fp->hstyle) {
	G_free(fp->hstyle);
    }
    if (fp->points) {
	for (gpt = fp->points; gpt;) {
	    G_free(gpt->cats);
	    if(gpt->style) {
		G_free(gpt->style);
	    }
	    
	    tmp = gpt;
	    gpt = gpt->next;
	    G_free(tmp);
	}
	
	fp->n_sites = 0;
	fp->points = NULL;
    }

    if (fp->tstyle) {
	G_free(fp->tstyle->color_column);
	G_free(fp->tstyle->symbol_column);
	G_free(fp->tstyle->size_column);
	G_free(fp->tstyle->width_column);
    }
    
    return;
}

/*!
   \brief Set drape surfaces

   \param gp pointer to geosite struct
   \param hsurf list of surfaces (id)
   \param nsurf number of surfaces
 */
void gp_set_drapesurfs(geosite * gp, int hsurfs[], int nsurfs)
{
    int i;

    for (i = 0; i < nsurfs && i < MAX_SURFS; i++) {
	gp->drape_surf_id[i] = hsurfs[i];
    }

    return;
}
