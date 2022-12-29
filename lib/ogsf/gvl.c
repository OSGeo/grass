/*!
   \file lib/ogsf/gvl.c

   \brief OGSF library - loading and manipulating volumes (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown, UI-GMSL (May 1997)
   \author Tomas Paudits (February 2004)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "gsget.h"

#define FIRST_VOL_ID 81721

static geovol *Vol_top = NULL;

/*!
   \brief Get volume set structure

   \param id volume set id

   \return pointer to geovol struct
   \return NULL on failure
 */
geovol *gvl_get_vol(int id)
{
    geovol *gvl;

    G_debug(5, "gvl_get_vol():");

    for (gvl = Vol_top; gvl; gvl = gvl->next) {
	if (gvl->gvol_id == id) {
	    G_debug(5, "    id=%d", id);
	    return (gvl);
	}
    }

    return (NULL);
}

/*!
   \brief Get previous volume

   \param id current volume set id

   \return pointer to geovol struct
   \return NULL on failure
 */
geovol *gvl_get_prev_vol(int id)
{
    geovol *pv;

    G_debug(5, "gvl_get_prev_vol");

    for (pv = Vol_top; pv; pv = pv->next) {
	if (pv->gvol_id == id - 1) {
	    return (pv);
	}
    }

    return (NULL);
}

/*!
   \brief Get all volumes

   \param[out] list of geovol structs

   \return number of available volume sets
 */
int gvl_getall_vols(geovol ** gvols)
{
    geovol *gvl;
    int i;

    G_debug(5, "gvl_getall_vols");

    for (i = 0, gvl = Vol_top; gvl; gvl = gvl->next, i++) {
	gvols[i] = gvl;
    }

    return (i);
}

/*!
   \brief Get number of loaded volume sets

   \return number of volumes
 */
int gvl_num_vols(void)
{
    geovol *gvl;
    int i;

    for (i = 0, gvl = Vol_top; gvl; gvl = gvl->next, i++) ;

    G_debug(5, "gvl_num_vols(): num=%d", i);

    return (i);
}

/*!
   \brief Get last volume set from the list

   \return pointer to geovol struct
   \return NULL on failure
 */
geovol *gvl_get_last_vol(void)
{
    geovol *lvl;

    G_debug(5, "gvl_get_last_vol");

    if (!Vol_top) {
	return (NULL);
    }

    for (lvl = Vol_top; lvl->next; lvl = lvl->next) ;

    G_debug(5, "  last vol id: %d", lvl->gvol_id);

    return (lvl);
}

/*!
   \brief Allocate new volume set and add it to the list

   \return pointer to geovol struct
   \return NULL on failure
 */
geovol *gvl_get_new_vol(void)
{
    geovol *nvl, *lvl;

    G_debug(5, "gvl_get_new_vol()");

    nvl = (geovol *) G_malloc(sizeof(geovol));	/* G_fatal_error */
    if (!nvl) {
	return (NULL);
    }

    if ((lvl = gvl_get_last_vol())) {
	lvl->next = nvl;
	nvl->gvol_id = lvl->gvol_id + 1;
    }
    else {
	Vol_top = nvl;
	nvl->gvol_id = FIRST_VOL_ID;
    }

    nvl->next = NULL;

    G_debug(5, "    id=%d", nvl->gvol_id);
    
    return (nvl);
}

/*!
   \brief Initialize geovol structure

   \param gvl pointer to geovol struct
   \param ox,oy,oz
   \param rows number of rows
   \param cols number of cols
   \param xres,yres,zres x/y/z resolution value

   \return -1 on failure
   \return 1 on success
 */
int gvl_init_vol(geovol * gvl, double ox, double oy, double oz,
		 int rows, int cols, int depths, double xres, double yres,
		 double zres)
{
    G_debug(5, "gvl_init_vol() id=%d", gvl->gvol_id);

    if (!gvl) {
	return (-1);
    }

    gvl->ox = ox;
    gvl->oy = oy;
    gvl->oz = oz;
    gvl->rows = rows;
    gvl->cols = cols;
    gvl->depths = depths;
    gvl->xres = xres;
    gvl->yres = yres;
    gvl->zres = zres;

    gvl->xmin = ox;
    gvl->xmax = ox + (cols - 1) * xres;
    gvl->xrange = gvl->xmax - gvl->xmin;
    gvl->ymin = oy;
    gvl->ymax = oy + (rows - 1) * yres;
    gvl->yrange = gvl->ymax - gvl->ymin;
    gvl->zmin = oz;
    gvl->zmax = oz + (depths - 1) * zres;
    gvl->zrange = gvl->zmax - gvl->zmin;

    gvl->x_trans = gvl->y_trans = gvl->z_trans = 0.0;
    gvl->draw_wire = 0;

    gvl->n_isosurfs = 0;
    G_zero(gvl->isosurf, sizeof(geovol_isosurf *) * MAX_ISOSURFS);
    gvl->isosurf_x_mod = 1;
    gvl->isosurf_y_mod = 1;
    gvl->isosurf_z_mod = 1;
    gvl->isosurf_draw_mode = DM_GOURAUD;

    gvl->n_slices = 0;
    G_zero(gvl->slice, sizeof(geovol_slice *) * MAX_SLICES);
    gvl->slice_x_mod = 1;
    gvl->slice_y_mod = 1;
    gvl->slice_z_mod = 1;
    gvl->slice_draw_mode = DM_GOURAUD;

    gvl->hfile = -1;
    gvl->clientdata = NULL;

    return (1);
}

/*!
   \brief Remove volume set from list

   \param id volume set id
 */
void gvl_delete_vol(int id)
{
    geovol *fvl;

    G_debug(5, "gvl_delete_vol");

    fvl = gvl_get_vol(id);

    if (fvl) {
	gvl_free_vol(fvl);
    }

    return;
}

/*!
   \brief Free geovol struct

   \param fvl pointer to geovol struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_free_vol(geovol * fvl)
{
    geovol *gvl;
    int found = 0;

    G_debug(5, "gvl_free_vol");

    if (Vol_top) {
	if (fvl == Vol_top) {
	    if (Vol_top->next) {
		/* can't free top if last */
		found = 1;
		Vol_top = fvl->next;
	    }
	    else {
		gvl_free_volmem(fvl);
		G_free(fvl);
		Vol_top = NULL;
	    }
	}
	else {
	    for (gvl = Vol_top; gvl && !found; gvl = gvl->next) {
		/* can't free top */
		if (gvl->next) {
		    if (gvl->next == fvl) {
			found = 1;
			gvl->next = fvl->next;
		    }
		}
	    }
	}

	if (found) {
	    gvl_free_volmem(fvl);
	    G_free(fvl);
	    fvl = NULL;
	}

	return (1);
    }

    return (-1);
}

/*!
   \brief Free geovol struct memory

   \param fvl pointer to geovol struct
 */
void gvl_free_volmem(geovol * fvl)
{
    if (0 < fvl->hfile)
	gvl_file_free_datah(fvl->hfile);

    return;
}

/*!
   \brief Debug volume fields

   \param gvl pointer to geovol struct
 */
void print_vol_fields(geovol * gvl)
{
    G_debug(5, "ID: %d", gvl->gvol_id);
    G_debug(5, "cols: %d rows: %d depths: %d", gvl->cols, gvl->rows,
	    gvl->depths);
    G_debug(5, "ox: %lf oy: %lf oz: %lf", gvl->ox, gvl->oy, gvl->oz);
    G_debug(5, "xres: %lf yres: %lf zres: %lf", gvl->xres, gvl->yres,
	    gvl->zres);
    G_debug(5, "xmin: %f ymin: %f zmin: %f", gvl->xmin, gvl->ymin, gvl->zmin);
    G_debug(5, "xmax: %f ymax: %f zmax: %f", gvl->xmax, gvl->ymax, gvl->zmax);
    G_debug(5, "x_trans: %f y_trans: %f z_trans: %f", gvl->x_trans,
	    gvl->y_trans, gvl->z_trans);

    return;
}

/*!
   \brief Get volume x-extent value

   \param gvl pointer to geovol struct
   \param[out] min x-min value
   \param[out] max y-max value

   \return 1
 */
int gvl_get_xextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->xmin + gvl->x_trans;
    *max = gvl->xmax + gvl->x_trans;

    return (1);
}

/*!
   \brief Get volume y-extent value

   \param gvl pointer to geovol struct
   \param[out] min y-min value
   \param[out] max y-max value

   \return 1
 */
int gvl_get_yextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->ymin + gvl->y_trans;
    *max = gvl->ymax + gvl->y_trans;

    return (1);
}

/*!
   \brief Get volume z-extent value

   \param gvl pointer to geovol struct
   \param[out] min z-min value
   \param[out] max z-max value

   \return 1
 */
int gvl_get_zextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->zmin + gvl->z_trans;
    *max = gvl->zmax + gvl->z_trans;

    return (1);
}

/*!
   \brief Get volume x-range value

   \param[out] min x-min value
   \param[out] max x-max value

   \return 1
 */
int gvl_get_xrange(float *min, float *max)
{
    geovol *gvl;
    float tmin, tmax;

    if (Vol_top) {
	gvl_get_xextents(Vol_top, &tmin, &tmax);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gvl = Vol_top->next; gvl; gvl = gvl->next) {
	gvl_get_xextents(gvl, &tmin, &tmax);

	if (tmin < *min) {
	    *min = tmin;
	}

	if (tmax > *max) {
	    *max = tmax;
	}
    }

    return (1);
}

/*!
   \brief Get volume y-range value

   \param[out] min y-min value
   \param[out] max y-max value

   \return 1
 */
int gvl_get_yrange(float *min, float *max)
{
    geovol *gvl;
    float tmin, tmax;

    if (Vol_top) {
	gvl_get_yextents(Vol_top, &tmin, &tmax);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gvl = Vol_top->next; gvl; gvl = gvl->next) {
	gvl_get_yextents(gvl, &tmin, &tmax);

	if (tmin < *min) {
	    *min = tmin;
	}

	if (tmax > *max) {
	    *max = tmax;
	}
    }

    return (1);
}

/*!
   \brief Get volume z-range value

   \param[out] min z-min value
   \param[out] max z-max value

   \return 1
 */
int gvl_get_zrange(float *min, float *max)
{
    geovol *gvl;
    float tmin, tmax;

    if (Vol_top) {
	gvl_get_zextents(Vol_top, &tmin, &tmax);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gvl = Vol_top->next; gvl; gvl = gvl->next) {
	gvl_get_zextents(gvl, &tmin, &tmax);

	if (tmin < *min) {
	    *min = tmin;
	}

	if (tmax > *max) {
	    *max = tmax;
	}
    }

    return (1);
}

/************************************************************************/
/* ISOSURFACES */

/************************************************************************/

/*!
   \brief Initialize geovol_isosurf struct

   \param isosurf pointer to geovol_isosurf struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_init(geovol_isosurf * isosurf)
{
    int i;

    G_debug(5, "gvl_isosurf_init");

    if (!isosurf)
	return (-1);

    for (i = 0; i < MAX_ATTS; i++) {
	isosurf->att[i].att_src = NOTSET_ATT;
	isosurf->att[i].constant = 0.;
	isosurf->att[i].hfile = -1;
	isosurf->att[i].user_func = NULL;
	isosurf->att[i].att_data = NULL;
	isosurf->att[i].changed = 0;
    }

    isosurf->data = NULL;
    isosurf->data_desc = 0;
    isosurf->inout_mode = 0;

    return (1);
}

/*!
   \brief Free geovol_isosurf struct

   \param isosurf pointer to geovol_isosurf struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_freemem(geovol_isosurf * isosurf)
{
    int i;

    G_debug(5, "gvl_isosurf_freemem");

    if (!isosurf)
	return (-1);

    for (i = 0; i < MAX_ATTS; i++) {
	gvl_isosurf_set_att_src(isosurf, i, NOTSET_ATT);
    }

    G_free(isosurf->data);

    return (1);
}

/*!
   \brief Get isosurface of given volume set

   \param id volume set id
   \param isosurf_id isosurface id (0 - MAX_ISOSURFS)

   \return pointer to geovol_isosurf struct
   \return NULL on failure
 */
geovol_isosurf *gvl_isosurf_get_isosurf(int id, int isosurf_id)
{
    geovol *gvl;

    G_debug(5, "gvl_isosurf_get_isosurf(): id=%d isosurf=%d", id, isosurf_id);
    
    gvl = gvl_get_vol(id);

    if (gvl) {
	if ((isosurf_id < 0) || (isosurf_id > (gvl->n_isosurfs - 1)))
	    return (NULL);

	return gvl->isosurf[isosurf_id];
    }

    return (NULL);
}

/*!
   \brief Get attribute source

   \param isosurf pointer to geovol_isosurf struct
   \param desc attribute id

   \return -1 on failure
   \return attribute value
 */
int gvl_isosurf_get_att_src(geovol_isosurf * isosurf, int desc)
{
    G_debug(5, "isosurf_get_att_src");

    if (!LEGAL_ATT(desc)) {
	return (-1);
    }

    if (isosurf) {
	return (isosurf->att[desc].att_src);
    }

    return (-1);
}

/*!
   \brief Set attribute source

   \param isosurf pointer to geovol_isosurf struct
   \param desc attribute id
   \param src attribute value

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_set_att_src(geovol_isosurf * isosurf, int desc, int src)
{
    G_debug(5, "gvl_isosurf_set_att_src");

    /* check if old source was MAP_ATT, deattach volfile */
    if (MAP_ATT == gvl_isosurf_get_att_src(isosurf, desc)) {
	gvl_file_free_datah(isosurf->att[desc].hfile);

	if (desc == ATT_COLOR) {
	    Gvl_unload_colors_data(isosurf->att[desc].att_data);
	}
    }

    if (isosurf && LEGAL_SRC(src)) {
	isosurf->att[desc].att_src = src;
	gvl_isosurf_set_att_changed(isosurf, desc);

	return (1);
    }

    return (-1);
}

/*!
   \brief Set isosurface attribute constant

   \param isosurf pointer to geovol_isosurf struct
   \param desc attribute descriptor
   \param constant attribute value

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_set_att_const(geovol_isosurf * isosurf, int desc,
			      float constant)
{
    G_debug(5, "gvl_isosurf_set_att_const(): att=%d, const=%f",
	    desc, constant);

    if (isosurf) {
	isosurf->att[desc].constant = constant;

	gvl_isosurf_set_att_src(isosurf, desc, CONST_ATT);

	return (1);
    }

    return (-1);
}

/*!
   \brief Set attribute map

   \param isosurf pointer to geovol_isosurf struct
   \param desc attribute id
   \param filename filename

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_set_att_map(geovol_isosurf * isosurf, int desc,
			    const char *filename)
{
    int hfile;

    G_debug(5, "gvl_isosurf_set_att_map(): att=%d map=%s", desc, filename);

    if (isosurf) {
	if (0 > (hfile = gvl_file_newh(filename, VOL_FTYPE_RASTER3D)))
	    return (-1);

	gvl_isosurf_set_att_src(isosurf, desc, MAP_ATT);

	isosurf->att[desc].hfile = hfile;

	if (ATT_COLOR == desc) {
	    Gvl_load_colors_data(&(isosurf->att[desc].att_data), filename);
	}
	return (1);
    }

    return (-1);
}

/*!
   \brief Set attribute changed

   \param isosurf pointer to geovol_isosurf struct
   \param desc attribute id

   \return -1 on failure
   \return 1 on success
 */
int gvl_isosurf_set_att_changed(geovol_isosurf * isosurf, int desc)
{
    int i;

    G_debug(5, "gvl_isosurf_set_att_changed");

    if (isosurf && LEGAL_ATT(desc)) {
	isosurf->att[desc].changed = 1;

	if ((desc == ATT_TOPO) || (desc == ATT_MASK)) {
	    for (i = 1; i < MAX_ATTS; i++)
		isosurf->att[i].changed = 1;
	}

	return (1);
    }

    return (-1);
}

/************************************************************************/
/* SLICES */

/************************************************************************/

/*!
   \brief Initialize geovol_slice struct

   \param slice pointer to geovol_slice struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_slice_init(geovol_slice * slice)
{
    G_debug(5, "gvl_slice_init");

    if (!slice)
	return (-1);

    slice->data = NULL;
    slice->changed = 0;
    slice->mode = 1;
    slice->transp = 0;

    slice->z1 = 0;
    slice->z2 = 99;

    return (1);
}

/*!
   \brief Free geovol_slice struct

   \param slice pointer to geovol_slice struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_slice_freemem(geovol_slice * slice)
{
    G_debug(5, "gvl_slice_freemem");

    if (!slice)
	return (-1);

    G_free(slice->data);

    return (1);
}

/*!
   \brief Get geovol_slice struct

   \param id volume set id
   \param slice_id slice id

   \return pointer to geovol_slice struct
   \return NULL on failure
 */
geovol_slice *gvl_slice_get_slice(int id, int slice_id)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
	if ((slice_id < 0) || (slice_id > (gvl->n_slices - 1)))
	    return (NULL);

	return gvl->slice[slice_id];
    }

    return (NULL);
}
