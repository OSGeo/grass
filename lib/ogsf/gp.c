/*
* $Id$
*/

/*  gp.c
    Bill Brown, USACERL  
    January 1994
*/

#include <stdlib.h>
#include <stdio.h>

#include <grass/gstypes.h>

#define FIRST_SITE_ID 21720

static geosite *Site_top = NULL;

/***********************************************************************/
geosite *gp_get_site(int id)
{
    geosite *gp;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_get_site");
    }
#endif

    for (gp = Site_top; gp; gp = gp->next) {
	if (gp->gsite_id == id) {
	    return (gp);
	}
    }

    return (NULL);
}

/***********************************************************************/
geosite *gp_get_prev_site(int id)
{
    geosite *pp;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_get_prev_site");
    }
#endif

    for (pp = Site_top; pp; pp = pp->next) {
	if (pp->gsite_id == id - 1) {
	    return (pp);
	}
    }

    return (NULL);
}

/***********************************************************************/
int gp_num_sites(void)
{
    geosite *gp;
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_num_sites");
    }
#endif

    for (i = 0, gp = Site_top; gp; gp = gp->next, i++);

    return (i);
}

/***********************************************************************/
geosite *gp_get_last_site(void)
{
    geosite *lp;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_get_last_site");
    }
#endif

    if (!Site_top) {
	return (NULL);
    }

    for (lp = Site_top; lp->next; lp = lp->next);

#ifdef DEBUG
    {
	fprintf(stderr, "last site id: %d\n", lp->gsite_id);
    }
#endif

    return (lp);
}

/***********************************************************************/
geosite *gp_get_new_site(void)
{
    geosite *np, *lp;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_get_new_site");
    }
#endif

    if (NULL == (np = (geosite *) malloc(sizeof(geosite)))) {
	gs_err("gp_get_new_site");
	return (NULL);
    }

    if (lp = gp_get_last_site()) {
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

/***********************************************************************/
/* call after surface is deleted */
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

/***********************************************************************/
int gp_set_defaults(geosite * gp)
{
    int i;
    float dim;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_set_defaults");
    }
#endif

    if (!gp) {
	return (-1);
    }

    GS_get_longdim(&dim);

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

/***********************************************************************/
void print_site_fields(geosite * gp)
{
    int i;

    fprintf(stderr, "n_sites=%d use_z=%d n_surfs=%d use_mem=%d\n",
	    gp->n_sites, gp->use_z, gp->n_surfs, gp->use_mem);
    fprintf(stderr, "x_trans=%.2f x_trans=%.2f x_trans=%.2f\n",
	    gp->x_trans, gp->y_trans, gp->z_trans);
    fprintf(stderr, "size = %.2f\n", gp->size);
    fprintf(stderr, "points = %lx\n", (unsigned long) gp->points);
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

/***********************************************************************/
int gp_init_site(geosite * gp)
{
#ifdef TRACE_FUNCS
    {
	Gs_status("gp_init_site");
    }
#endif

    if (!gp) {
	return (-1);
    }

    return (0);
}

/***********************************************************************/
void gp_delete_site(int id)
{
    geosite *fp;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_delete_site");
    }
#endif

    fp = gp_get_site(id);

    if (fp) {
	gp_free_site(fp);
    }

    return;
}

/***********************************************************************/
int gp_free_site(geosite * fp)
{
    geosite *gp;
    int found = 0;

#ifdef TRACE_FUNCS
    {
	Gs_status("gp_free_site");
    }
#endif

    if (Site_top) {
	if (fp == Site_top) {
	    if (Site_top->next) {
		/* can't free top if last */
		found = 1;
		Site_top = fp->next;
	    }
	    else {
		gp_free_sitemem(fp);
		free(fp);
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
	    free(fp);
	    fp = NULL;
	}

	return (1);
    }

    return (-1);
}

/***********************************************************************/
void gp_free_sitemem(geosite * fp)
{
    geopoint *gpt, *tmp;

    if (fp->points) {
	for (gpt = fp->points; gpt;) {
	    if (gpt->cattr) {
		free(gpt->cattr);
	    }

	    tmp = gpt;
	    gpt = gpt->next;
	    free(tmp);
	}

	fp->n_sites = 0;
	fp->points = NULL;
    }

    return;
}

/***********************************************************************/
void gp_set_drapesurfs(geosite * gp, int hsurfs[], int nsurfs)
{
    int i;

    for (i = 0; i < nsurfs && i < MAX_SURFS; i++) {
	gp->drape_surf_id[i] = hsurfs[i];
    }

    return;
}
