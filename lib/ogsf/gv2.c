/*!
   \file lib/ogsf/gv2.c

   \brief OGSF library - loading and manipulating vector sets (higher level functions)

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Updated by Martin landa <landa.martin gmail.com>
   (doxygenized in May 2008, thematic mapping in June 2011)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "gsget.h"

static int Vect_ID[MAX_VECTS];
static int Next_vect = 0;

/*!
   \brief Check if vector set exists

   \param id vector set id

   \return 0 not found
   \return 1 found
 */
int GV_vect_exists(int id)
{
    int i, found = 0;

    G_debug(3, "GV_vect_exists");

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

/*!
   \brief Register new vector set

   \return vector set id
   \return -1 on error
 */
int GV_new_vector(void)
{
    geovect *nv;

    if (Next_vect < MAX_VECTS) {
	nv = gv_get_new_vect();
	gv_set_defaults(nv);
	Vect_ID[Next_vect] = nv->gvect_id;
	++Next_vect;

	G_debug(3, "GV_new_vector(): id=%d", nv->gvect_id);

	return nv->gvect_id;
    }

    return -1;
}

/*!
   \brief Get number of available vector sets

   \return number of vector sets
 */
int GV_num_vects(void)
{
    return (gv_num_vects());
}

/*!
   \brief Get list of vector sets

   Must free when no longer needed!

   \param numvects number of vector sets

   \return pointer to list of point sets
   \return NULL on error
 */
int *GV_get_vect_list(int *numvects)
{
    int i, *ret;

    *numvects = Next_vect;

    if (Next_vect) {
	ret = (int *)G_malloc(Next_vect * sizeof(int));
	if (!ret) {
	    return (NULL);
	}

	for (i = 0; i < Next_vect; i++) {
	    ret[i] = Vect_ID[i];
	}

	return (ret);
    }

    return (NULL);
}

/*!
   \brief Delete vector set from list

   \param id vector set id

   \return 1 on success
   \return -1 on error
 */
int GV_delete_vector(int id)
{
    int i, j, found = 0;

    G_debug(3, "GV_delete_vect");

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

/*!
   \brief Load vector set

   Check to see if handle already loaded, if so - free before loading
   new for now, always load to memory

   \todo Load file handle & ready for reading instead of using
   memory

   \param id vector set id
   \param filename filename

   \return -1 on error (invalid vector set id)
   \return 1 on success
 */
int GV_load_vector(int id, const char *filename)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    if (gv->lines) {
	gv_free_vectmem(gv);
    }

    gv->filename = G_store(filename);

    if ((gv->lines = Gv_load_vect(filename, &(gv->n_lines)))) {
	return (1);
    }

    return (-1);
}

/*!
   \brief Get vector map name

   Note: char array is allocated by G_store()

   \param id vector set id
   \param filename &filename

   \return -1 on error (invalid vector set id)
   \return 1 on success
 */
int GV_get_vectname(int id, char **filename)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return (-1);
    }

    *filename = G_store(gv->filename);

    return (1);
}

/*!
   \brief Set vector style

   \param id vector set id
   \param mem non-zero for use memory
   \param color color value
   \param width line width
   \param use_z non-zero for 3d mode

   \return -1 on error (invalid vector set id)
   \return 1 on success
 */
int GV_set_style(int id, int mem, int color, int width, int use_z)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return -1;
    }

    gv->use_mem = mem;
    gv->use_z = use_z;
    gv->style->color = color;
    gv->style->width = width;

    return 1;
}


/*!
   \brief Get vector style

   \param id vector set id
   \param[out] mem non-zero for use memory
   \param[out] color color value
   \param[out] width line width
   \param[out] use_z non-zero for 3d mode

   \return -1 on error (invalid vector set id)
   \return 1 on success
 */
int GV_get_style(int id, int *mem, int *color, int *width, int *use_z)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return -1;
    }

    *mem = gv->use_mem;
    *color = gv->style->color;
    *width = gv->style->width;
    *use_z = gv->use_z;

    return 1;
}

/*!
   \brief Set vector set style for thematic mapping
   
   Updates also style for each geoline.
   
   \param id vector set id
   \param layer layer number for thematic mapping
   \param color color column name
   \param width width column name
   \param colors pointer to Colors structure or NULL

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GV_set_style_thematic(int id, int layer, const char* color, const char* width,
			  struct Colors *color_rules)
{
    geovect *gv;

    if (NULL == (gv = gv_get_vect(id))) {
	return -1;
    }

    if(!gv->tstyle)
	gv->tstyle = (gvstyle_thematic *)G_malloc(sizeof(gvstyle_thematic));
    G_zero(gv->tstyle, sizeof(gvstyle_thematic));
    
    gv->tstyle->active = 1;
    gv->tstyle->layer = layer;
    if (color)
	gv->tstyle->color_column = G_store(color);
    if (width)
	gv->tstyle->width_column = G_store(width);

    Gv_load_vect_thematic(gv, color_rules);

    return 1;
}

/*!
   \brief Make style for thematic mapping inactive
   
   \param id vector set id

   \return 1 on success
   \return -1 on error (point set not found)
 */
int GV_unset_style_thematic(int id)
{
    geovect *gv;

    G_debug(4, "GV_unset_style_thematic(): id=%d", id);

    if (NULL == (gv = gv_get_vect(id))) {
	return -1;
    }

    if (gv->tstyle) {
	gv->tstyle->active = 0;
    }

    return 1;
}

/*!
   \brief Set trans ?

   \param id vector set id
   \param xtrans,ytrans,ztrans x/y/z trans values
 */
void GV_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geovect *gv;

    G_debug(3, "GV_set_trans");

    gv = gv_get_vect(id);

    if (gv) {
	gv->x_trans = xtrans;
	gv->y_trans = ytrans;
	gv->z_trans = ztrans;
    }

    return;
}

/*!
   \brief Get trans ?

   \param id vector set id
   \param[out] xtrans,ytrans,ztrans x/y/z trans values
 */
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

/*!
   \brief Select surface identified by hs to have vector identified
   by hv draped over it

   \param hv vector set id
   \param hs surface id

   \return 1 on success
   \return -1 on error
 */
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

/*!
   \brief Unselect surface

   \param hv vector set id
   \param hs surface id

   \return 1 on success
   \return -1 on error
 */
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

/*!
   \brief Check if surface is selected

   \param hv vector set id
   \param hs surface id

   \return 1 selected
   \return 0 not selected
 */
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

/*!
   \brief Draw vector set

   \param vid vector set id
 */
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

/*!
   \brief Draw all loaded vector sets
 */
void GV_alldraw_vect(void)
{
    int id;

    for (id = 0; id < Next_vect; id++) {
	GV_draw_vect(Vect_ID[id]);
    }

    return;
}

/*!
   \brief Draw vector set (fast mode)

   \todo Seems to be broken, nothing is drawn

   \param vid vector set id
 */
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

/*!
   \brief Draw all loaded vector sets (fast mode)
 */
void GV_alldraw_fastvect(void)
{
    int id;

    for (id = 0; id < Next_vect; id++) {
	GV_draw_fastvect(Vect_ID[id]);
    }

    return;
}

/*!
   \brief Set client data

   \param id vector set id
   \param clientd pointer to client data

   \return 1 on success
   \return -1 on error
 */
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

/*!
   \brief Get client data

   \param id vector set id

   \return pointer to client data
   \return NULL on error
 */
void *GV_Get_ClientData(int id)
{
    geovect *gv;

    gv = gv_get_vect(id);

    if (gv) {
	return (gv->clientdata);
    }

    return (NULL);
}
