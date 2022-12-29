/*!
   \file lib/ogsf/gvl2.c

   \brief OGSF library - loading and manipulating volumes

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown UI-GMSL (May 1997)
   Tomas Paudits (February 2004)
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/ogsf.h>
#include <grass/glocale.h>
#include "gsget.h"

static int Vol_ID[MAX_VOLS];
static int Next_vol = 0;

static RASTER3D_Region wind3;
static double Region[6];

/*!
   \brief Library initialization for volumes

   Set region extent (N,S,W,E,T,B)
 */
void GVL_libinit(void)
{
    Rast3d_init_defaults();
    Rast3d_get_window(&wind3);

    Region[0] = wind3.north;
    Region[1] = wind3.south;
    Region[2] = wind3.west;
    Region[3] = wind3.east;
    Region[4] = wind3.top;
    Region[5] = wind3.bottom;

    return;
}

/*!
   \brief Initialize 3D region

   Set region extent (N,S,W,E,T,B)
 */
void GVL_init_region(void)
{
    Rast3d_read_window(&wind3, NULL);

    Region[0] = wind3.north;
    Region[1] = wind3.south;
    Region[2] = wind3.west;
    Region[3] = wind3.east;
    Region[4] = wind3.top;
    Region[5] = wind3.bottom;

    return;
}

/*!
   \brief Get region extent settings

   \param[out] n,s,w,e north, south, west, east
   \param[out] t,b top, bottom

   \return 1
 */
int GVL_get_region(float *n, float *s, float *w, float *e, float *t, float *b)
{
    *n = Region[0];
    *s = Region[1];
    *w = Region[2];
    *e = Region[3];
    *t = Region[4];
    *b = Region[5];

    return (1);
}

/*!
   \brief Get window 

   \todo gvl_file.c use this - change

   \return pointer to RASTER3D_Region struct (static)
 */
void *GVL_get_window()
{
    return &wind3;
}

/*!
   \brief Check if volume set exists

   \param id volume set id

   \return 1 found
   \return 0 not found
 */
int GVL_vol_exists(int id)
{
    int i, found = 0;

    G_debug(3, "GVL_vol_exists");

    if (NULL == gvl_get_vol(id)) {
	return (0);
    }

    for (i = 0; i < Next_vol && !found; i++) {
	if (Vol_ID[i] == id) {
	    found = 1;
	}
    }

    return (found);
}

/*!
   \brief Create new volume set

   \return volume set id
   \return -1 on error
 */
int GVL_new_vol(void)
{
    geovol *nvl;

    G_debug(3, "GVL_new_vol():");

    if (Next_vol < MAX_VOLS) {
	nvl = gvl_get_new_vol();

	gvl_init_vol(nvl, wind3.west + wind3.ew_res / 2.,
		     wind3.south + wind3.ns_res / 2., wind3.bottom,
		     wind3.rows, wind3.cols, wind3.depths,
		     wind3.ew_res, wind3.ns_res, wind3.tb_res);

	Vol_ID[Next_vol] = nvl->gvol_id;
	++Next_vol;

	G_debug(3, "    id=%d", nvl->gvol_id);
	
	return (nvl->gvol_id);
    }

    return (-1);
}

/*!
   \brief Get number of loaded volume sets

   \return number of volume sets
 */
int GVL_num_vols(void)
{
    return (gvl_num_vols());
}

/*!
   \brief Get list of loaded volume sets

   Must be freed if not needed!

   \param[out] numvols number of volume sets

   \return pointer to list of volume sets
   \return NULL on error
 */
int *GVL_get_vol_list(int *numvols)
{
    int i, *ret;

    *numvols = Next_vol;

    if (Next_vol) {
	ret = (int *)G_malloc(Next_vol * sizeof(int));
	if (!ret)
	    return (NULL);

	for (i = 0; i < Next_vol; i++) {
	    ret[i] = Vol_ID[i];
	}

	return (ret);
    }

    return (NULL);
}

/*!
   \brief Delete volume set from list

   \param id volume set id

   \return 1 on success
   \return -1 on error (invalid volume set id)
 */
int GVL_delete_vol(int id)
{
    int i, j, found = 0;

    G_debug(3, "GVL_delete_vol");

    if (GVL_vol_exists(id)) {

	for (i = 0; i < GVL_isosurf_num_isosurfs(id); i++) {
	    GVL_isosurf_del(id, 0);
	}

	for (i = 0; i < GVL_slice_num_slices(id); i++) {
	    GVL_slice_del(id, 0);
	}

	gvl_delete_vol(id);

	for (i = 0; i < Next_vol && !found; i++) {
	    if (Vol_ID[i] == id) {
		found = 1;
		for (j = i; j < Next_vol; j++) {
		    Vol_ID[j] = Vol_ID[j + 1];
		}
	    }
	}

	if (found) {
	    --Next_vol;

	    return (1);
	}
    }

    return (-1);
}

/*!
   \brief Load 3d raster map to volume set

   \param id volume set id
   \param filename 3d raster map name

   \return -1 on error
   \return 0 on success
 */
int GVL_load_vol(int id, const char *filename)
{
    geovol *gvl;
    int handle;

    G_debug(3, "GVL_load_vol(): id=%d, name=%s", id, filename);

    if (NULL == (gvl = gvl_get_vol(id))) {
	return (-1);
    }

    G_message(_("Loading 3d raster map <%s>..."), filename);

    if (0 > (handle = gvl_file_newh(filename, VOL_FTYPE_RASTER3D)))
	return (-1);

    gvl->hfile = handle;

    return (0);
}

/*!
   \brief Get volume set name

   \param id volume set id
   \param[out] filename name (must be allocated)

   \return -1 on error
   \return 1 on success
 */
int GVL_get_volname(int id, char *filename)
{
    geovol *gvl;

    if (NULL == (gvl = gvl_get_vol(id))) {
	return (-1);
    }

    if (0 > gvl->hfile) {
	return (-1);
    }

    strcpy(filename, gvl_file_get_name(gvl->hfile));

    return (1);
}

/*!
   \brief Get volume dimensions

   \param id volume set id
   \param[out] rows,cols,depths number of rows, cols, depths
 */
void GVL_get_dims(int id, int *rows, int *cols, int *depths)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	*rows = gvl->rows;
	*cols = gvl->cols;
	*depths = gvl->depths;
    }

    G_debug(3, "GVL_get_dims() id=%d, rows=%d, cols=%d, depths=%d",
	    gvl->gvol_id, gvl->rows, gvl->cols, gvl->depths);
    
    return;
}

/*!
   \brief Set trans ?

   \param id volume set id
   \param xtrans,ytrans,ztrans x/y/z trans values
 */
void GVL_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geovol *gvl;

    G_debug(3, "GVL_set_trans");

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->x_trans = xtrans;
	gvl->y_trans = ytrans;
	gvl->z_trans = ztrans;
    }

    return;
}

/*!
   \brief Get trans ?

   \param id volume set id
   \param[out] xtrans,ytrans,ztrans x/y/z trans values

   \return 1 on success
   \return -1 on error
 */
int GVL_get_trans(int id, float *xtrans, float *ytrans, float *ztrans)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	*xtrans = gvl->x_trans;
	*ytrans = gvl->y_trans;
	*ztrans = gvl->z_trans;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set drawing wire box

   \param id volume set id
   \param draw_wire 1 for drawing wire, 0 otherwise
 */
void GVL_set_draw_wire(int id, int draw_wire)
{
    geovol *gvl;

    G_debug(3, "GVL_set_draw_wire");

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->draw_wire = draw_wire;
    }

    return;
}

/*!
   \brief Draw volume set

   \param vid volume set id
 */
void GVL_draw_vol(int vid)
{
    geovol *gvl;

    gvl = gvl_get_vol(vid);

    if (gvl) {
	gvld_vol(gvl);
        if (gvl->draw_wire) {
	    gvld_wind3_box(gvl);
        }
    }

    return;
}

/*!
   \brief Draw volume in wire mode

   \param id volume set id
 */
void GVL_draw_wire(int id)
{
    geovol *gvl;

    G_debug(3, "GVL_draw_wire(): id=%d", id);

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvld_wire_vol(gvl);
    }

    return;
}

/*!
   \brief Draw all volume sets
 */
void GVL_alldraw_vol(void)
{
    int id;

    for (id = 0; id < Next_vol; id++) {
	GVL_draw_vol(Vol_ID[id]);
    }

    return;
}

/*!
   \brief Draw all volume sets in wire mode
 */
void GVL_alldraw_wire(void)
{
    int id;

    for (id = 0; id < Next_vol; id++) {
	GVL_draw_wire(Vol_ID[id]);
    }

    return;
}

/*!
   \brief Set client data for volume set

   \param id volume set id
   \param clientd pointer to client data

   \return 1 on success
   \return -1 on error
 */
int GVL_Set_ClientData(int id, void *clientd)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->clientdata = clientd;

	return (1);
    }

    return (-1);
}

/*!
   \brief Get client data

   \param id volume set id

   \return pointer to client data
   \return NULL on error
 */
void *GVL_Get_ClientData(int id)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	return (gvl->clientdata);
    }

    return (NULL);
}

/*!
   \brief Set focus on map center

   \param id volume set id
 */
void GVL_set_focus_center_map(int id)
{
    float center[3];
    geovol *gvl;

    G_debug(3, "GS_set_focus_center_map");

    gvl = gvl_get_vol(id);

    if (gvl) {
	center[X] = (gvl->xmax - gvl->xmin) / 2.;
	center[Y] = (gvl->ymax - gvl->ymin) / 2.;
	center[Z] = (gvl->zmax - gvl->zmin) / 2.;

	GS_set_focus(center);
    }

    return;
}

/************************************************************************/
/* ISOSURFACES */

/************************************************************************/

/*!
   \brief Get draw resolution for isosurface

   \todo error handling

   \param id volume set id
   \param[out] xres,yres,zres x/y/z resolution value
 */
void GVL_isosurf_get_drawres(int id, int *xres, int *yres, int *zres)
{
    geovol *gvl;

    G_debug(3, "GVL_isosurf_get_drawres");

    gvl = gvl_get_vol(id);

    if (gvl) {
	*xres = gvl->isosurf_x_mod;
	*yres = gvl->isosurf_y_mod;
	*zres = gvl->isosurf_z_mod;
    }

    return;
}

/*!
   \brief Set isosurface draw resolution

   \param id volume set id
   \param xres,yres,zres x/y/z resolution value

   \return -1 on error (invalid values/volume set id)
   \return 0 on success
 */
int GVL_isosurf_set_drawres(int id, int xres, int yres, int zres)
{
    geovol *gvl;
    int i;

    G_debug(3, "GVL_isosurf_set_drawres(): id=%d", id);

    if (xres < 1 || yres < 1 || zres < 1) {
	return (-1);
    }

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->isosurf_x_mod = xres;
	gvl->isosurf_y_mod = yres;
	gvl->isosurf_z_mod = zres;

	for (i = 0; i < gvl->n_isosurfs; i++) {
	    gvl_isosurf_set_att_changed(gvl->isosurf[i], ATT_TOPO);
	}

	return (0);
    }

    return (-1);
}

/*!
   \brief Get isosurface draw mode

   \param id volume set id
   \param[out] mode draw-mode

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_get_drawmode(int id, int *mode)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	*mode = gvl->isosurf_draw_mode;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set isosurface draw mode

   \param id volume set id
   \param mode draw mode

   \return 0 on success
   \return -1 on error (invalid volume set id)
 */
int GVL_isosurf_set_drawmode(int id, int mode)
{
    geovol *gvl;

    G_debug(3, "GVL_isosurf_set_drawmode(): id=%d mode=%d", id, mode);

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->isosurf_draw_mode = mode;

	return (0);
    }

    return (-1);
}

/*!
   \brief Add isosurface

   \param id volume set id

   \return -1 on error (invalid volume set id
   \return 1 on success
 */
int GVL_isosurf_add(int id)
{
    geovol *gvl;
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_add() id=%d", id);

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (gvl->n_isosurfs == MAX_ISOSURFS)
	return (-1);

    isosurf = (geovol_isosurf *) G_malloc(sizeof(geovol_isosurf));
    if (!isosurf) {
	return (-1);
    }

    gvl_isosurf_init(isosurf);

    gvl->n_isosurfs++;
    gvl->isosurf[gvl->n_isosurfs - 1] = (geovol_isosurf *) isosurf;

    return (1);
}

/*!
   \brief Delete isosurface

   \param id volume set id
   \param isosurf_id isosurface id

   \return -1 on error
   \return 1 on success
 */
int GVL_isosurf_del(int id, int isosurf_id)
{
    geovol *gvl;
    geovol_isosurf *isosurf;
    int i;

    G_debug(3, "GVL_isosurf_del");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (!isosurf)
	return (-1);

    if (!gvl_isosurf_freemem(isosurf)) {
	return (-1);
    }

    gvl = gvl_get_vol(id);

    G_free(gvl->isosurf[isosurf_id]);

    for (i = isosurf_id + 1; i < gvl->n_isosurfs; i++) {
	gvl->isosurf[i - 1] = gvl->isosurf[i];
    }

    gvl->n_isosurfs--;

    return (1);
}

/*!
   \brief Move up isosurface in list

   \param id volume set id
   \param isosurf_id isosurface id

   \return -1 on error
   \return 1 on success
 */
int GVL_isosurf_move_up(int id, int isosurf_id)
{
    geovol *gvl;
    geovol_isosurf *tmp;

    G_debug(3, "GVL_isosurf_move_up");

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (isosurf_id < 0 || isosurf_id > (gvl->n_isosurfs - 1))
	return (-1);

    if (isosurf_id == 0)
	return (1);

    tmp = gvl->isosurf[isosurf_id - 1];
    gvl->isosurf[isosurf_id - 1] = gvl->isosurf[isosurf_id];
    gvl->isosurf[isosurf_id] = tmp;

    return (1);
}

/*!
   \brief Move down isosurface in list

   \param id volume set id
   \param isosurf_id isosurface id

   \return -1 on error
   \return 1 on success
 */
int GVL_isosurf_move_down(int id, int isosurf_id)
{
    geovol *gvl;
    geovol_isosurf *tmp;

    G_debug(3, "GVL_isosurf_move_up");

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (isosurf_id < 0 || isosurf_id > (gvl->n_isosurfs - 1))
	return (-1);

    if (isosurf_id == (gvl->n_isosurfs - 1))
	return (1);

    tmp = gvl->isosurf[isosurf_id + 1];
    gvl->isosurf[isosurf_id + 1] = gvl->isosurf[isosurf_id];
    gvl->isosurf[isosurf_id] = tmp;

    return (1);
}

/*!
   \brief Get isosurface attributes

   \param id volume set id
   \param isosurf_id surface id
   \param att attribute id
   \param[out] set
   \param[out] constant
   \param[out] mapname

   \return -1 on error
   \return 1 on success
 */
int GVL_isosurf_get_att(int id, int isosurf_id,
			int att, int *set, float *constant, char *mapname)
{
    int src;
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_get_att");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	if (-1 != (src = gvl_isosurf_get_att_src(isosurf, att))) {
	    *set = src;

	    if (src == CONST_ATT) {
		*constant = isosurf->att[att].constant;
	    }
	    else if (src == MAP_ATT) {
		strcpy(mapname, gvl_file_get_name(isosurf->att[att].hfile));
	    }

	    return (1);
	}

	return (-1);
    }

    return (-1);
}

/*!
   \brief Unset isosurface attributes

   \param id volume set id
   \param isosurface_id isosurface id
   \param att attribute id

   \return ?
   \return -1 on error
 */
int GVL_isosurf_unset_att(int id, int isosurf_id, int att)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_unset_att");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	return (gvl_isosurf_set_att_src(isosurf, att, NOTSET_ATT));
    }

    return (-1);
}

/*!
   \brief Set constant isosurface attribute

   Attributes:
    - ATT_NORM
    - ATT_TOPO topography (level) constant
    - ATT_COLOR color map/constant
    - ATT_MASK mask map
    - ATT_TRANSP transparency map/constant
    - ATT_SHINE shininess map/constant
    - ATT_EMIT emission map/constant

   \param id volume set id
   \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
   \param att attribute descriptor
   \param constant constant value

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_set_att_const(int id, int isosurf_id, int att, float constant)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_set_att_const() id=%d isosurf_id=%d "
	    "att=%d const=%f", id, isosurf_id, att, constant);

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	return (gvl_isosurf_set_att_const(isosurf, att, constant));
    }

    return (-1);
}

/*!
   \brief Set isosurface map attribute

   Attributes:
    - ATT_NORM
    - ATT_TOPO topography (level) constant
    - ATT_COLOR color map/constant
    - ATT_MASK mask map
    - ATT_TRANSP transparency map/constant
    - ATT_SHINE shininess map/constant
    - ATT_EMIT emission map/constant

   \param id volume set id
   \param isosurf_id isosurface id (0 - MAX_ISOSURFS)
   \param att attribute descriptor
   \param filename map name

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_set_att_map(int id, int isosurf_id, int att,
			    const char *filename)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_set_att_map(): id=%d, isosurf_id=%d "
	    "att=%d map=%s", id, isosurf_id, att, filename);

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	return gvl_isosurf_set_att_map(isosurf, att, filename);
    }

    return (-1);
}

/*!
   \brief Get isosurface flags

   \param id volume set id
   \param isosurf_id isosurface id
   \param[out] inout map name

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_get_flags(int id, int isosurf_id, int *inout)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_get_flags");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	*inout = isosurf->inout_mode;

	return (1);
    }
    return (-1);
}

/*!
   \brief Set isosurface flags

   \param id volume set id
   \param isosurf_id isosurface id
   \param inout map name

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_set_flags(int id, int isosurf_id, int inout)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_get_flags");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	isosurf->inout_mode = inout;

	return (1);
    }

    return (-1);
}

/*!
   \brief Get number of available isosurfaces

   \param id volume set id

   \return number of isosurfaces
   \return -1 on error
 */
int GVL_isosurf_num_isosurfs(int id)
{
    geovol *gvl;

    G_debug(3, "GVL_isosurf_num_isosurfs");

    gvl = gvl_get_vol(id);

    if (gvl) {
	return gvl->n_isosurfs;
    }

    return (-1);
}

/*!
   \brief Set mask attribute mode

   Mask attribute special: constant is set to indicate invert or no

   \param id volume set id
   \param isosurf_id isosurface id
   \param mode attribute mode

   \return mode id
   \return -1 on error
 */
int GVL_isosurf_set_maskmode(int id, int isosurf_id, int mode)
{
    geovol_isosurf *isosurf;

    G_debug(3, "GVL_isosurf_set_att_const");

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	isosurf->att[ATT_MASK].constant = mode;

	return (mode);
    }

    return (-1);
}

/*!
   \brief Get isosurface mask mode

   \param id volume set id
   \param isosurf_id isosurface id
   \param mode attribute mode

   \return 1 on success
   \return -1 on error
 */
int GVL_isosurf_get_maskmode(int id, int isosurf_id, int *mode)
{
    geovol_isosurf *isosurf;

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
	*mode = isosurf->att[ATT_MASK].constant;

	return (1);
    }

    return (-1);
}

/************************************************************************/
/* SLICES */

/************************************************************************/

/*!
   \brief Get draw resolution of slice

   \param id volume set id
   \param[out] xres,yres,zres x/y/z resolution value
 */
void GVL_slice_get_drawres(int id, int *xres, int *yres, int *zres)
{
    geovol *gvl;

    G_debug(3, "GVL_slice_get_drawres");

    gvl = gvl_get_vol(id);

    if (gvl) {
	*xres = gvl->slice_x_mod;
	*yres = gvl->slice_y_mod;
	*zres = gvl->slice_z_mod;
    }

    return;
}

/*!
   \brief Set slice draw resolution

   \param id volume set id
   \param xres,yres,zres x/y/z resolution value

   \return 0 on success
   \return -1 on error (invalid value or id)
 */
int GVL_slice_set_drawres(int id, int xres, int yres, int zres)
{
    geovol *gvl;
    int i;

    G_debug(3, "GVL_slice_set_drawres(): id=%d", id);

    if (xres < 1 || yres < 1 || zres < 1) {
	return (-1);
    }

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->slice_x_mod = xres;
	gvl->slice_y_mod = yres;
	gvl->slice_z_mod = zres;

	for (i = 0; i < gvl->n_slices; i++) {
	    gvl->slice[i]->changed = 1;
	}

	return (0);
    }

    return (-1);
}

/*!
   \brief Get slice draw mode

   \param id volume set id
   \param[out] mode draw mode

   \return 1 on success
   \return -1 on error (invalid id)
 */
int GVL_slice_get_drawmode(int id, int *mode)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	*mode = gvl->slice_draw_mode;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set slice draw mode

   \param id volume set id
   \param mode draw mode

   \return 0 on success
   \return -1 on error (invalid id)
 */
int GVL_slice_set_drawmode(int id, int mode)
{
    geovol *gvl;

    G_debug(3, "GVL_slice_set_drawmode(): id=%d, mode=%d", id, mode);

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvl->slice_draw_mode = mode;

	return (0);
    }

    return (-1);
}

/*!
   \brief Add slice

   \param id volume set id

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_add(int id)
{
    geovol *gvl;
    geovol_slice *slice;

    G_debug(3, "GVL_slice_add");

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (gvl->n_slices == MAX_SLICES)
	return (-1);

    if (NULL == (slice = (geovol_slice *) G_malloc(sizeof(geovol_slice)))) {
	return (-1);
    }

    gvl_slice_init(slice);

    gvl->n_slices++;
    gvl->slice[gvl->n_slices - 1] = (geovol_slice *) slice;

    return (1);
}

/*!
   \brief Delete slice

   \param id volume set id
   \param slice_id slice id

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_del(int id, int slice_id)
{
    geovol *gvl;
    geovol_slice *slice;
    int i;

    G_debug(3, "GVL_slice_del");

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
	return (-1);

    if (!gvl_slice_freemem(slice)) {
	return (-1);
    }

    gvl = gvl_get_vol(id);

    G_free(gvl->slice[slice_id]);

    for (i = slice_id + 1; i < gvl->n_slices; i++) {
	gvl->slice[i - 1] = gvl->slice[i];
    }

    gvl->n_slices--;

    return (1);
}

/*!
   \brief Move up slice

   \param id volume set id
   \param slice_id slice id

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_move_up(int id, int slice_id)
{
    geovol *gvl;
    geovol_slice *tmp;

    G_debug(3, "GVL_slice_move_up");

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (slice_id < 0 || slice_id > (gvl->n_slices - 1))
	return (-1);

    if (slice_id == 0)
	return (1);

    tmp = gvl->slice[slice_id - 1];
    gvl->slice[slice_id - 1] = gvl->slice[slice_id];
    gvl->slice[slice_id] = tmp;

    return (1);
}

/*!
   \brief Move down slice

   \param id volume set id
   \param slice_id slice id

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_move_down(int id, int slice_id)
{
    geovol *gvl;
    geovol_slice *tmp;

    G_debug(3, "GVL_slice_move_up");

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    if (slice_id < 0 || slice_id > (gvl->n_slices - 1))
	return (-1);

    if (slice_id == (gvl->n_slices - 1))
	return (1);

    tmp = gvl->slice[slice_id + 1];
    gvl->slice[slice_id + 1] = gvl->slice[slice_id];
    gvl->slice[slice_id] = tmp;

    return (1);
}

/*!
   \brief Get number or slices

   \param id volume set id

   \return number of slices
   \return -1 on error
 */
int GVL_slice_num_slices(int id)
{
    geovol *gvl;

    G_debug(3, "GVL_isosurf_num_isosurfs");

    gvl = gvl_get_vol(id);

    if (gvl) {
	return gvl->n_slices;
    }

    return (-1);
}

/*!
   \brief Get slice position

   \param id volume set id
   \param slice_id slice id
   \param[out] x1,y1,z1 coordinates ?
   \param[out] x2,y2,z2 coordinates ?
   \param[out] dir direction

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_get_pos(int id, int slice_id,
		      float *x1, float *x2, float *y1, float *y2, float *z1,
		      float *z2, int *dir)
{
    geovol *gvl;
    geovol_slice *slice;
    int cols, rows, depths;

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
	return (-1);

    if (slice->dir == X) {
	cols = gvl->rows;
	rows = gvl->depths;
	depths = gvl->cols;
    }
    else if (slice->dir == Y) {
	cols = gvl->cols;
	rows = gvl->depths;
	depths = gvl->rows;
    }
    else if (slice->dir == Z) {
	cols = gvl->cols;
	rows = gvl->rows;
	depths = gvl->depths;
    }
    else {
	return (-1);
    }

    *x1 = slice->x1 / (cols - 1);
    *x2 = slice->x2 / (cols - 1);
    *y1 = slice->y1 / (rows - 1);
    *y2 = slice->y2 / (rows - 1);
    *z1 = slice->z1 / (depths - 1);
    *z2 = slice->z2 / (depths - 1);

    *dir = slice->dir;

    return (1);
}

/*!
   \brief Get slice position

   \param id volume set id
   \param slice_id slice id
   \param x1,y1,z1 coordinates ?
   \param x2,y2,z2 coordinates ?
   \param dir direction

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_set_pos(int id, int slice_id,
		      float x1, float x2, float y1, float y2, float z1,
		      float z2, int dir)
{
    geovol *gvl;
    geovol_slice *slice;
    int cols, rows, depths;

    gvl = gvl_get_vol(id);

    if (!gvl)
	return (-1);

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
	return (-1);

    if (dir == X) {
	cols = gvl->rows;
	rows = gvl->depths;
	depths = gvl->cols;
    }
    else if (dir == Y) {
	cols = gvl->cols;
	rows = gvl->depths;
	depths = gvl->rows;
    }
    else if (dir == Z) {
	cols = gvl->cols;
	rows = gvl->rows;
	depths = gvl->depths;
    }
    else {
	return (-1);
    }

    slice->x1 = ((x1 < 0.) ? 0. : ((x1 > 1.) ? 1. : x1)) * (cols - 1);
    slice->x2 = ((x2 < 0.) ? 0. : ((x2 > 1.) ? 1. : x2)) * (cols - 1);
    slice->y1 = ((y1 < 0.) ? 0. : ((y1 > 1.) ? 1. : y1)) * (rows - 1);
    slice->y2 = ((y2 < 0.) ? 0. : ((y2 > 1.) ? 1. : y2)) * (rows - 1);
    slice->z1 = ((z1 < 0.) ? 0. : ((z1 > 1.) ? 1. : z1)) * (depths - 1);
    slice->z2 = ((z2 < 0.) ? 0. : ((z2 > 1.) ? 1. : z2)) * (depths - 1);

    slice->dir = dir;

    slice->changed = 1;

    return (1);
}

/*!
   \brief Get slice trans ?

   \param id volume set id
   \param slice_id slice id
   \param[out] transp transp value

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_get_transp(int id, int slice_id, int *transp)
{
    geovol_slice *slice;

    G_debug(3, "GVL_get_transp");

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
	return (-1);

    *transp = slice->transp;

    return (1);
}

/*!
   \brief Set slice trans ?

   \param id volume set id
   \param slice_id slice id
   \param transp transp value

   \return -1 on error
   \return 1 on success
 */
int GVL_slice_set_transp(int id, int slice_id, int transp)
{
    geovol_slice *slice;

    G_debug(3, "GVL_set_transp");

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
	return (-1);

    slice->transp = transp;

    return (1);
}
