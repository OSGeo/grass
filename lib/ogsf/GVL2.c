/*  GVL.c
    Volume access routines
    Bill Brown, UI-GMSL, May 1997
    Tomas Paudits, Februar 2004
*/

#include <grass/gis.h>
#include <grass/G3d.h>
#include <grass/gstypes.h>
#include "gsget.h"

#ifdef TRACE_FUNCS
#define TRACE_GVL_FUNCS
#endif

static int Vol_ID[MAX_VOLS];
static int Next_vol = 0;

static G3D_Region wind3;
static double Region[6];

/***********************************************************************/
void GVL_libinit(void)
{
    G3d_initDefaults();
    G3d_getWindow(&wind3);

    Region[0] = wind3.north;
    Region[1] = wind3.south;
    Region[2] = wind3.west;
    Region[3] = wind3.east;
    Region[4] = wind3.top;
    Region[5] = wind3.bottom;

    return;
}

/***********************************************************************/
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

/***********************************************************************/
/* TODO: gvl_file.c use this - change*/
void *GVL_get_window()
{
    return &wind3;
}

/***********************************************************************/
int GVL_vol_exists(int id)
{
    int i, found = 0;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_vol_exists");
    }
#endif

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

/***********************************************************************/
int GVL_new_vol(void)
{
    geovol *nvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_new_vol");
    }
#endif

    if (Next_vol < MAX_VOLS) {
        nvl = gvl_get_new_vol();

        gvl_init_vol(nvl, wind3.west, wind3.south, wind3.bottom,
            wind3.rows, wind3.cols, wind3.depths,
            wind3.ew_res, wind3.ns_res, wind3.tb_res);

        Vol_ID[Next_vol] = nvl->gvol_id;
        ++Next_vol;

        return (nvl->gvol_id);
    }

    return (-1);
}

/***********************************************************************/
int GVL_num_vols(void)
{
    return (gvl_num_vols());
}

/***********************************************************************/
/* USER must free!! */
int *GVL_get_vol_list(int *numvols)
{
    int i, *ret;

    *numvols = Next_vol;

    if (Next_vol) {
    if (NULL == (ret = (int *) G_malloc(Next_vol * sizeof(int)))) {
        fprintf(stderr, "can't malloc\n");

        return (NULL);
    }

    for (i = 0; i < Next_vol; i++) {
        ret[i] = Vol_ID[i];
    }

    return (ret);
    }

    return (NULL);
}

/***********************************************************************/
int GVL_delete_vol(int id)
{
    int i, j, found = 0;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_delete_vol");
    }
#endif

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

/***********************************************************************/
int GVL_load_vol(int id, char *filename)
{
	geovol *gvl;
    int handle;

    if (NULL == (gvl = gvl_get_vol(id))) {
        return (-1);
    }

    if (0 > (handle = gvl_file_newh(filename, VOL_FTYPE_G3D)))
        return (-1);

    gvl->hfile = handle;

	return (0);
}

/***********************************************************************/
int GVL_get_volname(int id, char *filename)
{
    geovol *gvl;

    if (NULL == (gvl = gvl_get_vol(id))) {
    return (-1);
    }

    if (0 > gvl->hfile) {
    return (-1);
    }

    G_strcpy(filename, gvl_file_get_name(gvl->hfile));

    return (1);
}

/***********************************************************************/
void GVL_get_dims(int id, int *rows, int *cols, int *depths)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_get_dims");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
    *rows = gvl->rows;
    *cols = gvl->cols;
    *depths = gvl->depths;
    }

    return;
}

/***********************************************************************/
void GVL_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_set_trans");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
    gvl->x_trans = xtrans;
    gvl->y_trans = ytrans;
    gvl->z_trans = ztrans;
    }

    return;
}

/***********************************************************************/
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

/***********************************************************************/
void GVL_draw_vol(int vid)
{
    geovol *gvl;

    gvl = gvl_get_vol(vid);

    if (gvl) {
    	gvld_vol(gvl);
    }

    return;
}

/***********************************************************************/
void GVL_draw_wire(int id)
{
    geovol *gvl;

#ifdef TRACE_GS_FUNCS
    {
	Gs_status("GVL_draw_wire");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
	gvld_wire_vol(gvl);
    }

    return;
}

/***********************************************************************/
void GVL_alldraw_vol(void)
{
    int id;

    for (id = 0; id < Next_vol; id++) {
    GVL_draw_vol(Vol_ID[id]);
    }

    return;
}

/***********************************************************************/
void GVL_alldraw_wire(void)
{
    int id;

    for (id = 0; id < Next_vol; id++) {
    GVL_draw_wire(Vol_ID[id]);
    }

    return;
}

/***********************************************************************/
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

/***********************************************************************/
void *GVL_Get_ClientData(int id)
{
    geovol *gvl;

    gvl = gvl_get_vol(id);

    if (gvl) {
    return (gvl->clientdata);
    }

    return (NULL);
}

/***********************************************************************/
void GVL_set_focus_center_map(int id)
{
    float center[3];
    geovol *gvl;

#ifdef TRACE_GS_FUNCS
    {
	Gs_status("GS_set_focus_center_map");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
	center[X] = (gvl->xmax - gvl->xmin) / 2.;
	center[Y] = (gvl->ymax - gvl->ymin) / 2.;
	center[Z] = (gvl->zmax - gvl->zmin) / 2.;

	GS_set_focus(center);
    }
}

/************************************************************************/
/* ISOSURFACES */
/************************************************************************/

/***********************************************************************/
void GVL_isosurf_get_drawres(int id, int *xres, int *yres, int *zres)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_get_drawres");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
        *xres = gvl->isosurf_x_mod;
        *yres = gvl->isosurf_y_mod;
        *zres = gvl->isosurf_z_mod;
    }

    return;
}

/***********************************************************************/
int GVL_isosurf_set_drawres(int id, int xres, int yres, int zres)
{
    geovol *gvl;
    int i;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_set_drawres");
    }
#endif

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
    }

    return (0);
}

/***********************************************************************/
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

/***********************************************************************/
int GVL_isosurf_set_drawmode(int id, int mode)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_set_drawmode");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
    gvl->isosurf_draw_mode = mode;

    return (0);
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_add(int id)
{
    geovol *gvl;
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_add");
    }
#endif

    gvl = gvl_get_vol(id);

    if (!gvl)
        return (-1);

    if (gvl->n_isosurfs == MAX_ISOSURFS)
        return (-1);

    if (NULL == (isosurf = (geovol_isosurf *) G_malloc(sizeof(geovol_isosurf)))) {
        return (-1);
    }

    gvl_isosurf_init(isosurf);

    gvl->n_isosurfs++;
    gvl->isosurf[gvl->n_isosurfs - 1] = (geovol_isosurf *) isosurf;

    return (1);
}

/***********************************************************************/
int GVL_isosurf_del(int id, int isosurf_id)
{
    geovol *gvl;
    geovol_isosurf *isosurf;
    int i;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_del");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (!isosurf)
        return (-1);

    if (!gvl_isosurf_freemem(isosurf)) {
        return (-1);
    }

    gvl = gvl_get_vol(id);

    G_free(gvl->isosurf[isosurf_id]);

    for (i = isosurf_id + 1; i < gvl->n_isosurfs; i++) {
        gvl->isosurf[i-1] = gvl->isosurf[i];
    }

    gvl->n_isosurfs--;

    return (1);
}

/***********************************************************************/
int GVL_isosurf_move_up(int id, int isosurf_id)
{
	geovol *gvl;
	geovol_isosurf *tmp;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_move_up");
    }
#endif

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

/***********************************************************************/
int GVL_isosurf_move_down(int id, int isosurf_id)
{
	geovol *gvl;
	geovol_isosurf *tmp;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_move_up");
    }
#endif

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

/***********************************************************************/
int GVL_isosurf_get_att(int id, int isosurf_id, int att, int *set, float *constant, char *mapname)
{
    int src;
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_get_att");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        if (-1 != (src = gvl_isosurf_get_att_src(isosurf, att))) {
            *set = src;

            if (src == CONST_ATT) {
                *constant = isosurf->att[att].constant;
            }
            else if (src == MAP_ATT) {
                G_strcpy(mapname, gvl_file_get_name(isosurf->att[att].hfile));
            }

            return (1);
        }

        return (-1);
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_unset_att(int id, int isosurf_id, int att)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_unset_att");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        return (gvl_isosurf_set_att_src(isosurf, att, NOTSET_ATT));
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_set_att_const(int id, int isosurf_id, int att, float constant)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_set_att_const");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        return (gvl_isosurf_set_att_const(isosurf, att, constant));
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_set_att_map(int id, int isosurf_id, int att, char *filename)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_set_att_map");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        return gvl_isosurf_set_att_map(isosurf, att, filename);
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_get_flags(int id, int isosurf_id, int *inout)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_get_flags");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        *inout = isosurf->inout_mode;

        return (1);
    }
    return (-1);
}

/***********************************************************************/
int GVL_isosurf_set_flags(int id, int isosurf_id, int inout)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_get_flags");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        isosurf->inout_mode = inout;

		return (1);
    }

    return (-1);
}

/***********************************************************************/
int GVL_isosurf_num_isosurfs(int id)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_num_isosurfs");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
        return gvl->n_isosurfs;
    }

    return (-1);
}

/***********************************************************************/
/* mask attribute special: constant is set to indicate invert or no */
int GVL_isosurf_set_maskmode(int id, int isosurf_id, int mode)
{
    geovol_isosurf *isosurf;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_set_att_const");
    }
#endif

    isosurf = gvl_isosurf_get_isosurf(id, isosurf_id);

    if (isosurf) {
        isosurf->att[ATT_MASK].constant = mode;

        return (mode);
    }

    return (-1);
}

/***********************************************************************/
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

/***********************************************************************/
void GVL_slice_get_drawres(int id, int *xres, int *yres, int *zres)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_get_drawres");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
        *xres = gvl->slice_x_mod;
        *yres = gvl->slice_y_mod;
        *zres = gvl->slice_z_mod;
    }

    return;
}

/***********************************************************************/
int GVL_slice_set_drawres(int id, int xres, int yres, int zres)
{
    geovol *gvl;
    int i;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_set_drawres");
    }
#endif

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
    }

    return (0);
}

/***********************************************************************/
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

/***********************************************************************/
int GVL_slice_set_drawmode(int id, int mode)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_set_drawmode");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
    gvl->slice_draw_mode = mode;

    return (0);
    }

    return (-1);
}

/***********************************************************************/
int GVL_slice_add(int id)
{
    geovol *gvl;
    geovol_slice *slice;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_add");
    }
#endif

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

/***********************************************************************/
int GVL_slice_del(int id, int slice_id)
{
    geovol *gvl;
    geovol_slice *slice;
    int i;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_del");
    }
#endif

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
        return (-1);

    if (!gvl_slice_freemem(slice)) {
        return (-1);
    }

    gvl = gvl_get_vol(id);

    G_free(gvl->slice[slice_id]);

    for (i = slice_id + 1; i < gvl->n_slices; i++) {
        gvl->slice[i-1] = gvl->slice[i];
    }

    gvl->n_slices--;

    return (1);
}

/***********************************************************************/
int GVL_slice_move_up(int id, int slice_id)
{
	geovol *gvl;
	geovol_slice *tmp;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_move_up");
    }
#endif

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

/***********************************************************************/
int GVL_slice_move_down(int id, int slice_id)
{
	geovol *gvl;
	geovol_slice *tmp;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_slice_move_up");
    }
#endif

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

/***********************************************************************/
int GVL_slice_num_slices(int id)
{
    geovol *gvl;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_isosurf_num_isosurfs");
    }
#endif

    gvl = gvl_get_vol(id);

    if (gvl) {
        return gvl->n_slices;
    }

    return (-1);
}

/***********************************************************************/
int GVL_slice_get_pos(int id, int slice_id, float *x1, float *x2, float *y1, float *y2, float *z1, float *z2, int *dir)
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
	} else if (slice->dir == Y) {
		cols = gvl->cols;
		rows = gvl->depths;
		depths = gvl->rows;
	}  else if (slice->dir == Z) {
		cols = gvl->cols;
		rows = gvl->rows;
		depths = gvl->depths;
	} else {
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

/***********************************************************************/
int GVL_slice_set_pos(int id, int slice_id, float x1, float x2, float y1, float y2, float z1, float z2, int dir)
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
	} else if (dir == Y) {
		cols = gvl->cols;
		rows = gvl->depths;
		depths = gvl->rows;
	}  else if (dir == Z) {
		cols = gvl->cols;
		rows = gvl->rows;
		depths = gvl->depths;
	} else {
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

/***********************************************************************/
int GVL_slice_get_transp(int id, int slice_id, int *transp)
{
    geovol_slice *slice;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_get_transp");
    }
#endif

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
        return (-1);

    *transp = slice->transp;

    return (1);
}

/***********************************************************************/
int GVL_slice_set_transp(int id, int slice_id, int transp)
{
    geovol_slice *slice;

#ifdef TRACE_GVL_FUNCS
    {
    Gs_status("GVL_set_transp");
    }
#endif

    slice = gvl_slice_get_slice(id, slice_id);

    if (!slice)
        return (-1);

    slice->transp = transp;

    return (1);
}
