/*
* $Id$
*/

/*  gvl.c
    volume access routines
    Bill Brown, UI-GMSL, May 1997
	Tomas Paudits, Februar 2004
*/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gstypes.h>
#include "gsget.h"

#define FIRST_VOL_ID 81721

static geovol *Vol_top = NULL;

/***********************************************************************/
geovol *gvl_get_vol(int id)
{
    geovol *gvl;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_get_vol");
    }
#endif

    for (gvl = Vol_top; gvl; gvl = gvl->next) {
    if (gvl->gvol_id == id) {
        return (gvl);
    }
    }

    return (NULL);
}

/***********************************************************************/
geovol *gvl_get_prev_vol(int id)
{
    geovol *pv;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_get_prev_vol");
    }
#endif

    for (pv = Vol_top; pv; pv = pv->next) {
    if (pv->gvol_id == id - 1) {
        return (pv);
    }
    }

    return (NULL);
}

/***********************************************************************/
int gvl_getall_vols(geovol ** gvols)
{
    geovol *gvl;
    int i;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_getall_vols");
    }
#endif

    for (i = 0, gvl = Vol_top; gvl; gvl = gvl->next, i++) {
    gvols[i] = gvl;
    }

    return (i);
}

/***********************************************************************/
int gvl_num_vols(void)
{
    geovol *gvl;
    int i;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_num_vols");
    }
#endif

    for (i = 0, gvl = Vol_top; gvl; gvl = gvl->next, i++);

    return (i);
}

/***********************************************************************/
geovol *gvl_get_last_vol(void)
{
    geovol *lvl;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_get_last_vol");
    }
#endif

    if (!Vol_top) {
    return (NULL);
    }

    for (lvl = Vol_top; lvl->next; lvl = lvl->next);

#ifdef DEBUG
    {
    fprintf(stderr, "last vol id: %d\n", lvl->gvol_id);
    }
#endif

    return (lvl);
}

/***********************************************************************/
geovol *gvl_get_new_vol(void)
{
    geovol *nvl, *lvl;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_get_new_vol");
    }
#endif

    if (NULL == (nvl = (geovol *) malloc(sizeof(geovol)))) {
    gs_err("gvl_get_new_vol");

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

    return (nvl);
}

/***********************************************************************/
int gvl_init_vol(geovol * gvl, double ox, double oy, double oz,
        int rows, int cols, int depths, double xres, double yres, double zres)
{
#ifdef TRACE_FUNCS
    {
        Gs_status("gvl_init_vol");
    }
#endif

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
    gvl->xmax = ox + cols * xres;
    gvl->xrange = gvl->xmax - gvl->xmin;
    gvl->ymin = oy;
    gvl->ymax = oy + rows * yres;
    gvl->yrange = gvl->ymax - gvl->ymin;
    gvl->zmin = oz;
    gvl->zmax = oz + depths * zres;
    gvl->zrange = gvl->zmax - gvl->zmin;

    gvl->x_trans = gvl->y_trans = gvl->z_trans = 0.0;

    gvl->n_isosurfs = 0;
    gvl->isosurf_x_mod = 1;
    gvl->isosurf_y_mod = 1;
    gvl->isosurf_z_mod = 1;

	gvl->n_slices = 0;
    gvl->slice_x_mod = 1;
    gvl->slice_y_mod = 1;
    gvl->slice_z_mod = 1;

    gvl->hfile = -1;
    gvl->clientdata = NULL;

    return (1);
}

/***********************************************************************/
void gvl_delete_vol(int id)
{
    geovol *fvl;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_delete_vol");
    }
#endif

    fvl = gvl_get_vol(id);

    if (fvl) {
    gvl_free_vol(fvl);
    }

    return;
}

/***********************************************************************/
int gvl_free_vol(geovol * fvl)
{
    geovol *gvl;
    int found = 0;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_free_vol");
    }
#endif

    if (Vol_top) {
    if (fvl == Vol_top) {
        if (Vol_top->next) {
        /* can't free top if last */
        found = 1;
        Vol_top = fvl->next;
        }
        else {
        gvl_free_volmem(fvl);
        free(fvl);
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
        free(fvl);
        fvl = NULL;
    }

    return (1);
    }

    return (-1);
}

/***********************************************************************/
void gvl_free_volmem(geovol * fvl)
{
    if (0 < fvl->hfile)
        gvl_file_free_datah(fvl->hfile);

    return;
}

/***********************************************************************/
void print_vol_fields(geovol *gvl)
{
    fprintf(stderr, "ID: %d\n", gvl->gvol_id);
    fprintf(stderr, "cols: %d rows: %d depths: %d\n", gvl->cols, gvl->rows, gvl->depths);
    fprintf(stderr, "ox: %lf oy: %lf oz: %lf\n", gvl->ox, gvl->oy, gvl->oz);
    fprintf(stderr, "xres: %lf yres: %lf zres: %lf\n", gvl->xres, gvl->yres, gvl->zres);
    fprintf(stderr, "xmin: %f ymin: %f zmin: %f\n", gvl->xmin, gvl->ymin, gvl->zmin);
    fprintf(stderr, "xmax: %f ymax: %f zmax: %f\n", gvl->xmax, gvl->ymax, gvl->zmax);
    fprintf(stderr, "x_trans: %f y_trans: %f z_trans: %f\n", gvl->x_trans, gvl->y_trans, gvl->z_trans);

    return;
}

/***********************************************************************/
int gvl_get_xextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->xmin + gvl->x_trans;
    *max = gvl->xmax + gvl->x_trans;

    return (1);
}

/***********************************************************************/
int gvl_get_yextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->ymin + gvl->y_trans;
    *max = gvl->ymax + gvl->y_trans;

    return (1);
}

/***********************************************************************/
int gvl_get_zextents(geovol * gvl, float *min, float *max)
{
    *min = gvl->zmin + gvl->z_trans;
    *max = gvl->zmax + gvl->z_trans;

    return (1);
}

/***********************************************************************/
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

/***********************************************************************/
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

/***********************************************************************/
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

/************************************************************************/
int gvl_isosurf_init(geovol_isosurf *isosurf)
{
    int i;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_init");
    }
#endif

    if (!isosurf)
        return (-1);

    for (i = 0; i < MAX_ATTS; i++) {
        isosurf->att[i].att_src = NOTSET_ATT;
        isosurf->att[i].constant = 0.;
        isosurf->att[i].hfile = -1;
    }

    isosurf->data = NULL;
	isosurf->data_desc = 0;
    isosurf->inout_mode = 0;

    return (1);
}


/************************************************************************/
int gvl_isosurf_freemem(geovol_isosurf *isosurf)
{
    int i;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_freemem");
    }
#endif

    if (!isosurf)
        return (-1);

    for (i = 0; i < MAX_ATTS; i++) {
        gvl_isosurf_set_att_src(isosurf, i, NOTSET_ATT);
    }

    free(isosurf->data);

    return (1);
}

/***********************************************************************/
geovol_isosurf* gvl_isosurf_get_isosurf(int id, int isosurf_id)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
        if ((isosurf_id < 0) || (isosurf_id > (gvl->n_isosurfs - 1)))
            return (NULL);

        return gvl->isosurf[isosurf_id];
    }

    return (NULL);
}

/***********************************************************************/
int gvl_isosurf_get_att_src(geovol_isosurf *isosurf, int desc)
{

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_get_att_src");
    }
#endif

    if (!LEGAL_ATT(desc)) {
    return (-1);
    }

    if (isosurf) {
    return (isosurf->att[desc].att_src);
    }

    return (-1);
}

/***********************************************************************/
int gvl_isosurf_set_att_src(geovol_isosurf *isosurf, int desc, int src)
{

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_set_att_src");
    }
#endif

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

/***********************************************************************/
int gvl_isosurf_set_att_const(geovol_isosurf *isosurf, int desc, float constant)
{

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_set_att_const");
    }
#endif

    if (isosurf) {
    isosurf->att[desc].constant = constant;

    gvl_isosurf_set_att_src(isosurf, desc, CONST_ATT);

    return (1);
    }

    return (-1);
}

/***********************************************************************/
int gvl_isosurf_set_att_map(geovol_isosurf *isosurf, int desc, char *filename)
{
    int hfile;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_set_att_map");
    }
#endif

#ifdef DEBUG_MSG
    {
    fprintf(stderr, "att_map: %s\n", filename);
    }
#endif

    if (isosurf) {
    if (0 > (hfile = gvl_file_newh(filename, VOL_FTYPE_G3D)))
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

/***********************************************************************/
int gvl_isosurf_set_att_changed(geovol_isosurf *isosurf, int desc)
{
	int i;

#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_isosurf_set_att_changed");
    }
#endif

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

/************************************************************************/
int gvl_slice_init(geovol_slice *slice)
{
#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_slice_init");
    }
#endif

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

/************************************************************************/
int gvl_slice_freemem(geovol_slice *slice)
{
#ifdef TRACE_FUNCS
    {
    Gs_status("gvl_slice_freemem");
    }
#endif

    if (!slice)
        return (-1);

    free(slice->data);

    return (1);
}

/***********************************************************************/
geovol_slice* gvl_slice_get_slice(int id, int slice_id)
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
