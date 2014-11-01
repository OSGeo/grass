/*!
   \file lib/ogsf/gsd_wire.c

   \brief OGSF library - 

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (January 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "gsget.h"
#include "rowcol.h"


#define DO_ARROWS

/************************************************************************/
/* Notes on exageration:
   vertical exageration is of two forms: 
   1) global exageration (from geoview struct) 
   2) vertical exageration for each surface - not implemented
 */

/************************************************************************/
/* may need to add more parameters to tell it which window or off_screen
 * pixmap to draw into. nah - just have one current (OpenGL limitation)
 */

/*!
   \brief Draw surface wire

   \param surf surface (geosurf)

   \return
 */
int gsd_wire_surf(geosurf * surf)
{
    int desc, ret;

    G_debug(3, "gsd_wire_surf(): id=%d", surf->gsurf_id);

    desc = ATT_TOPO;

    switch (gs_get_att_src(surf, desc)) {
    case NOTSET_ATT:
	ret = (-1);

	break;

    case MAP_ATT:
	if (surf->draw_mode & DM_GRID_WIRE)
	    ret = (gsd_wire_surf_map(surf));	/* draw mesh */
	else
	    ret = (gsd_coarse_surf_map(surf));	/* draw coarse surf */

#ifdef DO_ARROWS
	/*
	   gsd_wire_arrows(surf);
	 */
#endif

	break;

    case CONST_ATT:
	ret = (gsd_wire_surf_const(surf, surf->att[desc].constant));
	break;

    case FUNC_ATT:
	ret = (gsd_wire_surf_func(surf, surf->att[desc].user_func));

	break;

    default:
	ret = (-1);

	break;
    }

    return (ret);
}

/*!
   \brief ADD

   \param surf surface (geosurf)

   \return
 */
int gsd_wire_surf_map(geosurf * surf)
{
    int check_mask, check_color;
    typbuff *buff, *cobuff;
    int xmod, ymod, row, col, cnt, xcnt, ycnt, x1off;
    long offset, y1off;
    float pt[4], xres, yres, ymax, zexag;
    int col_src, curcolor;
    gsurf_att *coloratt;

    G_debug(3, "gsd_wire_surf_map");

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);

    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

    /*
       checks ATT_TOPO & ATT_COLOR no_zero flags, make a mask from each,
       combine it/them with any current mask, put in typbuff:
       if(surf->att[ATT_TOPO].constant)
     */

    xmod = surf->x_modw;
    ymod = surf->y_modw;
    xres = xmod * surf->xres;
    yres = ymod * surf->yres;
    ymax = (surf->rows - 1) * surf->yres;
    xcnt = 1 + (surf->cols - 1) / xmod;
    ycnt = 1 + (surf->rows - 1) / ymod;

    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    zexag = surf->z_exag;

    gsd_colormode(CM_COLOR);

    /* will need to check for color source of FUNC_ATT & NOTSET_ATT, 
       or else use more general and inefficient gets */

    check_color = (surf->wire_color == WC_COLOR_ATT);

    if (check_color) {
	coloratt = &(surf->att[ATT_COLOR]);
	col_src = surf->att[ATT_COLOR].att_src;

	if (col_src != MAP_ATT) {
	    if (col_src == CONST_ATT) {
		gsd_color_func((int)surf->att[ATT_COLOR].constant);
	    }
	    else {
		gsd_color_func(surf->wire_color);
	    }

	    check_color = 0;
	}
    }
    else {
	gsd_color_func(surf->wire_color);
    }

    /* would also be good to check if colormap == surfmap, to increase speed */
    for (row = 0; row < ycnt; row++) {
	pt[Y] = ymax - row * yres;
	y1off = row * ymod * surf->cols;

	gsd_bgnline();
	cnt = 0;

	for (col = 0; col < xcnt; col++) {
	    pt[X] = col * xres;
	    x1off = col * xmod;
	    offset = x1off + y1off;

	    if (check_mask) {
		if (BM_get(surf->curmask, col * xmod, row * ymod)) {
		    gsd_endline();
		    gsd_bgnline();
		    cnt = 0;
		    continue;
		}
	    }

	    GET_MAPATT(buff, offset, pt[Z]);

	    if (check_color) {
		curcolor = gs_mapcolor(cobuff, coloratt, offset);
		gsd_color_func(curcolor);
		/* could use this & skip the GET if colordata == elevdata
		   gsd_color_func(gs_fastmapcolor(cobuff, coloratt, offset,
		   (int)pt[Z]));
		 */
	    }

	    pt[Z] = pt[Z] * zexag;

	    gsd_vert_func(pt);

	    if (cnt == 255) {
		gsd_endline();
		gsd_bgnline();
		cnt = 0;
		gsd_vert_func(pt);
	    }

	    cnt++;
	}

	gsd_endline();
    }

    for (col = 0; col < xcnt; col++) {
	pt[X] = col * xres;
	x1off = col * xmod;

	gsd_bgnline();
	cnt = 0;

	for (row = 0; row < ycnt; row++) {
	    pt[Y] = ymax - row * yres;
	    y1off = row * ymod * surf->cols;
	    offset = x1off + y1off;

	    if (check_mask) {
		if (BM_get(surf->curmask, col * xmod, row * ymod)) {
		    gsd_endline();
		    gsd_bgnline();
		    cnt = 0;
		    continue;
		}
	    }

	    GET_MAPATT(buff, offset, pt[Z]);

	    if (check_color) {
		curcolor = gs_mapcolor(cobuff, coloratt, offset);
		gsd_color_func(curcolor);
		/* could use this & skip the GET if colordata == elevdata
		   gsd_color_func(gs_fastmapcolor(coloratt, offset, (int)pt[Z]));
		 */
	    }

	    pt[Z] = pt[Z] * zexag;

	    gsd_vert_func(pt);

	    if (cnt == 255) {
		gsd_endline();
		gsd_bgnline();
		cnt = 0;
		gsd_vert_func(pt);
	    }

	    cnt++;
	}

	gsd_endline();
    }

    gsd_popmatrix();
    gsd_colormode(CM_DIFFUSE);

    return (1);
}

/*!
   \brief ADD

   \param surf surface (geosurf)
   \param k

   \return
 */
int gsd_wire_surf_const(geosurf * surf, float k)
{
    int do_diff, check_mask, check_color;
    int xmod, ymod, row, col, cnt, xcnt, ycnt, x1off;
    long offset, y1off;
    float pt[4], xres, yres, ymax, zexag;
    int col_src;
    gsurf_att *coloratt;
    typbuff *cobuff;

    G_debug(3, "gsd_wire_surf_const");

    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);

    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

    do_diff = (NULL != gsdiff_get_SDref());

    xmod = surf->x_modw;
    ymod = surf->y_modw;
    xres = xmod * surf->xres;
    yres = ymod * surf->yres;

    xcnt = 1 + (surf->cols - 1) / xmod;
    ycnt = 1 + (surf->rows - 1) / ymod;
    ymax = (surf->rows - 1) * surf->yres;

    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    zexag = surf->z_exag;

    gsd_colormode(CM_COLOR);

    /* will need to check for color source of FUNC_ATT & NOTSET_ATT, 
       or else use more general and inefficient gets */

    check_color = (surf->wire_color == WC_COLOR_ATT);

    if (check_color) {
	coloratt = &(surf->att[ATT_COLOR]);
	col_src = surf->att[ATT_COLOR].att_src;

	if (col_src != MAP_ATT) {
	    if (col_src == CONST_ATT) {
		gsd_color_func((int)surf->att[ATT_COLOR].constant);
	    }
	    else {
		gsd_color_func(surf->wire_color);
	    }

	    check_color = 0;
	}
    }
    else {
	gsd_color_func(surf->wire_color);
    }

    pt[Z] = k * zexag;

    for (row = 0; row < ycnt; row++) {
	pt[Y] = ymax - row * yres;
	y1off = row * ymod * surf->cols;

	gsd_bgnline();
	cnt = 0;

	for (col = 0; col < xcnt; col++) {
	    pt[X] = col * xres;
	    x1off = col * xmod;
	    offset = x1off + y1off;

	    if (check_mask) {
		if (BM_get(surf->curmask, col * xmod, row * ymod)) {
		    gsd_endline();
		    gsd_bgnline();
		    cnt = 0;
		    continue;
		}
	    }

	    if (check_color) {
		gsd_color_func(gs_mapcolor(cobuff, coloratt, offset));
	    }

	    if (do_diff) {
		pt[Z] = gsdiff_do_SD(k * zexag, offset);
	    }

	    gsd_vert_func(pt);

	    if (cnt == 255) {
		gsd_endline();
		gsd_bgnline();
		cnt = 0;
		gsd_vert_func(pt);
	    }

	    cnt++;
	}

	gsd_endline();
    }

    for (col = 0; col < xcnt; col++) {
	pt[X] = col * xres;
	x1off = col * xmod;

	gsd_bgnline();
	cnt = 0;

	for (row = 0; row < ycnt; row++) {
	    pt[Y] = ymax - row * yres;
	    y1off = row * ymod * surf->cols;
	    offset = x1off + y1off;

	    if (check_mask) {
		if (BM_get(surf->curmask, col * xmod, row * ymod)) {
		    gsd_endline();
		    gsd_bgnline();
		    cnt = 0;
		    continue;
		}
	    }

	    if (check_color) {
		gsd_color_func(gs_mapcolor(cobuff, coloratt, offset));
	    }

	    if (do_diff) {
		pt[Z] = gsdiff_do_SD(k * zexag, offset);
	    }

	    gsd_vert_func(pt);

	    if (cnt == 255) {
		gsd_endline();
		gsd_bgnline();
		cnt = 0;
		gsd_vert_func(pt);
	    }

	    cnt++;
	}

	gsd_endline();
    }

    gsd_popmatrix();
    gsd_colormode(CM_DIFFUSE);

    return (1);
}

/*!
   \brief ADD

   Not yet implemented.

   \param gs surface (geosurf)
   \param user_func user defined function

   \return 1
 */
int gsd_wire_surf_func(geosurf * gs, int (*user_func) ())
{
    return (1);
}

/*!
   \brief ADD

   Need to do Zexag scale of normal for arrow direction, drawing
   routine unexags z for arrow

   \param surf surface (geosurf)

   \return
 */
int gsd_wire_arrows(geosurf * surf)
{
    typbuff *buff, *cobuff;
    int check_mask, check_color;
    int xmod, ymod, row, col, xcnt, ycnt;
    long offset, y1off;
    float tx, ty, tz, sz;
    float n[3], pt[4], xres, yres, ymax, zexag;
    int col_src, curcolor;
    gsurf_att *coloratt;

    G_debug(3, "gsd_norm_arrows");

    /* avoid scaling by zero */
    GS_get_scale(&tx, &ty, &tz, 1);

    if (tz == 0.0) {
	return (0);
    }

    sz = GS_global_exag();

    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

    /*
       checks ATT_TOPO & ATT_COLOR no_zero flags, make a mask from each,
       combine it/them with any current mask, put in surf->curmask:
     */

    check_color = 1;
    curcolor = 0;
    coloratt = &(surf->att[ATT_COLOR]);
    col_src = surf->att[ATT_COLOR].att_src;

    if (col_src != MAP_ATT) {
	if (col_src == CONST_ATT) {
	    curcolor = (int)surf->att[ATT_COLOR].constant;
	}
	else {
	    curcolor = surf->wire_color;
	}

	check_color = 0;
    }

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);

    xmod = surf->x_modw;
    ymod = surf->y_modw;
    xres = xmod * surf->xres;
    yres = ymod * surf->yres;
    ymax = (surf->rows - 1) * surf->yres;
    xcnt = 1 + (surf->cols - 1) / xmod;
    ycnt = 1 + (surf->rows - 1) / ymod;

    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    zexag = surf->z_exag;
    /* CURRENTLY ALWAYS 1.0 */

    gsd_colormode(CM_COLOR);

    for (row = 0; row < ycnt; row++) {
	pt[Y] = ymax - row * yres;
	y1off = row * ymod * surf->cols;

	for (col = 0; col < xcnt; col++) {
	    pt[X] = col * xres;
	    offset = col * xmod + y1off;

	    if (check_mask) {
		if (BM_get(surf->curmask, col * xmod, row * ymod)) {
		    continue;
		}
	    }

	    FNORM(surf->norms[offset], n);
	    GET_MAPATT(buff, offset, pt[Z]);
	    pt[Z] *= zexag;

	    if (check_color) {
		curcolor = gs_mapcolor(cobuff, coloratt, offset);
	    }

	    gsd_arrow(pt, curcolor, xres * 2, n, sz, surf);
	}			/* ea col */
    }				/* ea row */

    gsd_popmatrix();
    gsd_colormode(CM_DIFFUSE);

    return (1);
}

/*!
   \brief Draw coarse surface

   New (TEST) wire routine that draws low res surface 
   Based on new Trinagle Fan routine
   Resolution is a function of current surface resolution 
   times wire resolution

   \todo normals have to be recalculated before proper low
   res surface can be drawn

   In window optimization has been removed 

   \param surf surface (geosurf)

   \return
 */
int gsd_coarse_surf_map(geosurf * surf)
{
    int check_mask, check_color, check_transp;
    int check_material, check_emis, check_shin;
    typbuff *buff, *cobuff, *trbuff, *embuff, *shbuff;
    int xmod, ymod;
    int row, col, xcnt, ycnt;
    long y1off, y2off, y3off;
    long offset2[10];
    float pt2[10][2];
    int ii;
    float x1, x2, x3, y1, y2, y3, tx, ty, tz, ttr;
    float n[3], pt[4], xres, yres, ymax, zexag;
    int em_src, sh_src, trans_src, col_src, curcolor;
    gsurf_att *ematt, *shatt, *tratt, *coloratt;


    int datarow1, datacol1, datarow2, datacol2, datarow3, datacol3;

    float kem, ksh, pkem, pksh;
    unsigned int ktrans;

    int step_val = 2 * surf->x_modw;	/* should always be factor of 2 for fan */
    int start_val = surf->x_modw;

    /* ensure normals are correct */
    gs_calc_normals(surf);

    /* avoid scaling by zero */
    GS_get_scale(&tx, &ty, &tz, 1);

    if (tz == 0.0) {
	return (gsd_surf_const(surf, 0.0));
    }
    /* else if (surf->z_exag  == 0.0)
       {
       return(gsd_surf_const(surf, surf->z_min));
       }
       NOT YET IMPLEMENTED */

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);

    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

    /*
       checks ATT_TOPO & ATT_COLOR no_zero flags, make a mask from each,
       combine it/them with any current mask, put in surf->curmask:
     */
    xmod = surf->x_mod;
    ymod = surf->y_mod;
    xres = xmod * surf->xres;
    yres = ymod * surf->yres;
    ymax = (surf->rows - 1) * surf->yres;

    xcnt = VCOLS(surf);
    ycnt = VROWS(surf);

    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);
    zexag = surf->z_exag;

    gsd_colormode(CM_DIFFUSE);


    /* CURRENTLY ALWAYS 1.0 */
#ifdef CALC_AREA
    sz = GS_global_exag();
#endif

    /* TODO: get rid of (define) these magic numbers scaling the attribute vals */
    check_transp = 0;
    tratt = &(surf->att[ATT_TRANSP]);
    ktrans = (255 << 24);
    trans_src = surf->att[ATT_TRANSP].att_src;

    if (CONST_ATT == trans_src && surf->att[ATT_TRANSP].constant != 0.0) {
	ktrans = (255 - (int)surf->att[ATT_TRANSP].constant) << 24;
	gsd_blend(1);
	gsd_zwritemask(0x0);
    }
    else if (MAP_ATT == trans_src) {
	trbuff = gs_get_att_typbuff(surf, ATT_TRANSP, 0);
	check_transp = trbuff ? 1 : 0;
	gsd_blend(1);
	gsd_zwritemask(0x0);
    }

    check_emis = 0;
    ematt = &(surf->att[ATT_EMIT]);
    kem = 0.0;
    pkem = 1.0;
    em_src = surf->att[ATT_EMIT].att_src;

    if (CONST_ATT == em_src) {
	kem = surf->att[ATT_EMIT].constant / 255.;
    }
    else if (MAP_ATT == em_src) {
	embuff = gs_get_att_typbuff(surf, ATT_EMIT, 0);
	check_emis = embuff ? 1 : 0;
    }

    check_shin = 0;
    shatt = &(surf->att[ATT_SHINE]);
    ksh = 0.0;
    pksh = 1.0;
    sh_src = surf->att[ATT_SHINE].att_src;

    if (CONST_ATT == sh_src) {
	ksh = surf->att[ATT_SHINE].constant / 255.;
	gsd_set_material(1, 0, ksh, kem, 0x0);
    }
    else if (MAP_ATT == sh_src) {
	shbuff = gs_get_att_typbuff(surf, ATT_SHINE, 0);
	check_shin = shbuff ? 1 : 0;
    }

    /* will need to check for color source of FUNC_ATT & NOTSET_ATT, 
       or else use more general and inefficient gets */
    check_color = 1;
    curcolor = 0;
    coloratt = &(surf->att[ATT_COLOR]);
    col_src = surf->att[ATT_COLOR].att_src;

    if (col_src != MAP_ATT) {
	if (col_src == CONST_ATT) {
	    curcolor = (int)surf->att[ATT_COLOR].constant;
	}
	else {
	    curcolor = surf->wire_color;
	}

	check_color = 0;
    }

    check_material = (check_shin || check_emis || (kem && check_color));

    /* would also be good to check if colormap == surfmap, to increase speed */
    /* will also need to set check_transp, check_shine, etc & fix material */
    for (row = start_val; row <= ycnt - start_val; row += step_val) {

	datarow1 = row * ymod;
	datarow2 = (row - (step_val / 2)) * ymod;
	datarow3 = (row + (step_val / 2)) * ymod;


	y1 = ymax - row * yres;
	y2 = ymax - (row - (step_val / 2)) * yres;
	y3 = ymax - (row + (step_val / 2)) * yres;

	y1off = row * ymod * surf->cols;
	y2off = (row - (step_val / 2)) * ymod * surf->cols;
	y3off = (row + (step_val / 2)) * ymod * surf->cols;

	for (col = start_val; col <= xcnt - start_val; col += step_val) {

	    datacol1 = col * xmod;
	    datacol2 = (col - (step_val / 2)) * xmod;
	    datacol3 = (col + (step_val / 2)) * xmod;

	    x1 = col * xres;
	    x2 = (col - (step_val / 2)) * xres;
	    x3 = (col + (step_val / 2)) * xres;


	    /* Do not need BM_get because GET_MAPATT calls
	     * same and returns zero if masked
	     */
	    offset2[0] = y1off + datacol1;	/* fan center */
	    pt2[0][X] = x1;
	    pt2[0][Y] = y1;	/* fan center */
	    pt[X] = pt2[0][X];
	    pt[Y] = pt2[0][Y];
	    if (!GET_MAPATT(buff, offset2[0], pt[Z]))
		continue;	/* masked */
	    pt[Z] *= zexag;

	    offset2[1] = y2off + datacol2;
	    offset2[2] = y2off + datacol1;
	    offset2[3] = y2off + datacol3;
	    offset2[4] = y1off + datacol3;
	    offset2[5] = y3off + datacol3;
	    offset2[6] = y3off + datacol1;
	    offset2[7] = y3off + datacol2;
	    offset2[8] = y1off + datacol2;
	    offset2[9] = y2off + datacol2;	/* repeat 1st corner to close */

	    pt2[1][X] = x2;
	    pt2[1][Y] = y2;
	    pt2[2][X] = x1;
	    pt2[2][Y] = y2;
	    pt2[3][X] = x3;
	    pt2[3][Y] = y2;
	    pt2[4][X] = x3;
	    pt2[4][Y] = y1;
	    pt2[5][X] = x3;
	    pt2[5][Y] = y3;
	    pt2[6][X] = x1;
	    pt2[6][Y] = y3;
	    pt2[7][X] = x2;
	    pt2[7][Y] = y3;
	    pt2[8][X] = x2;
	    pt2[8][Y] = y1;
	    pt2[9][X] = x2;
	    pt2[9][Y] = y2;	/* repeat 1st corner to close */

	    /* Run through triangle fan */
	    gsd_bgntfan();
	    for (ii = 0; ii < 10; ii++) {


		if (ii > 0) {
		    pt[X] = pt2[ii][X];
		    pt[Y] = pt2[ii][Y];
		    if (!GET_MAPATT(buff, offset2[ii], pt[Z]))
			continue;
		    pt[Z] *= zexag;
		}

		FNORM(surf->norms[offset2[ii]], n);

		if (check_color)
		    curcolor = gs_mapcolor(cobuff, coloratt, offset2[ii]);

		if (check_transp) {
		    GET_MAPATT(trbuff, offset2[ii], ttr);
		    ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
		    ktrans = (char)(255 - ktrans) << 24;
		}


		if (check_material) {
		    if (check_emis) {
			GET_MAPATT(embuff, offset2[ii], kem);
			kem = SCALE_ATT(ematt, kem, 0., 1.);
		    }

		    if (check_shin) {
			GET_MAPATT(shbuff, offset2[ii], ksh);
			ksh = SCALE_ATT(shatt, ksh, 0., 1.);
		    }

		    if (pksh != ksh || pkem != kem || (kem && check_color)) {
			pksh = ksh;
			pkem = kem;
			gsd_set_material(check_shin, check_emis,
					 ksh, kem, curcolor);
		    }
		}


		gsd_litvert_func(n, ktrans | curcolor, pt);


	    }			/* close ii loop */
	    gsd_endtfan();

	}			/* end col */

    }				/* end row */

    gsd_popmatrix();
    /*
       gsd_colormode(CM_DIFFUSE);
     */
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (0);
}
