/*!
   \file lib/ogsf/gs.c

   \brief OGSF library - loading and manipulating surfaces (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (January 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <stdio.h>

#include <grass/ogsf.h>
#include <grass/glocale.h>

#include "gsget.h"
#include "rowcol.h"

#define FIRST_SURF_ID 110658

static geosurf *Surf_top;
static int Invertmask;

/***********************************************************************/
void gs_err(const char *msg)
{
    G_warning("%s", msg);

    return;
}

/*!
   \brief Initialize library

   Still need to take care of library initialization, 
   probably want to define a Surf_top of constant value (i.e., 0)
 */
void gs_init(void)
{
    Surf_top = NULL;

    return;
}

/*!
   \brief Get geosurf struct

   \param id surface id

   \return pointer to geosurf struct
   \return NULL if not found
 */
geosurf *gs_get_surf(int id)
{
    geosurf *gs;

    G_debug(5, "gs_get_surf():");

    for (gs = Surf_top; gs; gs = gs->next) {
	if (gs->gsurf_id == id) {
	    G_debug(5, "  id=%d", id);
	    return (gs);
	}
    }

    return (NULL);
}

/*!
   \brief Get previous geosurf struct

   \param id current surface id

   \return pointer to geosurf struct
   \return NULL if not found
 */
geosurf *gs_get_prev_surface(int id)
{
    geosurf *ps;

    G_debug(5, "gs_get_prev_surface");

    for (ps = Surf_top; ps; ps = ps->next) {
	if (ps->gsurf_id == id - 1) {
	    return (ps);
	}
    }

    return (NULL);
}

/*!
   \brief Get array of geosurf structs

   \param gsurfs pointer to array

   \return number of geosurfs
 */
int gs_getall_surfaces(geosurf ** gsurfs)
{
    geosurf *gs;
    int i;

    for (i = 0, gs = Surf_top; gs; gs = gs->next, i++) {
	gsurfs[i] = gs;
    }

    G_debug(5, "gs_num_surfaces(): num=%d", i);

    return (i);
}

/*!
   \brief Get number of surfaces

   \return number of surfaces
 */
int gs_num_surfaces(void)
{
    geosurf *gs;
    int i;

    for (i = 0, gs = Surf_top; gs; gs = gs->next, i++) ;

    G_debug(5, "gs_num_surfaces(): num=%d", i);

    return (i);
}


/*!
   \brief Check if attribute is set

   \param surf pointer to gsurf or NULL to look at all geosurfs
   \param att attribute id

   \return 1 attribute is set up
   \return 0 attribute is not set up
 */
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

/*!
   \brief Get last allocated geosurf struct from list

   \return pointer to geosurf struct
 */
geosurf *gs_get_last_surface(void)
{
    geosurf *ls;

    if (!Surf_top) {
	return (NULL);
    }

    for (ls = Surf_top; ls->next; ls = ls->next) ;

    G_debug(5, "gs_get_last_surface(): last surface id=%d", ls->gsurf_id);

    return (ls);
}


/*!
   \brief Allocate new geosurf struct

   \return pointer to geosurf struct
 */
geosurf *gs_get_new_surface(void)
{
    geosurf *ns, *ls;

    ns = (geosurf *) G_malloc(sizeof(geosurf));	/* G_fatal_error */
    if (!ns) {
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

    G_debug(5, "gs_get_new_surface(): id=%d", ns->gsurf_id);

    return (ns);
}

/*!
   \brief Initialize allocated geosurf struct

   \todo Now xmin & ox are the same, right? - get rid of ox, oy in geosurf struct?

   \param gs pointer to geosurf struct
   \param ox,oy x/y origin coordinates
   \param rows number of rows
   \param cols number of cols
   \param xres,yres x/y resolution value

   \return -1 on error
   \return 0 on success
 */
int gs_init_surf(geosurf * gs, double ox, double oy, int rows, int cols,
		 double xres, double yres)
{
    geosurf *ps;
    int i;

    if (!gs) {
	return (-1);
    }

    G_debug(5, "gs_init_surf() id=%d", gs->gsurf_id);

    /* default attributes */
    for (i = 0; i < MAX_ATTS; i++) {
	gs->att[i].att_src = NOTSET_ATT;
	gs->att[i].att_type = ATTY_INT;
	gs->att[i].hdata = -1;
	gs->att[i].user_func = NULL;
	gs->att[i].constant = 0.;
	gs->att[i].lookup = NULL;
	gs->att[i].min_nz = gs->att[i].max_nz = gs->att[i].range_nz = 0;
	gs->att[i].default_null = 0.;
    }

    /* default values */
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
    gs->zmin = gs->zmin_nz = gs->zminmasked = 0;
    gs->zmax = gs->zmax_nz = 0;
    gs->zrange = gs->zrange_nz = 0;
    gs->wire_color = 0x00888888;
    gs->x_trans = gs->y_trans = gs->z_trans = 0.0;
    gs->nz_topo = gs->nz_color = 0;
    gs->norm_needupdate = 1;
    gs->mask_needupdate = 1;
    gs->curmask = NULL;
    gs->norms = NULL;

    gs->draw_mode = DM_GOURAUD;

    /* default z_exag value */
    if (gs->gsurf_id == FIRST_SURF_ID) {
	gs->z_exag = 1.0;
    }
    else {
	ps = gs_get_prev_surface(gs->gsurf_id);
	gs->z_exag = ps->z_exag;
    }

    return (0);
}

/*!
   \brief Init geosurf normbuff

   \param gs pointer to geosurf struct

   \return 0 on error
   \return 1 on success
 */
int gs_init_normbuff(geosurf * gs)
{
    long size;

    if (!gs) {
	return (0);
    }

    if (gs->norms) {
	G_free(gs->norms);
    }

    size = gs->rows * gs->cols * sizeof(unsigned long);

    gs->norms = (unsigned long *)G_malloc(size);	/* G_fatal_error */
    if (!gs->norms) {
	return (-1);
    }

    gs->norm_needupdate = 1;

    return (1);
}

/*!
   \brief Debugging, print 'from/to' model coordinates to stderr

   \todo G_debug ?

   \param ft pointer to coordinates
 */
void print_frto(float (*ft)[4])
{
    fprintf(stderr, "FROM: %f, %f, %f\n", ft[FROM][X], ft[FROM][Y],
	    ft[FROM][Z]);
    fprintf(stderr, "TO: %f, %f, %f\n", ft[TO][X], ft[TO][Y], ft[TO][Z]);

    return;
}

/*!
   \brief Debugging, print 'to' real coordinates to stderr

   \todo G_debug ?

   \param ft pointer to coordinates
 */
void print_realto(float *rt)
{
    fprintf(stderr, "REAL TO: %f, %f, %f\n", rt[X], rt[Y], rt[Z]);

    return;
}

/*!
   \brief Debugging, 256 integer values from buffer

   \todo G_debug ?

   \param ft pointer to buffer
 */
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

/*!
   \brief Debugging, print geosurf fields to stderr

   \todo G_debug ?

   \param s pointer to geosurf struct
 */
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

/*!
   \brief Debugging, print geoview fields to stderr

   \todo G_debug ?

   \param gv pointer to geoview struct
 */
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

/*!
   \brief Set default attribute values

   \param gs pointer to geosurf struct
   \param defs array of default values (dim MAX_ATTRS)
   \param null_defs array of null default values (dim MAX_ATTRS)
 */
void gs_set_defaults(geosurf * gs, float *defs, float *null_defs)
{
    int i;

    G_debug(5, "gs_set_defaults(): id=%d", gs->gsurf_id);

    for (i = 0; i < MAX_ATTS; i++) {
	gs->att[i].constant = defs[i];
	gs->att[i].default_null = null_defs[i];
	gs->att[i].lookup = NULL;
	gs->att[i].hdata = -1;
	gs->att[i].att_src = NOTSET_ATT;
    }

    return;
}

/*!
   \brief Remove geosurf struct from list

   \param id surface id
 */
void gs_delete_surf(int id)
{
    geosurf *fs;

    G_debug(5, "gs_delete_surf");

    fs = gs_get_surf(id);

    if (fs) {
	gs_free_surf(fs);
    }

    return;
}

/*!
   \brief Free geosurf struct

   \param fs pointer to geosurf struct

   \return 1 found
   \return 0 not found
   \return -1 on error
 */
int gs_free_surf(geosurf * fs)
{
    geosurf *gs;
    int found = 0;

    G_debug(5, "gs_free_surf");

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
		    G_free(fs->curmask);
		}

		if (fs->norms) {
		    G_free(fs->norms);
		}

		G_free(fs);
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
		G_free(fs->curmask);
	    }

	    if (fs->norms) {
		G_free(fs->norms);
	    }

	    G_free(fs);
	    fs = NULL;
	}

	return (found);
    }

    return (-1);
}

/*!
   \brief Free unshared buffers of geosurf struct

   <i>fs</i> has already been taken out of the list

   This function is fairly revealing about how shared datsets work

   \param fs pointer to geosurf struct
 */
void gs_free_unshared_buffs(geosurf * fs)
{
    geosurf *gs;
    int i, j, same;
    int old_datah;

    G_debug(5, "gs_free_unshared_buffs");

    /* for each attribute 
       if !same, free buff   
     */
    for (i = 0; i < MAX_ATTS; i++) {
	same = 0;

	if (0 < (old_datah = fs->att[i].hdata)) {
	    /* for ea att of all other surfs */
	    for (gs = Surf_top; gs; gs = gs->next) {
		for (j = 0; j < MAX_ATTS; j++) {
		    if ((old_datah == gs->att[j].hdata) && (fs != gs)) {
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

/*!
   \brief Get number of reused values

   \param dh value

   \return number of reused values
 */
int gs_num_datah_reused(int dh)
{
    geosurf *gs;
    int ref, j;

    G_debug(5, "gs_num_datah_reused");

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

/*!
   \brief Get attribute type

   \param gs pointer to geosurf struct
   \param desc attribute id

   \return -1 on error
   \return attribute type
 */
int gs_get_att_type(geosurf * gs, int desc)
{
    G_debug(5, "gs_get_att_type");

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

/*!
   \brief Get attribute source

   \param gs pointer to geosurf struct
   \param desc attribute id (descriptor)

   \return -1 on error
   \return attribute source id
 */
int gs_get_att_src(geosurf * gs, int desc)
{
    if (gs)
	G_debug(5, "gs_get_att_src(): id=%d, desc=%d", gs->gsurf_id, desc);
    if (!LEGAL_ATT(desc)) {
	return (-1);
    }

    if (gs) {
	return (gs->att[desc].att_src);
    }

    return (-1);
}

/*!
   \brief Get attribute data buffer

   \param gs pointer to geosurf struct
   \param desc attribute id (descriptor)
   \param to_write non-zero value for 'write'

   \return NULL on error
   \return pointer to typbuff
 */
typbuff *gs_get_att_typbuff(geosurf * gs, int desc, int to_write)
{
    typbuff *tb;
    geosurf *gsref;

    if (gs) {
	G_debug(5, "gs_get_att_typbuff(): id=%d desc=%d to_write=%d",
		gs->gsurf_id, desc, to_write);
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

/*!
   \brief Allocate attribute buffer

   \param gs pointer to geosurf struct
   \param desc attribute id (descriptor)
   \param type buffer type (based on raster map type)

   \return -1 on error
   \return amount of allocated memory
 */
size_t gs_malloc_att_buff(geosurf * gs, int desc, int type)
{
    int hdata, dims[2], ndims;

    G_debug(5, "gs_malloc_att_buff");

    if (gs) {
	if (0 < (hdata = gs->att[desc].hdata)) {
	    dims[0] = gs->rows;
	    dims[1] = gs->cols;
	    ndims = 2;
	    gs_set_att_type(gs, desc, type);

	    return (gsds_alloc_typbuff(hdata, dims, ndims, type));
	}
    }

    return 0;
}

/*!
   \brief Allocate attribute lookup

   \param gs pointer to geosurf struct
   \param desc attribute id

   \return -1 on error
   \return pointer to typbuff (casted)
 */
int gs_malloc_lookup(geosurf * gs, int desc)
{
    int size;

    G_debug(5, "gs_malloc_lookup");

    if (gs) {
	if (gs->att[desc].lookup) {
	    G_free(gs->att[desc].lookup);
	    gs->att[desc].lookup = NULL;
	}

	switch (gs->att[desc].att_type) {
	case (ATTY_SHORT):
	    size = 32768 * sizeof(int);

	    /* positive integers only, because use as array index */
	    gs->att[desc].lookup = (int *)G_malloc(size);	/* G_fatal_error */
	    if (!gs->att[desc].lookup) {
		return (-1);
	    }

	    break;
	case (ATTY_CHAR):
	    size = 256 * sizeof(int);

	    /* unsigned char */
	    gs->att[desc].lookup = (int *)G_malloc(size);
	    if (!gs->att[desc].lookup) {
		return (-1);
	    }

	    break;
	default:
	    G_warning("bad type: gs_malloc_lookup");
	    return (-1);
	}

	if (gs->att[desc].lookup) {
	    return (0);
	}

    }

    return (-1);
}

/*!
   \brief Set attribute type

   \param gs pointer to geosurf struct
   \param desc attribute id
   \param type attribute type

   \return -1 on error
   \return 0 on success
 */
int gs_set_att_type(geosurf * gs, int desc, int type)
{

    G_debug(5, "gs_set_att_type(): desc=%d, type=%d", desc, type);

    if (gs && LEGAL_TYPE(type)) {
	gs->att[desc].att_type = type;

	return (0);
    }

    return (-1);
}

/*!
   \brief Set attribute source

   \param gs pointer to geosurf struct
   \param desc attribute id (descriptor)
   \param src source id

   \return -1 on error
   \return 0 on success
 */
int gs_set_att_src(geosurf * gs, int desc, int src)
{
    if (gs)
	G_debug(5, "gs_set_att_src(): id=%d desc=%d src=%d",
		gs->gsurf_id, desc, src);

    /* check if old source was MAP_ATT, free buff */
    if (MAP_ATT == gs_get_att_src(gs, desc)) {
	if (1 == gs_num_datah_reused(gs->att[desc].hdata)) {
	    /* only reference */
	    G_debug(5, "gs_set_att_src(): replacing existing map");
	    gsds_free_datah(gs->att[desc].hdata);
	}

	if (ATT_TOPO == desc) {
	    if (gs->norms) {
		G_free(gs->norms);
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

/*!
   \brief Set attribute constant value

   \todo set typbuf constant

   \param gs pointer to geosurf struct
   \param desc attribute id
   \param constant constant value

   \return 0 on success
   \return -1 on error
 */
int gs_set_att_const(geosurf * gs, int desc, float constant)
{

    if (gs) {
	G_debug(5, "gs_set_att_const(): id=%d, desc=%d, const=%f",
		gs->gsurf_id, desc, constant);
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

/*!
   \brief Set geosurf mask mode

   \param invert invert mask
 */
void gs_set_maskmode(int invert)
{
    Invertmask = invert;

    return;
}

/*!
   \brief Check if mask is defined

   \param gs pointer to geosurf struct

   \return 1 if defined
   \return 0 not defined
 */
int gs_mask_defined(geosurf * gs)
{
    return (gs->att[ATT_MASK].att_src != NOTSET_ATT);
}

/*!
   \brief

   Should only be called when setting up the current mask (gs_bm.c)

   \param tb pointer to typbuff
   \param col number of cols
   \param row number of rows
   \param offset offset value

   \return 1
   \return 0
 */
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

/*!
   \brief

   Call this one when you already know att_src is MAP_ATT 

   \param cobuff
   \param coloratt color attribute
   \param offset offset value

   \return packed color for category at offset
 */
int gs_mapcolor(typbuff * cobuff, gsurf_att * coloratt, int offset)
{
    if (coloratt->lookup) {
	/* for now, but may add larger color lookup capabilities later,
	   so would have to use GET_MAPATT */
	return (coloratt->lookup[cobuff->cb[offset]]);
    }

    return (cobuff->ib[offset]);
}

/*
   In the following functions, "extents" refers to translated extents for 
   a single surface, while "range" refers to accumulated extents of all
   loaded surfaces
 */

/*!
   \brief Get z-extent values

   \todo pass flag to use zminmasked instead of zmin

   \param gs pointer to geosurf struct
   \param[out] min z-min value
   \param[out] max z-max value
   \param[out] mid z-middle value

   \return 1
 */
int gs_get_zextents(geosurf * gs, float *min, float *max, float *mid)
{
    *min = gs->zmin + gs->z_trans;
    *max = gs->zmax + gs->z_trans;
    *mid = (*max + *min) / 2.;

    return (1);
}

/*!
   \brief Get x-extent values

   \param gs pointer to geosurf struct
   \param[out] min x-min value
   \param[out] max x-max value

   \return 1
 */
int gs_get_xextents(geosurf * gs, float *min, float *max)
{
    *min = gs->xmin + gs->x_trans;
    *max = gs->xmax + gs->x_trans;

    return (1);
}

/*!
   \brief Get y-extent values

   \param gs pointer to geosurf struct
   \param[out] min y-min value
   \param[out] max y-max value

   \return 1
 */
int gs_get_yextents(geosurf * gs, float *min, float *max)
{
    *min = gs->ymin + gs->y_trans;
    *max = gs->ymax + gs->y_trans;

    return (1);
}


/*!
   \brief Get z-range

   \todo pass flag to use zminmasked instead of zmin
   could also have this return a weighted average for vertical "centroid"

   \param[out] min z-min value
   \param[out] max z-max value

   \return -1 on error (no surface)
   \return 1 on success
 */
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

/*!
   \brief Get z-range

   \param[out] min z-min value
   \param[out] max z-max value

   \return -1 on error (no surface)
   \return 1 on success
 */
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

/*!
   \brief Get x-range

   \param[out] min x-min value
   \param[out] max x-max value

   \return -1 on error (no surface)
   \return 1 on success
 */
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

/*!
   \brief Get y-range

   \param[out] min y-min value
   \param[out] max y-max value

   \return -1 on error (no surface)
   \return 1 on success
 */
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

/*!
   \brief Get average z-max value

   Useful for setting position of cplane, lighting ball, etc.

   \param[out] azmax average z-max value

   \return -1 on error
   \return 1 on success
 */
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

/*!
   \brief Get data center point

   \param[out] center (array X,Y,Z)

   \return -1 on error
   \return 1 on success
 */
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


/*!
   \brief Set for geosurf need-to-update mark

   \return -1 no surface available
   \return 1 on success
 */
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

/*!
   \brief Check if point is masked

   \param gs pointer to geosurf struct
   \param pt point coordinates (X,Y,Z)

   \return 1 masked
   \return 0 not masked
 */
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

/*!
   \brief Calculate distance on surface

   \param gs pointer to geosurf struct
   \param p1 from point
   \param p2 to point
   \param[out] dist distnace
   \param use_exag use exag for calculation

   \return 0 on error (points not in region)
   \return 1 on success
 */
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
