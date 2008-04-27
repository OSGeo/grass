/*
* $Id$
*/

/*  gs.c
    Bill Brown, USACERL  
    January 1993
*/

#include <stdlib.h>
#include <stdio.h>

#include <grass/gstypes.h>
#include "gsget.h"
#include "rowcol.h"

#define FIRST_SURF_ID 110658

/*
#define TRACE_FUNCS
*/

static geosurf *Surf_top;
static int Invertmask;

/***********************************************************************/
void gs_err(char *msg)
{
    fprintf(stderr, "%s\n", msg);

    return;
}

/***********************************************************************/
/* still need to take care of library initialization, 
   probably want to define a Surf_top of constant value (i.e., 0) */
/***********************************************************************/
void gs_init(void)
{
    Surf_top = NULL;

    return;
}


/***********************************************************************/
geosurf *gs_get_surf(int id)
{
    geosurf *gs;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_surf");
    }
#endif

    for (gs = Surf_top; gs; gs = gs->next) {
	if (gs->gsurf_id == id) {
	    return (gs);
	}
    }

    return (NULL);
}

/***********************************************************************/
geosurf *gs_get_prev_surface(int id)
{
    geosurf *ps;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_prev_surface");
    }
#endif

    for (ps = Surf_top; ps; ps = ps->next) {
	if (ps->gsurf_id == id - 1) {
	    return (ps);
	}
    }

    return (NULL);
}

/***********************************************************************/
int gs_getall_surfaces(geosurf ** gsurfs)
{
    geosurf *gs;
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_num_surfaces");
    }
#endif

    for (i = 0, gs = Surf_top; gs; gs = gs->next, i++) {
	gsurfs[i] = gs;
    }

    return (i);
}

/***********************************************************************/
int gs_num_surfaces(void)
{
    geosurf *gs;
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_num_surfaces");
    }
#endif

    for (i = 0, gs = Surf_top; gs; gs = gs->next, i++);

    return (i);
}


/***********************************************************************/
int gs_att_is_set(geosurf * surf, IFLAG att)
{
    geosurf *gs;

    if (surf) {
	return (NOTSET_ATT != surf->att[att].att_src);
    }

    /* if surf == NULL, look at all surfs */
    for (gs = Surf_top; gs; gs = gs->next) {
	if (NOTSET_ATT != gs->att[att].att_src) {
	    return (1);
	}
    }

    return (0);
}

/***********************************************************************/
geosurf *gs_get_last_surface(void)
{
    geosurf *ls;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_last_surface");
    }
#endif

    if (!Surf_top) {
	return (NULL);
    }

    for (ls = Surf_top; ls->next; ls = ls->next);

#ifdef DEBUG
    {
	fprintf(stderr, "last surface id: %d\n", ls->gsurf_id);
    }
#endif

    return (ls);
}


/***********************************************************************/
geosurf *gs_get_new_surface(void)
{
    geosurf *ns, *ls;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_new_surface");
    }
#endif

    if (NULL == (ns = (geosurf *) malloc(sizeof(geosurf)))) {
	gs_err("gs_get_new_surface");
	return (NULL);
    }

    if ((ls = gs_get_last_surface())) {
	ls->next = ns;
	ns->gsurf_id = ls->gsurf_id + 1;
    }
    else {
	Surf_top = ns;
	ns->gsurf_id = FIRST_SURF_ID;
    }

    ns->next = NULL;

    return (ns);
}


/***********************************************************************/
/* Now xmin & ox are the same, right? - get rid of ox, oy in geosurf struct?*/
int gs_init_surf(geosurf * gs, double ox, double oy, int rows, int cols,
		 double xres, double yres)
{
    geosurf *ps;
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_init_surf");
    }
#endif

    if (!gs) {
	return (-1);
    }

    for (i = 0; i < MAX_ATTS; i++) {
	gs->att[i].att_src = NOTSET_ATT;
	gs->att[i].att_type = ATTY_INT;
    }

    gs->ox = ox;
    gs->oy = oy;
    gs->rows = rows;
    gs->cols = cols;
    gs->xres = xres;
    gs->yres = yres;
    gs->x_mod = 2;
    gs->y_mod = 2;
    gs->x_modw = rows / 30;
    gs->y_modw = rows / 30;
    gs->xmin = ox;
    gs->xmax = ox + (cols - 1) * xres;
    gs->xrange = gs->xmax - gs->xmin;
    gs->ymin = oy;
    gs->ymax = oy + (rows - 1) * yres;
    gs->yrange = gs->ymax - gs->ymin;
    gs->wire_color = 0x00888888;
    gs->x_trans = gs->y_trans = gs->z_trans = 0.0;
    gs->nz_topo = gs->nz_color = 0;
    gs->norm_needupdate = 1;
    gs->mask_needupdate = 1;
    gs->curmask = NULL;
    gs->norms = NULL;

    if (gs->gsurf_id == FIRST_SURF_ID) {
	gs->z_exag = 1.0;
    }
    else {
	ps = gs_get_prev_surface(gs->gsurf_id);
	gs->z_exag = ps->z_exag;
    }

    return (0);
}

/***********************************************************************/
int gs_init_normbuff(geosurf * gs)
{
    long size;

    if (!gs) {
	return (0);
    }

    if (gs->norms) {
	free(gs->norms);
    }

    size = gs->rows * gs->cols * sizeof(unsigned long);

    if (NULL == (gs->norms = (unsigned long *) malloc(size))) {
	gs_err("gs_init_normbuff");
	return (-1);
    }

    gs->norm_needupdate = 1;

    return (1);
}


/***********************************************************************/
void print_frto(float (*ft)[4])
{
    fprintf(stderr, "FROM: %f, %f, %f\n", ft[FROM][X], ft[FROM][Y],
	    ft[FROM][Z]);
    fprintf(stderr, "TO: %f, %f, %f\n", ft[TO][X], ft[TO][Y], ft[TO][Z]);

    return;
}

/***********************************************************************/
void print_realto(float *rt)
{
    fprintf(stderr, "REAL TO: %f, %f, %f\n", rt[X], rt[Y], rt[Z]);

    return;
}

/***********************************************************************/
void print_256lookup(int *buff)
{
    int i;

    for (i = 0; i < 256; i++) {
	if (!(i % 8)) {
	    fprintf(stderr, "\n");
	}

	fprintf(stderr, "%x ", buff[i]);
    }

    fprintf(stderr, "\n");

    return;
}

/***********************************************************************/
void print_surf_fields(geosurf * s)
{

    fprintf(stderr, "ID: %d\n", s->gsurf_id);
    fprintf(stderr, "rows: %d cols: %d\n", s->rows, s->cols);
    fprintf(stderr, "draw_mode: %x\n", s->draw_mode);
    fprintf(stderr, "wire_color: %lx\n", s->wire_color);
    fprintf(stderr, "ox: %lf oy: %lf\n", s->ox, s->oy);
    fprintf(stderr, "xres: %lf yres: %lf\n", s->xres, s->yres);
    fprintf(stderr, "z_exag: %f \n", s->z_exag);
    fprintf(stderr, "x_trans: %f y_trans: %f z_trans: %f\n",
	    s->x_trans, s->y_trans, s->z_trans);
    fprintf(stderr, "xmin: %f ymin: %f zmin: %f\n",
	    s->xmin, s->ymin, s->zmin);
    fprintf(stderr, "xmax: %f ymax: %f zmax: %f\n",
	    s->xmax, s->ymax, s->zmax);
    fprintf(stderr, "x_mod: %d y_mod: %d x_modw: %d y_modw: %d\n",
	    s->x_mod, s->y_mod, s->x_modw, s->y_modw);

    return;
}

/***********************************************************************/
void print_view_fields(geoview * gv)
{
    fprintf(stderr, "coord_sys: %d\n", gv->coord_sys);
    fprintf(stderr, "view_proj: %d\n", gv->view_proj);
    fprintf(stderr, "infocus: %d\n", gv->infocus);
    print_frto(gv->from_to);
    fprintf(stderr, "twist: %d fov: %d\n", gv->twist, gv->fov);
    fprintf(stderr, "incl: %d look: %d\n", gv->incl, gv->look);
    fprintf(stderr, "real_to: %f %f %f\n",
	    gv->real_to[X], gv->real_to[Y], gv->real_to[Z]);
    fprintf(stderr, "vert_exag: %f scale: %f \n", gv->vert_exag, gv->scale);

    return;
}

/***********************************************************************/
void gs_set_defaults(geosurf * gs, float *defs, float *null_defs)
{
    int i;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_set_defaults");
    }
#endif

    for (i = 0; i < MAX_ATTS; i++) {
	gs->att[i].constant = defs[i];
	gs->att[i].default_null = null_defs[i];
	gs->att[i].lookup = NULL;
	gs->att[i].hdata = -1;
	gs->att[i].att_src = NOTSET_ATT;
    }

    return;
}

/***********************************************************************/
void gs_delete_surf(int id)
{
    geosurf *fs;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_delete_surf");
    }
#endif

    fs = gs_get_surf(id);

    if (fs) {
	gs_free_surf(fs);
    }

    return;
}

/***********************************************************************/
int gs_free_surf(geosurf * fs)
{
    geosurf *gs;
    int found = 0;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_free_surf");
    }
#endif

    if (Surf_top) {
	if (fs == Surf_top) {
	    if (Surf_top->next) {
		/* can't free top if last */
		found = 1;
		Surf_top = fs->next;
	    }
	    else {
		gs_free_unshared_buffs(fs);

		if (fs->curmask) {
		    free(fs->curmask);
		}

		if (fs->norms) {
		    free(fs->norms);
		}

		free(fs);
		Surf_top = NULL;
	    }
	}
	else {
	    for (gs = Surf_top; gs && !found; gs = gs->next) {
		if (gs->next) {
		    if (gs->next == fs) {
			found = 1;
			gs->next = fs->next;
		    }
		}
	    }
	}

	if (found) {
	    gs_free_unshared_buffs(fs);

	    if (fs->curmask) {
		free(fs->curmask);
	    }

	    if (fs->norms) {
		free(fs->norms);
	    }

	    free(fs);
	    fs = NULL;
	}

	return (found);
    }

    return (-1);
}

/***********************************************************************/
/* fs has already been taken out of the list */
/* this function is fairly revealing about how shared datsets work */
void gs_free_unshared_buffs(geosurf * fs)
{
    geosurf *gs;
    int i, j, same;
    int old_datah;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_free_unshared_buffs");
    }
#endif

    /* for each attribute 
       if !same, free buff   
     */
    for (i = 0; i < MAX_ATTS; i++) {
	same = 0;

	if (0 < (old_datah = fs->att[i].hdata)) {
	    /* for ea att of all other surfs */
	    for (gs = Surf_top; gs; gs = gs->next) {
		for (j = 0; j < MAX_ATTS; j++) {
		    if (old_datah == gs->att[j].hdata) {
			same = 1;
		    }
		}
	    }

	    if (!same) {
		gsds_free_datah(old_datah);
	    }
	}
    }

    return;
}

/***********************************************************************/
int gs_num_datah_reused(int dh)
{
    geosurf *gs;
    int ref, j;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_num_datah_reused");
    }
#endif

    /* for each attribute 
       if same, ++reference
     */
    /* for ea att of all surfs */
    ref = 0;

    for (gs = Surf_top; gs; gs = gs->next) {
	for (j = 0; j < MAX_ATTS; j++) {
	    if (dh == gs->att[j].hdata) {
		ref++;
	    }
	}
    }

    return (ref);
}


/***********************************************************************/
int gs_get_att_type(geosurf * gs, int desc)
{

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_att_type");
    }
#endif

    if (!LEGAL_ATT(desc)) {
	return (-1);
    }

    if (gs) {
	if (gs->att[desc].att_src != NOTSET_ATT) {
	    return (gs->att[desc].att_type);
	}
    }

    return (-1);
}

/***********************************************************************/
int gs_get_att_src(geosurf * gs, int desc)
{

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_get_att_src");
    }
#endif

    if (!LEGAL_ATT(desc)) {
	return (-1);
    }

    if (gs) {
	return (gs->att[desc].att_src);
    }

    return (-1);
}

/***********************************************************************/
typbuff *gs_get_att_typbuff(geosurf * gs, int desc, int to_write)
{
    typbuff *tb;
    geosurf *gsref;

    if (gs) {
	if ((tb = gsds_get_typbuff(gs->att[desc].hdata, to_write))) {
	    tb->tfunc = NULL;

	    if (desc == ATT_TOPO) {
		gsref = gsdiff_get_SDref();

		if (gsref && gsref != gs) {
		    tb->tfunc = gsdiff_do_SD;
		}
	    }

	    return (tb);
	}
    }

    return (NULL);
}

/***********************************************************************/
int gs_malloc_att_buff(geosurf * gs, int desc, int type)
{
    int hdata, dims[2], ndims;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_malloc_att_buff");
    }
#endif

    if (gs) {
	if (0 < (hdata = gs->att[desc].hdata)) {
	    dims[0] = gs->rows;
	    dims[1] = gs->cols;
	    ndims = 2;
	    gs_set_att_type(gs, desc, type);

	    return (gsds_alloc_typbuff(hdata, dims, ndims, type));
	}
    }

    return (-1);
}


/***********************************************************************/
int gs_malloc_lookup(geosurf * gs, int desc)
{
    int size;

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_malloc_lookup");
    }
#endif

    if (gs) {
	if (gs->att[desc].lookup) {
	    free(gs->att[desc].lookup);
	    gs->att[desc].lookup = NULL;
	}

	switch (gs->att[desc].att_type) {
	case (ATTY_SHORT):
	    size = 32768 * sizeof(int);

	    /* positive integers only, because use as array index */
	    if (NULL == (gs->att[desc].lookup = (int *) malloc(size))) {
		gs_err("gs_malloc_lookup");

		return (-1);
	    }

	    break;
	case (ATTY_CHAR):
	    size = 256 * sizeof(int);

	    /* unsigned char */
	    if (NULL == (gs->att[desc].lookup = (int *) malloc(size))) {
		gs_err("gs_malloc_lookup");

		return (-1);
	    }

	    break;
	default:
	    gs_err("bad type: gs_malloc_lookup");

	    return (-1);
	}

	if (gs->att[desc].lookup) {
	    return (0);
	}

    }

    return (-1);
}

/***********************************************************************/
int gs_set_att_type(geosurf * gs, int desc, int type)
{

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_set_att_type");
    }
#endif

    if (gs && LEGAL_TYPE(type)) {
	gs->att[desc].att_type = type;

	return (0);
    }

    return (-1);
}

/***********************************************************************/
int gs_set_att_src(geosurf * gs, int desc, int src)
{

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_set_att_src");
    }
#endif

    /* check if old source was MAP_ATT, free buff */
    if (MAP_ATT == gs_get_att_src(gs, desc)) {
	if (1 == gs_num_datah_reused(gs->att[desc].hdata)) {
	    /* only reference */
	    fprintf(stderr, "replacing existing map\n");
	    gsds_free_datah(gs->att[desc].hdata);
	}

	if (ATT_TOPO == desc) {
	    if (gs->norms) {
		free(gs->norms);
	    }

	    gs->norms = NULL;
	    gs->norm_needupdate = 0;
	}
    }

    if (gs && LEGAL_SRC(src)) {
	gs->att[desc].att_src = src;

	return (0);
    }

    return (-1);
}

/***********************************************************************/
/* TODO: set typbuf constant */
int gs_set_att_const(geosurf * gs, int desc, float constant)
{

#ifdef TRACE_FUNCS
    {
	Gs_status("gs_set_att_const");
    }
#endif

    if (gs) {
	gs->att[desc].constant = constant;

	if (ATT_MASK == desc) {
	    gs->mask_needupdate = 1;
	}
	else {
	    gs_set_att_src(gs, desc, CONST_ATT);
	}

	Gs_update_attrange(gs, desc);

	return (0);
    }

    return (-1);
}

/***********************************************************************/
void gs_set_maskmode(int invert)
{
    Invertmask = invert;

    return;
}

/***********************************************************************/
int gs_mask_defined(geosurf * gs)
{
    return (gs->att[ATT_MASK].att_src != NOTSET_ATT);
}

/***********************************************************************/
/* should only be called when setting up the current mask (gs_bm.c) */
int gs_masked(typbuff * tb, int col, int row, int offset)
{
    int ret;

    ret = 1;

    if (tb->bm) {
	ret = BM_get(tb->bm, col, row);
    }
    else if (tb->cb) {
	ret = tb->cb[offset];
    }
    else if (tb->sb) {
	ret = tb->sb[offset];
    }
    else if (tb->ib) {
	ret = tb->ib[offset];
    }
    else if (tb->fb) {
	ret = tb->fb[offset];
    }

    return (Invertmask ? ret : !ret);
}

/***********************************************************************/
/* call this one when you already know att_src is MAP_ATT 
   - returns packed color for catagory at offset */
int gs_mapcolor(typbuff * cobuff, gsurf_att * coloratt, int offset)
{
    if (coloratt->lookup) {
	/* for now, but may add larger color lookup capabilities later,
	   so would have to use GET_MAPATT */
	return (coloratt->lookup[cobuff->cb[offset]]);
    }

    return (cobuff->ib[offset]);
}

/* In the following functions, "extents" refers to translated extents for 
a single surface, while "range" refers to accumulated extents of all
loaded surfaces */

/***********************************************************************/
/* TODO: pass flag to use zminmasked instead of zmin */
int gs_get_zextents(geosurf * gs, float *min, float *max, float *mid)
{
    *min = gs->zmin + gs->z_trans;
    *max = gs->zmax + gs->z_trans;
    *mid = (*max + *min) / 2.;

    return (1);
}

/***********************************************************************/
int gs_get_xextents(geosurf * gs, float *min, float *max)
{
    *min = gs->xmin + gs->x_trans;
    *max = gs->xmax + gs->x_trans;

    return (1);
}

/***********************************************************************/
int gs_get_yextents(geosurf * gs, float *min, float *max)
{
    *min = gs->ymin + gs->y_trans;
    *max = gs->ymax + gs->y_trans;

    return (1);
}


/***********************************************************************/
/* TODO: pass flag to use zminmasked instead of zmin */
/* could also have this return a weighted average for vertical "centroid" */
int gs_get_zrange0(float *min, float *max)
{
    geosurf *gs;

    if (Surf_top) {
	*min = Surf_top->zmin;
	*max = Surf_top->zmax;
    }
    else {
	return (-1);
    }

    for (gs = Surf_top->next; gs; gs = gs->next) {
	if (gs->zmin < *min) {
	    *min = gs->zmin;
	}

	if (gs->zmax > *max) {
	    *max = gs->zmax;
	}
    }

    return (1);
}

/***********************************************************************/
int gs_get_zrange(float *min, float *max)
{
    geosurf *gs;
    float tmin, tmax, tmid;

    if (Surf_top) {
	gs_get_zextents(Surf_top, &tmin, &tmax, &tmid);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gs = Surf_top->next; gs; gs = gs->next) {
	gs_get_zextents(gs, &tmin, &tmax, &tmid);

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
int gs_get_xrange(float *min, float *max)
{
    geosurf *gs;
    float tmin, tmax;

    if (Surf_top) {
	gs_get_xextents(Surf_top, &tmin, &tmax);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gs = Surf_top->next; gs; gs = gs->next) {
	gs_get_xextents(gs, &tmin, &tmax);

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
int gs_get_yrange(float *min, float *max)
{
    geosurf *gs;
    float tmin, tmax;

    if (Surf_top) {
	gs_get_yextents(Surf_top, &tmin, &tmax);
	*min = tmin;
	*max = tmax;
    }
    else {
	return (-1);
    }

    for (gs = Surf_top->next; gs; gs = gs->next) {
	gs_get_yextents(gs, &tmin, &tmax);

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
/* useful for setting position of cplane, lighting ball, etc. */
int gs_get_data_avg_zmax(float *azmax)
{
    float zmax;
    int i;
    geosurf *gs;

    zmax = *azmax = 0.0;

    if (Surf_top) {
	for (i = 0, gs = Surf_top; gs; i++, gs = gs->next) {
	    zmax += (gs->zmax + gs->z_trans);
	}

	*azmax = zmax / i;

	return (1);
    }

    return (-1);
}

/***********************************************************************/
int gs_get_datacenter(float *cen)
{
    float zmin, zmax, ymin, ymax, xmin, xmax;
    geosurf *gs;

    if (Surf_top) {
	zmin = Surf_top->zmin;
	zmax = Surf_top->zmax;
	ymin = Surf_top->ymin;
	ymax = Surf_top->ymax;
	xmin = Surf_top->xmin;
	xmax = Surf_top->xmax;

	for (gs = Surf_top->next; gs; gs = gs->next) {
	    if (gs->zmin < zmin) {
		zmin = gs->zmin;
	    }

	    if (gs->zmax > zmax) {
		zmax = gs->zmax;
	    }

	    if (gs->ymin < ymin) {
		ymin = gs->ymin;
	    }

	    if (gs->ymax > ymax) {
		ymax = gs->ymax;
	    }

	    if (gs->xmin < xmin) {
		xmin = gs->xmin;
	    }

	    if (gs->xmax > xmax) {
		xmax = gs->xmax;
	    }
	}

	cen[X] = (xmin + xmax) / 2. - xmin;
	cen[Y] = (ymin + ymax) / 2. - ymin;
	cen[Z] = (zmin + zmax) / 2.;

	return (1);
    }

    cen[X] = cen[Y] = cen[Z] = 0.0;

    return (-1);
}


/***********************************************************************/
int gs_setall_norm_needupdate(void)
{
    geosurf *gs;

    if (Surf_top) {
	Surf_top->norm_needupdate = 1;
    }
    else {
	return (-1);
    }

    for (gs = Surf_top->next; gs; gs = gs->next) {
	gs->norm_needupdate = 1;
    }

    return (1);
}

/***********************************************************************/
int gs_point_is_masked(geosurf * gs, float *pt)
{
    int vrow, vcol, drow, dcol;
    int retmask = 0, npts = 0;
    float p2[2];

    if (!gs->curmask) {
	return (0);
    }

    vrow = Y2VROW(gs, pt[Y]);
    vcol = X2VCOL(gs, pt[X]);

    /* check right & bottom edges */
    if (pt[X] == VCOL2X(gs, VCOLS(gs))) {
	/* right edge */
	vcol -= 1;
    }

    if (pt[Y] == VROW2Y(gs, VROWS(gs))) {
	/* bottom edge */
	vrow -= 1;
    }

    drow = VROW2DROW(gs, vrow);
    dcol = VCOL2DCOL(gs, vcol);

    if (BM_get(gs->curmask, dcol, drow)) {
	retmask |= MASK_TL;
	npts++;
    }

    dcol = VCOL2DCOL(gs, vcol + 1);

    if (BM_get(gs->curmask, dcol, drow)) {
	retmask |= MASK_TR;
	npts++;
    }

    drow = VROW2DROW(gs, vrow + 1);

    if (BM_get(gs->curmask, dcol, drow)) {
	retmask |= MASK_BR;
	npts++;
    }

    dcol = VCOL2DCOL(gs, vcol);

    if (BM_get(gs->curmask, dcol, drow)) {
	retmask |= MASK_BL;
	npts++;
    }

    if (npts != 1) {
	/* zero or masked */
	return (retmask | npts);
    }

    p2[X] = VCOL2X(gs, vcol);
    p2[Y] = VROW2Y(gs, vrow + 1);

    switch (retmask) {
    case MASK_TL:
	if ((pt[X] - p2[X]) / VXRES(gs) > (pt[Y] - p2[Y]) / VYRES(gs)) {
	    /* lower triangle */
	    return (0);
	}

	return (retmask | npts);
    case MASK_TR:

	return (retmask | npts);
    case MASK_BR:
	if ((pt[X] - p2[X]) / VXRES(gs) <= (pt[Y] - p2[Y]) / VYRES(gs)) {
	    /* upper triangle */
	    return (0);
	}

	return (retmask | npts);
    case MASK_BL:

	return (retmask | npts);
    }

    /* Assume that if we get here it is an error */
    return (0);
}

/***********************************************************************/
int gs_distance_onsurf(geosurf * gs, float *p1, float *p2, float *dist,
		       int use_exag)
{
    Point3 *tmp;
    int np, i;
    float exag, length;

    if (in_vregion(gs, p1) && in_vregion(gs, p2)) {
	if (NULL == (tmp = gsdrape_get_segments(gs, p1, p2, &np))) {
	    return (0);
	}

	length = 0.;

	if (use_exag) {
	    exag = GS_global_exag();
	    tmp[0][Z] *= exag;

	    for (i = 0; i < (np - 1); i++) {
		tmp[i + 1][Z] *= exag;
		length += GS_distance(tmp[i], tmp[i + 1]);
	    }
	}
	else {
	    for (i = 0; i < (np - 1); i++) {
		length += GS_distance(tmp[i], tmp[i + 1]);
	    }
	}

	*dist = length;

	return (1);
    }

    return (0);
}
