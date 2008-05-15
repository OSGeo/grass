/*!
  \file GP2.c
 
  \brief OGSF library - loading and manipulating point sets (higher level functions)
 
  GRASS OpenGL gsurf OGSF Library 
 
  (C) 1999-2008 by the GRASS Development Team
 
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Bill Brown USACERL (January 1994)
  \author Doxygenized by Martin landa <landa.martin gmail.com> (May 2008)
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/gstypes.h>

#include "gsget.h"

static int Site_ID[MAX_SITES];
static int Next_site = 0;

/*!
  \brief Check if point set exists

  \param id point set id
  
  \return 1 found
  \return 0 not found
*/
int GP_site_exists(int id)
{
    int i, found = 0;

#ifdef TRACE_GP_FUNCS
    {
	Gs_status("GP_site_exists");
    }
#endif

    if (NULL == gp_get_site(id)) {
	return (0);
    }

    for (i = 0; i < Next_site && !found; i++) {
	if (Site_ID[i] == id) {
	    found = 1;
	}
    }

    return (found);
}

/*!
  \brief Register new point set

  \return point set id
  \return -1 on error (number of point sets exceeded)
*/
int GP_new_site(void)
{
    geosite *np;

#ifdef TRACE_GP_FUNCS
    {
	Gs_status("GP_new_site");
    }
#endif

    if (Next_site < MAX_SITES) {
	np = gp_get_new_site();
	gp_set_defaults(np);
	Site_ID[Next_site] = np->gsite_id;
	++Next_site;
	return (np->gsite_id);
    }

    return (-1);
}

/*!
  \brief Get number of available point sets

  \return number of point sets
*/
int GP_num_sites(void)
{
    return (gp_num_sites());
}

/*!
  \brief Get list of point sets
  
  Must freed when no longer needed!

  \param numsites number of point sets

  \return pointer to list of points sets
  \return NULL on error
*/
int *GP_get_site_list(int *numsites)
{
    int i, *ret;

    *numsites = Next_site;

    if (Next_site) {
	ret = (int *) G_malloc(Next_site * sizeof(int)); /* G_fatal_error */
	if (!ret) {
	    return (NULL);
	}

	for (i = 0; i < Next_site; i++) {
	    ret[i] = Site_ID[i];
	}

	return (ret);
    }

    return (NULL);
}

/*!
  \brief Delete registrated point set

  \param id point set id

  \return 1 on success
  \return -1 on error (point sets not available)
*/
int GP_delete_site(int id)
{
    int i, j, found = 0;

#ifdef TRACE_GP_FUNCS
    {
	Gs_status("GP_delete_site");
    }
#endif

    if (GP_site_exists(id)) {
	gp_delete_site(id);

	for (i = 0; i < Next_site && !found; i++) {
	    if (Site_ID[i] == id) {
		found = 1;
		for (j = i; j < Next_site; j++) {
		    Site_ID[j] = Site_ID[j + 1];
		}
	    }
	}

	if (found) {
	    --Next_site;
	    return (1);
	}
    }

    return (-1);
}

/*!
  \brief Load point set from file

  Check to see if handle already loaded, if so - free before loading new 
  for now, always load to memory 
  
  \todo load file handle & ready for reading instead of using
  memory

  \param id point set id
  \param filename point set filename

  \return -1 on error
  \return 1 on success
*/
int GP_load_site(int id, char *filename)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    if (gp->points) {
	gp_free_sitemem(gp);
    }

    if (NAME_SIZ > strlen(filename)) {
	strcpy(gp->filename, filename);
    }

    if (gp->points = Gp_load_sites(filename, &(gp->n_sites),
				   &(gp->has_z), &(gp->has_att))) {
	return (1);
    }

    return (-1);
}

/*!
  \brief Get point set filename

  \param id point set id
  \param[out] filename point set filename

  \return -1 on error (point set not found)
  \return 1 on success
*/
int GP_get_sitename(int id, char *filename)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    strcpy(filename, gp->filename);

    return (1);
}

/*!
  \brief Get point set mode
*/
int GP_get_sitemode(int id, int *atmod, int *color, int *width, float *size,
		    int *marker)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    *atmod = gp->attr_mode;
    *color = gp->color;
    *width = gp->width;
    *marker = gp->marker;
    *size = gp->size;

    return (1);
}

/*!
  \brief Set mode for point set

  \param id point set id
  \param atmod
  \param color color value
  \param width 
  \param size point size
  \param marker marker symbol

  \return -1 on error (point set not found)
*/
int GP_set_sitemode(int id, int atmod, int color, int width, float size,
		    int marker)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    gp->attr_mode = atmod;	/* FIX this - probably should be seperate */
    gp->color = color;
    gp->width = width;
    gp->marker = marker;
    gp->size = size;

    return (1);
}

/*!
  \brief Set attribute mode color

  \todo make similar routines for attmode_size, attmode_marker (use transform)
  
  \param id surface id
  \param filename filename

  \return 1 for success
  \return 0 for no attribute info
  \return -1 for bad parameter
*/
int GP_attmode_color(int id, char *filename)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    if (!gp->has_att) {
	return (0);
    }

    if (Gp_set_color(filename, gp->points)) {
	gp->attr_mode = ST_ATT_COLOR;
	return (1);
    }

    return (-1);
}

/*!
  \brief Set attribute mode to none

  \param id point set id

  \return -1 on error (invalid point set id)
  \return 1 on success
*/
int GP_attmode_none(int id)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    gp->attr_mode = ST_ATT_NONE;

    return (1);
}

/*!
  \brief Set z-mode

  \param id poin set id
  \param use_z use z ?

  \return 1 on success
  \return 0 no z
  \return -1 on error (invalid point set id)
*/
int GP_set_zmode(int id, int use_z)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    if (use_z) {
	if (gp->has_z) {
	    gp->use_z = 1;
	    return (1);
	}

	return (0);
    }

    gp->use_z = 0;
    return (1);
}

/*!
  \brief Get z-mode

  \param id point set id
  \param[out] use_z use z
  
  \return -1 on error (invalid point set id)
  \return 1 on success
*/
int GP_get_zmode(int id, int *use_z)
{
    geosite *gp;

    if (NULL == (gp = gp_get_site(id))) {
	return (-1);
    }

    *use_z = gp->use_z;
    return (1);
}

/*!
  \brief Set trans ?

  \param id point set id
  \param xtrans,ytrans,ztrans x/y/z trans values
*/
void GP_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geosite *gp;

#ifdef TRACE_GP_FUNCS
    {
	Gs_status("GP_set_trans");
    }
#endif

    gp = gp_get_site(id);
    if (gp) {
	gp->x_trans = xtrans;
	gp->y_trans = ytrans;
	gp->z_trans = ztrans;
    }

    return;
}

/*!
  \brief Get trans

  \param id point set id
  \param xtrans,ytrans,ztrans x/y/z trans values
*/
void GP_get_trans(int id, float *xtrans, float *ytrans, float *ztrans)
{
    geosite *gp;

#ifdef TRACE_GP_FUNCS
    {
	Gs_status("GP_get_trans");
    }
#endif

    gp = gp_get_site(id);

    if (gp) {
	*xtrans = gp->x_trans;
	*ytrans = gp->y_trans;
	*ztrans = gp->z_trans;
    }

    return;
}

/*!
  \brief Select surface

  \param hp point set id
  \param hs surface id

  \return 1 surface selected
  \return -1 on error
*/
int GP_select_surf(int hp, int hs)
{
    geosite *gp;

    if (GP_surf_is_selected(hp, hs)) {
	return (1);
    }

    gp = gp_get_site(hp);

    if (gp && GS_surf_exists(hs)) {
	gp->drape_surf_id[gp->n_surfs] = hs;
	gp->n_surfs += 1;
	return (1);
    }

    return (-1);
}

/*!
  \brief Unselect surface

  \param hp point set id
  \param hs surface id

  \return 1 surface unselected
  \return -1 on error
*/
int GP_unselect_surf(int hp, int hs)
{
    geosite *gp;
    int i, j;

    if (!GP_surf_is_selected(hp, hs)) {
	return (1);
    }

    gp = gp_get_site(hp);

    if (gp) {
	for (i = 0; i < gp->n_surfs; i++) {
	    if (gp->drape_surf_id[i] == hs) {
		for (j = i; j < gp->n_surfs - 1; j++) {
		    gp->drape_surf_id[j] = gp->drape_surf_id[j + 1];
		}

		gp->n_surfs -= 1;
		return (1);
	    }
	}
    }

    return (-1);
}

/*!
  \brief Check if surface is selected

  \param hp point set id
  \param hs surface id

  \return 1 selected
  \return 0 not selected
*/
int GP_surf_is_selected(int hp, int hs)
{
    int i;
    geosite *gp;

    gp = gp_get_site(hp);

    if (gp) {
	for (i = 0; i < gp->n_surfs; i++) {
	    if (hs == gp->drape_surf_id[i]) {
		return (1);
	    }
	}
    }

    return (0);
}

/*!
  \brief Draw point set

  \param id point set id
*/
void GP_draw_site(int id)
{
    geosurf *gs;
    geosite *gp;
    int i;
    float n, yo, xo, e;

    gp = gp_get_site(id);
    GS_get_region(&n, &yo, &xo, &e);

    /* kind of sloppy - maybe site files should have an origin, too */
    if (gp) {
	if (gp->use_z && gp->has_z) {
	    gpd_3dsite(gp, xo, yo, 0);
	}
	else {
	    for (i = 0; i < gp->n_surfs; i++) {
		gs = gs_get_surf(gp->drape_surf_id[i]);

		if (gs) {
		    gpd_2dsite(gp, gs, 0);
		#ifdef TRACE_GP_FUNCS
		    G_debug (3, "Drawing site %d on Surf %d", id,
			     gp->drape_surf_id[i]);
		    print_site_fields(gp);
		#endif
		}
	    }
	}
    }

    return;
}

/*!
  \brief Draw all available point sets
*/
void GP_alldraw_site(void)
{
    int id;

    for (id = 0; id < Next_site; id++) {
	GP_draw_site(Site_ID[id]);
    }

    return;
}

/*!
  \brief Set client data

  \param id point set id
  \param clientd client data

  \return 1 on success
  \return -1 on error (invalid point set id)
*/
int GP_Set_ClientData(int id, void *clientd)
{
    geosite *gp;

    gp = gp_get_site(id);

    if (gp) {
	gp->clientdata = clientd;
	return (1);
    }

    return (-1);
}

/*!
  \brief Get client data

  \param id point set id

  \return pointer to client data
  \return NULL on error
*/
void *GP_Get_ClientData(int id)
{
    geosite *gp;

    gp = gp_get_site(id);
    if (gp) {
	return (gp->clientdata);
    }

    return (NULL);
}
