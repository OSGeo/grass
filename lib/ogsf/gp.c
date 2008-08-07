/*!
   \file gp.c

   \brief OGSF library - loading and manipulating point sets

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (January 1994)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/gstypes.h>

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

    G_debug(5, "gp_get_site");

    for (gp = Site_top; gp; gp = gp->next) {
	if (gp->gsite_id == id) {
	    return (gp);
	}
    }

    return (NULL);
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

    G_debug(5, "gp_get_prev_site");

    for (pp = Site_top; pp; pp = pp->next) {
	if (pp->gsite_id == id - 1) {
	    return (pp);
	}
    }

    return (NULL);
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

    return (i);
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
	return (NULL);
    }

    for (lp = Site_top; lp->next; lp = lp->next) ;

    G_debug(5, " last site id: %d", lp->gsite_id);

    return (lp);
}

/*!
   \brief Create new geosite instance and add it to list

   \return pointer to geosite struct
   \return NULL on error
 */
geosite *gp_get_new_site(void)
{
    geosite *np, *lp;

    G_debug(5, "gp_get_new_site");

    np = (geosite *) G_malloc(sizeof(geosite));	/* G_fatal_error */
    if (!np) {
	return (NULL);
    }

    lp = gp_get_last_site();
    if (lp) {
	lp->next = np;
	np->gsite_id = lp->gsite_id + 1;
    }
    else {
	Site_top = np;
	np->gsite_id = FIRST_SITE_ID;
    }

    np->next = NULL;

    return (np);
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
    int i;
    float dim;

    G_debug(5, "gp_set_defaults");

    if (!gp) {
	return (-1);
    }

    GS_get_longdim(&dim);

    gp->filename = NULL;
    gp->n_sites = gp->use_z = gp->n_surfs = gp->use_mem = 0;
    gp->x_trans = gp->y_trans = gp->z_trans = 0.0;
    gp->size = dim / 100.;
    gp->points = NULL;
    gp->width = 1;
    gp->color = 0xFFFFFF;
    gp->marker = ST_X;
    gp->has_z = gp->has_att = 0;
    gp->attr_mode = ST_ATT_NONE;
    gp->next = NULL;
    for (i = 0; i < MAX_SURFS; i++) {
	gp->drape_surf_id[i] = 0;
    }

    return (1);
}

/*!
   \brief Print point set fields, debugging

   \param gp pointer to geosite struct
 */
void print_site_fields(geosite * gp)
{
    int i;

    fprintf(stderr, "n_sites=%d use_z=%d n_surfs=%d use_mem=%d\n",
	    gp->n_sites, gp->use_z, gp->n_surfs, gp->use_mem);
    fprintf(stderr, "x_trans=%.2f x_trans=%.2f x_trans=%.2f\n",
	    gp->x_trans, gp->y_trans, gp->z_trans);
    fprintf(stderr, "size = %.2f\n", gp->size);
    fprintf(stderr, "points = %lx\n", (unsigned long)gp->points);
    fprintf(stderr, "width = %d\n", gp->width);
    fprintf(stderr, "color = %x\n", gp->color);
    fprintf(stderr, "marker = %d\n", gp->marker);
    fprintf(stderr, "has_z = %d, has_att = %d\n", gp->has_z, gp->has_att);
    fprintf(stderr, "attr_mode = %d\n", gp->attr_mode);

    for (i = 0; i < MAX_SURFS; i++) {
	fprintf(stderr, "drape_surf_id[%d] = %d\n", i, gp->drape_surf_id[i]);
    }

    return;
}

/*!
   \brief Initialize geosite struct

   \param gp pointer to geosite struct

   \return -1 on failure
   \return 0 on success
 */
int gp_init_site(geosite * gp)
{
    G_debug(5, "gp_init_site");

    if (!gp) {
	return (-1);
    }

    return (0);
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
   \brief Free geosite struct

   \param fp pointer to geosite struct

   \return 1 on success
   \return -1 on failure
 */
int gp_free_site(geosite * fp)
{
    geosite *gp;
    int found = 0;

    G_debug(5, "gp_free_site");

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

    return (-1);
}

/*!
   \brief Free geosite

   \param fp pointer to geosite struct
 */
void gp_free_sitemem(geosite * fp)
{
    geopoint *gpt, *tmp;

    G_free((void *)fp->filename);
    fp->filename = NULL;
    if (fp->points) {
	for (gpt = fp->points; gpt;) {
	    if (gpt->cattr) {
		G_free(gpt->cattr);
	    }

	    tmp = gpt;
	    gpt = gpt->next;
	    G_free(tmp);
	}

	fp->n_sites = 0;
	fp->points = NULL;
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
