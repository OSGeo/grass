/*!
   \file lib/ogsf/GP2.c

   \brief OGSF library - loading and manipulating point sets (higher level functions)

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL (January 1994)
   \author Updated by Martin landa <landa.martin gmail.com>
   (doxygenized in May 2008, thematic mapping in June 2011)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/ogsf.h>
#include <grass/glocale.h>

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

    G_debug(4, "GP_site_exists(%d)", id);

    if (NULL == gp_get_site(id)) {
	return 0;
    }

    for (i = 0; i < Next_site && !found; i++) {
	if (Site_ID[i] == id) {
	    found = 1;
	}
    }

    G_debug(3, "GP_site_exists(): found=%d", found);

    return found;
}

/*!
   \brief Create new point set

   \return point set id
   \return -1 on error (number of point sets exceeded)
 */
int GP_new_site(void)
{
    geosite *np;

    if (Next_site < MAX_SITES) {
	np = gp_get_new_site();
	gp_set_defaults(np);
	Site_ID[Next_site] = np->gsite_id;
	++Next_site;

	G_debug(3, "GP_new_site() id=%d", np->gsite_id);

	return np->gsite_id;
    }

    return -1;
}

/*!
   \brief Get number of loaded point sets

   \return number of point sets
 */
int GP_num_sites(void)
{
    return gp_num_sites();
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
	ret = (int *)G_malloc(Next_site * sizeof(int));	/* G_fatal_error */
	if (!ret) {
	    return NULL;
	}

	for (i = 0; i < Next_site; i++) {
	    ret[i] = Site_ID[i];
	}

	return ret;
    }

    return NULL;
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

    G_debug(4, "GP_delete_site(%d)", id);

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
	    return 1;
	}
    }

    return -1;
}

/*!
   \brief Load point set from file

   Check to see if handle already loaded, if so - free before loading
   new for now, always load to memory.

   \todo load file handle & ready for reading instead of using memory

   \param id point set id
   \param filename point set filename

   \return -1 on error
   \return 1 on success
 */
int GP_load_site(int id, const char *filename)
{
    geosite *gp;

    G_debug(3, "GP_load_site(id=%d, name=%s)", id, filename);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    if (gp->points) {
	gp_free_sitemem(gp);
    }

    gp->filename = G_store(filename);

    gp->points = Gp_load_sites(filename, &(gp->n_sites), &(gp->has_z));
    
    if (gp->points) {
	return 1;
    }

    return -1;
}

/*!
   \brief Get point set filename

   Note: char array is allocated by G_store()

   \param id point set id
   \param[out] filename point set filename

   \return -1 on error (point set not found)
   \return 1 on success
 */
int GP_get_sitename(int id, char **filename)
{
    geosite *gp;

    G_debug(4, "GP_get_sitename(%d)", id);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    *filename = G_store(gp->filename);

    return 1;
}

/*!
   \brief Get point set style

   \param id point set id

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GP_get_style(int id, int *color, int *width, float *size, int *symbol)
{
    geosite *gp;

    G_debug(4, "GP_get_style(%d)", id);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    *color = gp->style->color;
    *width = gp->style->width;
    *symbol = gp->style->symbol;
    *size = gp->style->size;

    return 1;
}

/*!
   \brief Set point style

   Supported icon symbols (markers):
    - ST_X
    - ST_BOX
    - ST_SPHERE
    - ST_CUBE
    - ST_DIAMOND
    - ST_DEC_TREE
    - ST_CON_TREE
    - ST_ASTER
    - ST_GYRO
    - ST_HISTOGRAM

   \param id point set id
   \param color icon color
   \param width icon line width
   \param size icon size
   \param symbol icon symbol

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GP_set_style(int id, int color, int width, float size, int symbol)
{
    geosite *gp;

    G_debug(4, "GP_set_style(id=%d, color=%d, width=%d, size=%f, symbol=%d)", id, color, width, size,
	    symbol);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    gp->style->color = color;
    gp->style->symbol = symbol;
    gp->style->size = size;
    gp->style->width = width;

    return 1;
}

/*!
   \brief Set point set style for thematic mapping

   Updates also style for each geopoint.
   
   \param id point set id
   \param layer layer number for thematic mapping (-1 for undefined)
   \param color icon color column name
   \param width icon line width column name
   \param size icon size column name
   \param symbol icon symbol column name
   \param colors pointer to Colors structure or NULL

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GP_set_style_thematic(int id, int layer, const char* color, const char* width,
			  const char* size, const char* symbol, struct Colors *color_rules)
{
    geosite *gp;
    
    G_debug(4, "GP_set_style_thematic(id=%d, layer=%d, color=%s, width=%s, size=%s, symbol=%s)", id, layer,
	    color, width, size, symbol);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    if(!gp->tstyle)
	gp->tstyle = (gvstyle_thematic *)G_malloc(sizeof(gvstyle_thematic));
    G_zero(gp->tstyle, sizeof(gvstyle_thematic));
    
    gp->tstyle->active = 1;
    gp->tstyle->layer = layer;
    if (color)
	gp->tstyle->color_column = G_store(color);
    if (symbol)
	gp->tstyle->symbol_column = G_store(symbol);
    if (size)
	gp->tstyle->size_column = G_store(size);
    if (width)
	gp->tstyle->width_column = G_store(width);

    Gp_load_sites_thematic(gp, color_rules);

    return 1;
}

/*!
   \brief Make style for thematic mapping inactive
   
   \param id point set id

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GP_unset_style_thematic(int id)
{
    geosite *gp;

    G_debug(4, "GP_unset_style_thematic(): id=%d", id);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    if (gp->tstyle) {
	gp->tstyle->active = 0;
    }

    return 1;
}

/*!
   \brief Set z mode for point set

   \param id point set id
   \param use_z TRUE to use z-coordinaces when vector map is 3D

   \return 1 on success
   \return 0 vector map is not 3D
   \return -1 on error (invalid point set id)
 */
/* I don't see who is using it? Why it's required? */
int GP_set_zmode(int id, int use_z)
{
    geosite *gp;

    G_debug(3, "GP_set_zmode(%d,%d)", id, use_z);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    if (use_z) {
	if (gp->has_z) {
	    gp->use_z = 1;
	    return 1;
	}

	return 0;
    }

    gp->use_z = 0;
    return 1;
}

/*!
   \brief Get z-mode

   \todo Who's using this?
   
   \param id point set id
   \param[out] use_z non-zero code to use z

   \return -1 on error (invalid point set id)
   \return 1 on success
 */
int GP_get_zmode(int id, int *use_z)
{
    geosite *gp;

    G_debug(4, "GP_get_zmode(%d)", id);

    if (NULL == (gp = gp_get_site(id))) {
	return -1;
    }

    *use_z = gp->use_z;
    return 1;
}

/*!
   \brief Set transformation params

   \param id point set id
   \param xtrans,ytrans,ztrans x/y/z values
 */
void GP_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geosite *gp;

    G_debug(3, "GP_set_trans(): id=%d trans=%f,%f,%f",
	    id, xtrans, ytrans, ztrans);

    gp = gp_get_site(id);
    if (gp) {
	gp->x_trans = xtrans;
	gp->y_trans = ytrans;
	gp->z_trans = ztrans;
    }

    return;
}

/*!
   \brief Get transformation params

   \param id point set id
   \param[out] xtrans,ytrans,ztrans x/y/z values
 */
void GP_get_trans(int id, float *xtrans, float *ytrans, float *ztrans)
{
    geosite *gp;

    gp = gp_get_site(id);

    if (gp) {
	*xtrans = gp->x_trans;
	*ytrans = gp->y_trans;
	*ztrans = gp->z_trans;
    }

    G_debug(3, "GP_get_trans(): id=%d, trans=%f,%f,%f",
	    id, *xtrans, *ytrans, *ztrans);

    return;
}

/*!
   \brief Select surface for given point set

   \param hp point set id
   \param hs surface id

   \return 1 surface selected
   \return -1 on error
 */
int GP_select_surf(int hp, int hs)
{
    geosite *gp;

    G_debug(3, "GP_select_surf(%d,%d)", hp, hs);

    if (GP_surf_is_selected(hp, hs)) {
	return 1;
    }

    gp = gp_get_site(hp);

    if (gp && GS_surf_exists(hs)) {
	gp->drape_surf_id[gp->n_surfs] = hs;
	gp->n_surfs += 1;
	return 1;
    }

    return -1;
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

    G_debug(3, "GP_unselect_surf(%d,%d)", hp, hs);

    if (!GP_surf_is_selected(hp, hs)) {
	return 1;
    }

    gp = gp_get_site(hp);

    if (gp) {
	for (i = 0; i < gp->n_surfs; i++) {
	    if (gp->drape_surf_id[i] == hs) {
		for (j = i; j < gp->n_surfs - 1; j++) {
		    gp->drape_surf_id[j] = gp->drape_surf_id[j + 1];
		}

		gp->n_surfs -= 1;
		return 1;
	    }
	}
    }

    return -1;
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

    G_debug(3, "GP_surf_is_selected(%d,%d)", hp, hs);

    gp = gp_get_site(hp);

    if (gp) {
	for (i = 0; i < gp->n_surfs; i++) {
	    if (hs == gp->drape_surf_id[i]) {
		return 1;
	    }
	}
    }

    return 0;
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
		    G_debug(5, "Drawing site %d on Surf %d", id,
			    gp->drape_surf_id[i]);
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
	return 1;
    }

    return -1;
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

    return NULL;
}

/*!
  \brief Determine point marker symbol for string

  Supported markers:
    - ST_X
    - ST_BOX
    - ST_SPHERE
    - ST_CUBE
    - ST_DIAMOND
    - ST_DEC_TREE
    - ST_CON_TREE
    - ST_ASTER
    - ST_GYRO
    - ST_HISTOGRAM

  \param str string buffer

  \return marker code (default: ST_SPHERE)
*/
int GP_str_to_marker(const char *str)
{
    int marker;

    if (strcmp(str, "x") == 0)
	marker = ST_X;
    else if (strcmp(str, "box") == 0)
	marker = ST_BOX;
    else if (strcmp(str, "sphere") == 0)
	marker = ST_SPHERE;
    else if (strcmp(str, "cube") == 0)
	marker = ST_CUBE;
    else if (strcmp(str, "diamond") == 0)
	marker = ST_DIAMOND;
    else if (strcmp(str, "dec_tree") == 0)
	marker = ST_DEC_TREE;
    else if (strcmp(str, "con_tree") == 0)
	marker = ST_CON_TREE;
    else if (strcmp(str, "aster") == 0)
	marker = ST_ASTER;
    else if (strcmp(str, "gyro") == 0)
	marker = ST_GYRO;
    else if (strcmp(str, "histogram") == 0)
	marker = ST_HISTOGRAM;
    else {
	G_warning(_("Unknown icon marker, using \"sphere\""));
	marker = ST_SPHERE;
    }

    return marker;
}
