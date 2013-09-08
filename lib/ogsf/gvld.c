/*!
   \file gvld.c

   \brief OGSF library - loading and manipulating volumes (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Tomas Paudits (February 2004)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "mc33_table.h"

/* usefull macros */
#define READ() gvl_read_char(pos[i]++, gvl->isosurf[i]->data)

/*!
   \brief Draw volume set (slices and isosurfaces)

   \param gvl pointer to geovol struct

   \return -1 on failure
   \return 1 on success
 */
int gvld_vol(geovol *gvl)
{
    G_debug(5, "gvld_vol(): id=%d", gvl->gvol_id);

    /* SLICES */
    /* calculate slices data, if slices changed */
    if (0 > gvl_slices_calc(gvl))
	return (-1);
    /* draw slices */
    if (0 > gvld_slices(gvl))
	return (-1);

    /* ISOSURFACES */
    /* calculate isosurfaces data, if isosurfaces changed */
    if (0 > gvl_isosurf_calc(gvl))
	return (-1);
    /* draw isosurfaces */
    if (0 > gvld_isosurf(gvl))
	return (-1);

    return (1);
}

/*!
   \brief Draw volume in wire mode (bounding box)

   \param gvl pointer to geovol struct

   \return -1 on failure
   \return 1 on success
 */
int gvld_wire_vol(geovol * gvl)
{
    G_debug(5, "gvld_wire_vol(): id=%d", gvl->gvol_id);

    gvld_wind3_box(gvl);

    if (0 > gvld_wire_slices(gvl))
	return (-1);

    if (0 > gvld_wire_isosurf(gvl))
	return (-1);

    return (1);
}

/*!
   \brief Draw volume isosurfaces

   \param gvl pointer to geovol struct

   \return -1 on failure
   \return 1 on success
 */
int gvld_isosurf(geovol * gvl)
{
    float tx, ty, tz;
    int cols, rows, depths;
    int x, y, z, i, iv;
    float xc, yc, zc;
    float xres, yres, zres;
    unsigned r, g, b;

    int j, p, num, c_ndx, crnt_ev;
    float n[3], pt[4];

    int n_i = gvl->n_isosurfs;

    int *check_color, *check_transp, *check_material, *check_emis,
	*check_shin;
    float *kem, *ksh, pkem, pksh;
    unsigned int *ktrans, *curcolor;
    int pktransp = 0;

    int *pos, *nz, *e_dl, tmp_pos, edge_pos[13];

    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    geovol_isosurf *isosurf;

    /* Allocate memory for arrays */

    check_color = G_malloc(n_i * sizeof(int));
    check_transp = G_malloc(n_i * sizeof(int));
    check_material = G_malloc(n_i * sizeof(int));
    check_emis = G_malloc(n_i * sizeof(int));
    check_shin = G_malloc(n_i * sizeof(int));

    kem = G_malloc(n_i * sizeof(float));
    ksh = G_malloc(n_i * sizeof(float));

    ktrans = G_malloc(n_i * sizeof(unsigned int));
    curcolor = G_malloc(n_i * sizeof(unsigned int));

    pos = G_malloc(n_i * sizeof(int));
    nz = G_malloc(n_i * sizeof(int));
    e_dl = G_malloc(n_i * sizeof(int));

    G_debug(5, "gvld_isosurf():");
    for (i = 0; i < gvl->n_isosurfs; i++) {
	G_debug(5, "  start : gvl: %s isosurf : %d\n",
		gvl_file_get_name(gvl->hfile), i);
    }

    /* shade */
    gsd_shademodel(gvl->isosurf_draw_mode & DM_GOURAUD);

    /* scaling */
    GS_get_scale(&tx, &ty, &tz, 1);

    /* set number of cols, rows, dephs */
    cols = gvl->cols / gvl->isosurf_x_mod;
    rows = gvl->rows / gvl->isosurf_y_mod;
    depths = gvl->depths / gvl->isosurf_z_mod;

    /* set x,y,z resolution */
    xres =
	/*((float) gvl->cols) / ((float) cols) */ gvl->isosurf_x_mod *
	gvl->xres;
    yres =
	/*((float) gvl->rows) / ((float) rows) */ gvl->isosurf_y_mod *
	gvl->yres;
    zres =
	/*((float) gvl->depths) / ((float) depths) */ gvl->isosurf_z_mod *
	gvl->zres;

    /* get viewport */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);

    /* adjust window */
    window[0] += (int)(yres * 2);
    window[1] -= (int)(yres * 2);
    window[2] -= (int)(xres * 2);
    window[3] += (int)(xres * 2);

    gsd_colormode(CM_DIFFUSE);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(gvl->x_trans, gvl->y_trans, gvl->z_trans);

    pkem = 1.0;
    pksh = 1.0;

    /* set default attribute values for isosurfaces */
    for (i = 0; i < gvl->n_isosurfs; i++) {
	isosurf = gvl->isosurf[i];

	/* init isosurf one cube edge datalength */
	e_dl[i] = 4;

	/* transparency */
	check_transp[i] = 0;
	ktrans[i] = (255 << 24);
	if (CONST_ATT == isosurf->att[ATT_TRANSP].att_src &&
	    isosurf->att[ATT_TRANSP].constant != 0.0) {
	    ktrans[i] = (255 - (int)isosurf->att[ATT_TRANSP].constant) << 24;
	}
	else if (MAP_ATT == isosurf->att[ATT_TRANSP].att_src) {
	    check_transp[i] = 1;
	    e_dl[i]++;
	}

	/* emis */
	check_emis[i] = 0;
	kem[i] = 0.0;
	if (CONST_ATT == isosurf->att[ATT_EMIT].att_src) {
	    kem[i] = isosurf->att[ATT_EMIT].constant / 255.;
	}
	else if (MAP_ATT == isosurf->att[ATT_EMIT].att_src) {
	    check_emis[i] = 1;
	    e_dl[i]++;
	}

	/* shin */
	check_shin[i] = 0;
	ksh[i] = 0.0;
	if (CONST_ATT == isosurf->att[ATT_SHINE].att_src) {
	    ksh[i] = isosurf->att[ATT_SHINE].constant / 255.;
	}
	else if (MAP_ATT == isosurf->att[ATT_SHINE].att_src) {
	    check_shin[i] = 1;
	    e_dl[i]++;
	}

	/* color */
	check_color[i] = 0;
	curcolor[i] = 0.0;
	if (CONST_ATT == isosurf->att[ATT_COLOR].att_src) {
	    curcolor[i] = (int)isosurf->att[ATT_COLOR].constant;
	}
	else if (MAP_ATT == isosurf->att[ATT_COLOR].att_src) {
	    check_color[i] = 1;
	    e_dl[i] += 3;
	}

	/* check material */
	check_material[i] = (check_shin[i] || check_emis[i] ||
			     (kem[i] && check_color[i]));

	/* set position in data */
	pos[i] = 0;
	nz[i] = 0;
    }

    G_debug(5, "  intialize OK");

    for (z = 0; z < depths - 1; z++) {
	zc = z * zres;

	if (GS_check_cancel()) {
	    for (i = 0; i < gvl->n_isosurfs; i++) {
		G_debug(5, "  break : isosurf : %d datalength : %d B\n",
			i, pos[i]);
	    }

	    gsd_set_material(1, 1, 0., 0., 0x0);
	    gsd_popmatrix();
	    gsd_blend(0);
	    gsd_zwritemask(0xffffffff);

	    return (-1);
	}

	for (y = 0; y < rows - 1; y++) {
	    yc = ((rows - 1) * yres) - (y * yres);

	    for (x = 0; x < cols - 1; x++) {
		xc = x * xres;

		for (i = 0; i < gvl->n_isosurfs; i++) {

		    /* read cube index */
		    if (nz[i] != 0) {
			nz[i]--;
			continue;
		    }
		    else {
			c_ndx = READ();
			if (c_ndx == 0) {
			    nz[i] = READ() - 1;
			    continue;
			}
			else {
			    c_ndx = (c_ndx - 1) * 256 + READ();
			}
		    }

		    /* save position */
		    tmp_pos = pos[i];

		    /* set position for each cube edge data */
		    iv = 0;
		    for (j = 0; j < cell_table[c_ndx].nedges; j++) {
			if (cell_table[c_ndx].edges[j] == 12)
			    iv = 1;

			edge_pos[cell_table[c_ndx].edges[j]] =
			    pos[i] + j * e_dl[i];
		    }

		    /* enable/disable blending and depth buffer writing */
		    if (check_transp[i] || (ktrans[i] >> 24) < 255) {
			if (!pktransp) {
			    gsd_blend(1);
			    gsd_zwritemask(0);
			}
		    }
		    else if (pktransp) {
			gsd_blend(0);
			gsd_zwritemask(0xffffffff);
			pktransp = 0;
		    }

		    /* draw cube polygons */
		    for (p = 0, num = 0; num < cell_table[c_ndx].npolys;
			 num++) {
			gsd_bgnpolygon();

			for (j = 0; j < 3; j++) {
			    crnt_ev = cell_table[c_ndx].polys[p];
			    /* set position in data to current edge data */
			    pos[i] = edge_pos[crnt_ev];

			    /* triagle vertex */
			    if (crnt_ev == 12) {
				pt[X] = xc + (READ() / 255. * xres);
				pt[Y] = yc + (-(READ() / 255. * yres));
				pt[Z] = zc + (READ() / 255. * zres);
			    }
			    else {
				pt[edge_vert_pos[crnt_ev][0]] = READ() / 255.;
				pt[edge_vert_pos[crnt_ev][1]] =
				    edge_vert_pos[crnt_ev][2];
				pt[edge_vert_pos[crnt_ev][3]] =
				    edge_vert_pos[crnt_ev][4];

				pt[X] = xc + (pt[X] * xres);
				pt[Y] = yc + (-(pt[Y] * yres));
				pt[Z] = zc + (pt[Z] * zres);
			    }

			    n[X] = (READ() / 127. - 1.) / xres;
			    n[Y] = (-(READ() / 127. - 1.)) / yres;
			    n[Z] = (READ() / 127. - 1.) / zres;

			    if (gvl->isosurf[i]->inout_mode) {
				n[X] *= -1;
				n[Y] *= -1;
				n[Z] *= -1;
			    }

			    if (check_color[i]) {
				r = READ();
				g = READ();
				b = READ();
				curcolor[i] =
				    (r & 0xff) | ((g & 0xff) << 8) |
				    ((b & 0xff) << 16);
			    }

			    if (check_transp[i])
				ktrans[i] = READ() << 24;;

			    if (check_shin[i])
				ksh[i] = ((float)READ()) / 255.;

			    if (check_emis[i])
				kem[i] = ((float)READ()) / 255.;

			    if (pksh != ksh[i] || pkem != kem[i] ||
				(kem[i] && check_color[i])) {
				pksh = ksh[i];
				pkem = kem[i];
				gsd_set_material(1, 1, ksh[i], kem[i],
						 curcolor[i]);
			    }

			    gsd_litvert_func(n, ktrans[i] | curcolor[i], pt);
			    p++;
			}

			gsd_endpolygon();
		    }

		    /* set position to next cube */
		    pos[i] =
			tmp_pos + cell_table[c_ndx].nedges * e_dl[i] +
			(iv ? 2 : 0);
		}

	    }

	}

    }

    for (i = 0; i < gvl->n_isosurfs; i++) {
	G_debug(5, "  end : isosurf : %d datalength : %d B\n", i, pos[i]);
    }

    gsd_set_material(1, 1, 0., 0., 0x0);
    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (0);
}

/*!
   \brief Draw volume isosurface in draw mode

   \param gvl pointer to geovol struct

   \return 0
 */
int gvld_wire_isosurf(geovol * gvl)
{
    return (0);
}

/************************************************************************/
/* SLICES */

/************************************************************************/

#define DISTANCE_2(x1, y1, x2, y2)	sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))

/*!
   \brief Draw slices

   \param gvl pointer to geovol struct

   \return 0
 */
int gvld_slices(geovol * gvl)
{
    float tx, ty, tz;
    int i;

    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    G_debug(5, "gvld_slices");

    /* shade */
    gsd_shademodel(gvl->slice_draw_mode & DM_GOURAUD);

    /* scaling */
    GS_get_scale(&tx, &ty, &tz, 1);

    /* get viewport */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);

    gsd_colormode(CM_COLOR);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(gvl->x_trans, gvl->y_trans, gvl->z_trans);

    for (i = 0; i < gvl->n_slices; i++) {
	gsd_blend(0);
	gsd_zwritemask(0xffffffff);

	if (gvl->slice[i]->transp == 0)
	    gvld_slice(gvl, i);
    }

    for (i = 0; i < gvl->n_slices; i++) {
	gsd_blend(1);
	gsd_zwritemask(0x0);

	if (gvl->slice[i]->transp > 0)
	    gvld_slice(gvl, i);
    }

    gsd_set_material(1, 1, 0., 0., 0x0);
    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (0);
}

/*!
   \brief Draw slice

   \param gvl pointer to geovol struct
   \param ndx

   \return 1
 */
int gvld_slice(geovol * gvl, int ndx)
{
    geovol_slice *slice;

    int color, offset, transp;
    float n[3], pt[4];
    float x, nextx, y, nexty, z, stepx, stepy, stepz;
    int cols, rows, c, r;
    float f_cols, f_rows, distxy, distz, modx, mody, modz, modxy;
    int ptX, ptY, ptZ;
    double resx, resy, resz;

    /* current slice */
    slice = gvl->slice[ndx];

    /* distance between slice def. pts */
    distxy = DISTANCE_2(slice->x2, slice->y2, slice->x1, slice->y1);
    distz = fabsf(slice->z2 - slice->z1);

    /* distance between slice def pts is zero - zero slice */
    if (distxy == 0. || distz == 0.) {
	return (1);
    }

    /* set slice mod, resolution and set correct coords */
    if (slice->dir == X) {
	modx = gvl->slice_y_mod;
	mody = gvl->slice_z_mod;
	modz = gvl->slice_x_mod;
	resx = gvl->yres;
	resy = gvl->zres;
	resz = gvl->xres;
	ptX = Y;
	ptY = Z;
	ptZ = X;
    }
    else if (slice->dir == Y) {
	modx = gvl->slice_x_mod;
	mody = gvl->slice_z_mod;
	modz = gvl->slice_y_mod;
	resx = gvl->xres;
	resy = gvl->zres;
	resz = gvl->yres;
	ptX = X;
	ptY = Z;
	ptZ = Y;
    }
    else {
	modx = gvl->slice_x_mod;
	mody = gvl->slice_y_mod;
	modz = gvl->slice_z_mod;
	resx = gvl->xres;
	resy = gvl->yres;
	resz = gvl->zres;
	ptX = X;
	ptY = Y;
	ptZ = Z;
    }

    /* x,y mod */
    modxy =
	DISTANCE_2((slice->x2 - slice->x1) / distxy * modx,
		   (slice->y2 - slice->y1) / distxy * mody, 0., 0.);

    /* cols/rows of slice */
    f_cols = distxy / modxy;
    cols = f_cols > (int)f_cols ? (int)f_cols + 1 : (int)f_cols;

    f_rows = distz / modz;
    rows = f_rows > (int)f_rows ? (int)f_rows + 1 : (int)f_rows;

    /* step in x,y for each row of slice */
    stepx = (slice->x2 - slice->x1) / f_cols;
    stepy = (slice->y2 - slice->y1) / f_cols;
    stepz = (slice->z2 - slice->z1) / f_rows;

    /* set x,y intially to first slice pt */
    x = (slice->x1);
    y = (slice->y1);

    /* set next draw pt */
    if (f_cols < 1.) {
	nextx = x + stepx * f_cols;
	nexty = y + stepy * f_cols;
    }
    else {
	nextx = x + stepx;
	nexty = y + stepy;
    }

    /* set transparency */
    if (slice->transp > 0) {
	transp = (255 - slice->transp) << 24;
    }
    else {
	transp = 0x0;
    }

    /* loop in slice cols */
    for (c = 0; c < cols; c++) {

	/* set z to slice z1 pt */
	z = slice->z1;

	/* begin draw one row - triangle strip */
	gsd_bgntmesh();

	/* loop in slice rows */
	for (r = 0; r < rows + 1; r++) {
	    /* offset to data - 1. column */
	    offset = (c + 1) * (rows + 1) * 3 + r * 3;

	    /* get color from slice data */
	    color = (slice->data[offset] & 0xff) |
		((slice->data[offset + 1] & 0xff) << 8) |
		((slice->data[offset + 2] & 0xff) << 16);

	    /* triagle vertices */
	    pt[ptX] = nextx * resx;
	    pt[ptY] = nexty * resy;
	    pt[ptZ] = z * resz;

	    pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	    gsd_litvert_func(n, (unsigned int)transp | color, pt);

	    /* offset to data - 2. column */
	    offset = c * (rows + 1) * 3 + r * 3;

	    /* get color from slice data */
	    color = (slice->data[offset] & 0xff) |
		((slice->data[offset + 1] & 0xff) << 8) |
		((slice->data[offset + 2] & 0xff) << 16);

	    /* set triangle vertices */
	    pt[ptX] = x * resx;
	    pt[ptY] = y * resy;
	    pt[ptZ] = z * resz;

	    pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	    gsd_litvert_func(n, (unsigned int)transp | color, pt);

	    if (r + 1 > f_rows) {
		z += stepz * (f_rows - (float)r);
	    }
	    else {
		z += stepz;
	    }
	}

	gsd_endtmesh();

	/* step */
	if (c + 2 > f_cols) {
	    x += stepx;
	    nextx += stepx * (f_cols - (float)(c + 1));
	    y += stepy;
	    nexty += stepy * (f_cols - (float)(c + 1));
	}
	else {
	    x += stepx;
	    nextx += stepx;
	    y += stepy;
	    nexty += stepy;
	}
    }

    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (1);
}

/*!
   \brief Draw wire slices 

   \param gvl pointer to geovol struct

   \return 0
 */
int gvld_wire_slices(geovol * gvl)
{
    float pt[3];
    int i;
    int ptX, ptY, ptZ;
    double resx, resy, resz;

    geovol_slice *slice;

    G_debug(5, "gvld_wire_slices");

    gsd_pushmatrix();

    /* shading */
    gsd_shademodel(DM_FLAT);
    /* set color mode */
    gsd_colormode(CM_COLOR);
    /* do scale and set volume position */
    gsd_do_scale(1);
    gsd_translate(gvl->x_trans, gvl->y_trans, gvl->z_trans);

    /* set color and line width */
    gsd_color_func(0x0);
    gsd_linewidth(1);

    /* loop in slices */
    for (i = 0; i < gvl->n_slices; i++) {
	slice = gvl->slice[i];

	/* intialize correct coords */
	if (slice->dir == X) {
	    resx = gvl->yres;
	    resy = gvl->zres;
	    resz = gvl->xres;
	    ptX = Y;
	    ptY = Z;
	    ptZ = X;
	}
	else if (slice->dir == Y) {
	    resx = gvl->xres;
	    resy = gvl->zres;
	    resz = gvl->yres;
	    ptX = X;
	    ptY = Z;
	    ptZ = Y;
	}
	else {
	    resx = gvl->xres;
	    resy = gvl->yres;
	    resz = gvl->zres;
	    ptX = X;
	    ptY = Y;
	    ptZ = Z;
	}

	gsd_bgnline();

	/* first slice edge */
	pt[ptX] = slice->x1 * resx;
	pt[ptY] = slice->y1 * resy;
	pt[ptZ] = slice->z1 * resz;;
	pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	gsd_vert_func(pt);

	pt[ptX] = slice->x1 * resx;
	pt[ptY] = slice->y1 * resy;
	pt[ptZ] = slice->z2 * resz;;
	pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	gsd_vert_func(pt);

	pt[ptX] = slice->x2 * resx;
	pt[ptY] = slice->y2 * resy;
	pt[ptZ] = slice->z2 * resz;;
	pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	gsd_vert_func(pt);

	pt[ptX] = slice->x2 * resx;
	pt[ptY] = slice->y2 * resy;
	pt[ptZ] = slice->z1 * resz;;
	pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	gsd_vert_func(pt);

	pt[ptX] = slice->x1 * resx;
	pt[ptY] = slice->y1 * resy;
	pt[ptZ] = slice->z1 * resz;;
	pt[Y] = (gvl->rows - 1) * gvl->yres - pt[Y];
	gsd_vert_func(pt);

	gsd_endline();
    }

    gsd_set_material(1, 1, 0., 0., 0x0);
    gsd_popmatrix();

    return (0);
}

/*!
   \brief Draw volume bounding box

   \param gvl pointer to geovol struct

   \return 0
 */
int gvld_wind3_box(geovol * gvl)
{
    float pt[3];

    G_debug(5, "gvld_wind3_box(): id=%d", gvl->gvol_id);

    gsd_pushmatrix();

    /* shading */
    gsd_shademodel(DM_FLAT);
    /* set color mode */
    gsd_colormode(CM_COLOR);
    /* do scale and set volume position */
    gsd_do_scale(1);
    gsd_translate(gvl->x_trans, gvl->y_trans, gvl->z_trans);

    /* set color and line width */
    gsd_color_func(0x0);
    gsd_linewidth(1);

    /* draw box */

    /* front edges */
    gsd_bgnline();
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = 0;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = 0;
    gsd_vert_func(pt);
    gsd_endline();

    /* back edges */
    gsd_bgnline();
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = 0;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    gsd_endline();

    /* others edges */
    gsd_bgnline();
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = 0;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    gsd_endline();

    gsd_bgnline();
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = 0;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = 0;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    gsd_endline();

    gsd_bgnline();
    pt[X] = 0;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = 0;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    gsd_endline();

    gsd_bgnline();
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = 0;
    gsd_vert_func(pt);
    pt[X] = (gvl->cols - 1) * gvl->xres;
    pt[Y] = (gvl->rows - 1) * gvl->yres;
    pt[Z] = (gvl->depths - 1) * gvl->zres;
    gsd_vert_func(pt);
    gsd_endline();

    gsd_popmatrix();

    return (0);
}
