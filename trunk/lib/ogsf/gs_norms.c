/*!
   \file lib/ogsf/gs_norms.c

   \brief OGSF library - calculation normals (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "gsget.h"
#include "rowcol.h"

#define NTOP 0x00001000
#define NBOT 0x00000100
#define NLFT 0x00000010
#define NRGT 0x00000001

#define NALL 0x00001111

#define NTL  0x00001010
#define NTR  0x00001001
#define NBL  0x00000110
#define NBR  0x00000101

/*!
   \brief This macro is only used in the function calc_norm()
 */
#define SET_NORM(i) \
       dz1 = z1 - z2; \
       dz2 = z3 - z4; \
       temp[0] = (float) -dz1 * y_res_z2; \
       temp[1] = (float) dz2 * x_res_z2; \
       temp[2] = c_z2; \
       normalizer = sqrt(temp[0] * temp[0] + temp[1] * temp[1] + c_z2_sq); \
       if (!normalizer) normalizer= 1.0; \
       temp[X] /= normalizer; \
       temp[Y] /= normalizer; \
       temp[Z] /= normalizer; \
       PNORM(i,temp);

static long slice;
static float x_res_z2, y_res_z2;
static float c_z2, c_z2_sq;
static typbuff *elbuf;
static unsigned long *norm;

/*
   #define USE_GL_NORMALIZE
 */

/*!
   \brief Init variables

   for optimization

   \param gs surface (geosurf)
 */
void init_vars(geosurf * gs)
{
    /* optimized - these are static - global to this file */
    norm = gs->norms;
    elbuf = gs_get_att_typbuff(gs, ATT_TOPO, 0);

#ifdef USE_GL_NORMALIZE
    c_z2 =
	2.0 * gs->xres * gs->yres * gs->x_mod * gs->y_mod / GS_global_exag();
    c_z2_sq = c_z2 * c_z2;
    x_res_z2 = 2.0 * gs->xres * gs->z_exag * gs->x_mod;
    y_res_z2 = 2.0 * gs->yres * gs->z_exag * gs->y_mod;
#else

    {
	float sx, sy, sz;

	GS_get_scale(&sx, &sy, &sz, 1);

	c_z2 = 2.0 * gs->xres * gs->yres * gs->x_mod * gs->y_mod;
	c_z2_sq = c_z2 * c_z2;
	x_res_z2 = 2.0 * gs->xres * gs->z_exag * gs->x_mod;
	y_res_z2 = 2.0 * gs->yres * gs->z_exag * gs->y_mod;
    }
#endif

    slice = gs->y_mod * gs->cols;

    return;
}

/*!
   \brief Calculate normals

   OPTIMIZED for constant dy & dx

   The norm array is always the same size, but diff resolutions
   force resampled data points to have their normals recalculated,
   then only those norms are passed to n3f during drawing.
   Norms are converted to a packed unsigned int for storage,
   must be converted back at time of use.

   \todo fix to correctly calculate norms when mapped to sphere!

   Uses the previous and next cells (when available) for normal 
   calculations to produce smoother normals

   \param gs surface (geosurf)

   \return 1 on success
   \return 0 on failure
 */
int gs_calc_normals(geosurf * gs)
{
    int row, col;
    int xcnt, ycnt;
    int xmod, ymod;

    if (!gs->norm_needupdate || !gs->norms) {
	return (0);
    }

    gs->norm_needupdate = 0;
    gs_update_curmask(gs);

    xmod = gs->x_mod;
    ymod = gs->y_mod;

    xcnt = VCOLS(gs);
    ycnt = VROWS(gs);

    init_vars(gs);

    G_debug(5, "gs_calc_normals(): id=%d", gs->gsurf_id);

    /* first row - just use single cell */
    /* first col - use bottom & right neighbors */
    calc_norm(gs, 0, 0, NBR);

    for (col = 1; col < xcnt; col++) {
	/* turn off top neighbor for first row */
	calc_norm(gs, 0, col * xmod, ~NTOP);
    }

    /* use bottom & left neighbors for last col */
    calc_norm(gs, 0, col * xmod, NBL);

    /* now use four neighboring points for rows 1 - (n-1) */
    for (row = 1; row < ycnt; row++) {
	if (!(row % 100))
	    G_debug(5, "gs_calc_normals(): row=%d", row);

	/* turn off left neighbor for first col */
	calc_norm(gs, row * ymod, 0, ~NLFT);

	/* use all 4 neighbors until last col */
	for (col = 1; col < xcnt; col++) {
	    calc_norm(gs, row * ymod, col * xmod, NALL);
	}

	/* turn off right neighbor for last col */
	calc_norm(gs, row * ymod, col * xmod, ~NRGT);
    }

    /* last row */
    /* use top & right neighbors for first col */
    calc_norm(gs, row * ymod, 0, NTR);

    for (col = 1; col < xcnt; col++) {
	/* turn off bottom neighbor for last row */
	calc_norm(gs, row * ymod, col * xmod, ~NBOT);
    }

    /* use top & left neighbors for last column */
    calc_norm(gs, row * ymod, col * xmod, NTL);

    return (1);
}

/*!
   \brief Calculate normals

   Need either four neighbors or two non-linear neighbors
   passed initial state of neighbors known from array position
   and data row & col

   \param gs surface (geosurf)
   \param drow data row
   \param dcol data col
   \param neighbors neighbors id

   \return 0 no normals
   \return 1 on success
 */
int calc_norm(geosurf * gs, int drow, int dcol, unsigned int neighbors)
{
    long noffset;
    float temp[3], normalizer, dz1, dz2, z0, z1, z2, z3, z4;

    if (gs->curmask) {
	/* need to check masked neighbors */
	/* NOTE: this should automatically eliminate nullvals */
	if (neighbors & NTOP) {
	    if (BM_get(gs->curmask, dcol, drow - gs->y_mod)) {
		/* masked */
		neighbors &= ~NTOP;
	    }
	}

	if (neighbors & NBOT) {
	    if (BM_get(gs->curmask, dcol, drow + gs->y_mod)) {
		/* masked */
		neighbors &= ~NBOT;
	    }
	}

	if (neighbors & NLFT) {
	    if (BM_get(gs->curmask, dcol - gs->x_mod, drow)) {
		/* masked */
		neighbors &= ~NLFT;
	    }
	}

	if (neighbors & NRGT) {
	    if (BM_get(gs->curmask, dcol + gs->x_mod, drow)) {
		/* masked */
		neighbors &= ~NRGT;
	    }
	}
    }

    if (!neighbors) {
	/* none */
	return (0);
    }

    noffset = DRC2OFF(gs, drow, dcol);

    if (!GET_MAPATT(elbuf, noffset, z0)) {
	return (0);
    }

    z1 = z2 = z3 = z4 = z0;

    /* we know these aren't null now, maybe use faster GET_MAPATT? */
    if (neighbors & NRGT) {
	GET_MAPATT(elbuf, noffset + gs->x_mod, z1);
	if (!(neighbors & NLFT)) {
	    z2 = z0 + (z0 - z1);
	}
    }

    if (neighbors & NLFT) {
	GET_MAPATT(elbuf, noffset - gs->x_mod, z2);

	if (!(neighbors & NRGT)) {
	    z1 = z0 + (z0 - z2);
	}
    }

    if (neighbors & NTOP) {
	GET_MAPATT(elbuf, noffset - slice, z4);

	if (!(neighbors & NBOT)) {
	    z3 = z0 + (z0 - z4);
	}
    }

    if (neighbors & NBOT) {
	GET_MAPATT(elbuf, noffset + slice, z3);

	if (!(neighbors & NTOP)) {
	    z4 = z0 + (z0 - z3);
	}
    }

    SET_NORM(norm[noffset]);

    return (1);
}
