/*!
   \file gsd_fonts.c

   \brief OGSF library - manipulating surfaces/fridge (lower level function)

   GRASS OpenGL gsurf OGSF Library 

   \todo This file needs to be re-written in OpenGL

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/gstypes.h>

#include "gsget.h"
#include "rowcol.h"

#define FRINGE_FORE 0x000000
#define FRINGE_WIDTH 2

/*!
   \brief Normals
 */
float Nnorth[] = { 0.0, 0.8, 0.6 };
float Nsouth[] = { 0.0, -0.8, 0.6 };
float Neast[] = { 0.8, 0.0, 0.6 };
float Nwest[] = { -0.8, 0.0, 0.6 };
float Ntop[] = { 0.0, 0.0, 1.0 };
float Nbottom[] = { 0.0, 0.0, -1.0 };

/*!
   \brief Display fridge

   \todo add elevation for bottom
   add color option
   add ruler grid lines

   \param surf surface (geosurf)
   \param clr
   \param elev
   \param where
 */
void gsd_display_fringe(geosurf * surf, unsigned long clr, float elev,
			int where[4])
{
    float bot, xres, yres;	/* world size of view cell */
    int ycnt, xcnt;		/* number of view cells across */
    float xmax, ymax;

    xres = surf->x_mod * surf->xres;
    yres = surf->y_mod * surf->yres;

    xcnt = VCOLS(surf);
    ycnt = VROWS(surf);

    xmax = surf->xmax;
    ymax = surf->ymax;

    /* 
       bot = surf->zmin - ((surf->zrange/4.) * surf->z_exag);
     */
    bot = elev - ((surf->zrange / 4.) * surf->z_exag);


    gsd_linewidth(FRINGE_WIDTH);
    gsd_colormode(CM_COLOR);

    /* North fringe */
    if (where[0] || where[1]) {
	glNormal3fv(Nnorth);
	gsd_color_func(clr);
	gsd_zwritemask(0x0);
	gsd_fringe_horiz_poly(bot, surf, 0, 0);
	gsd_color_func(FRINGE_FORE);	/* WHITE */
	gsd_fringe_horiz_line(bot, surf, 0, 0);
	gsd_zwritemask(0xffffffff);
	/*   wmpack (0); ??? glColorMask */
	gsd_color_func(clr);
	gsd_fringe_horiz_poly(bot, surf, 0, 0);
    }

    /* South fringe */
    if (where[2] || where[3]) {
	glNormal3fv(Nsouth);
	gsd_color_func(clr);
	gsd_zwritemask(0x0);
	gsd_fringe_horiz_poly(bot, surf, ycnt - 2, 1);
	gsd_color_func(FRINGE_FORE);	/* WHITE */
	gsd_fringe_horiz_line(bot, surf, ycnt - 2, 1);
	gsd_zwritemask(0xffffffff);
	/*   wmpack (0); ??? glColorMask */
	gsd_color_func(clr);
	gsd_fringe_horiz_poly(bot, surf, ycnt - 2, 1);
    }

    /* West fringe */
    if (where[0] || where[2]) {
	glNormal3fv(Nwest);
	gsd_color_func(clr);
	gsd_zwritemask(0x0);
	gsd_fringe_vert_poly(bot, surf, 0, 0);
	gsd_color_func(FRINGE_FORE);
	gsd_fringe_vert_line(bot, surf, 0, 0);
	gsd_zwritemask(0xffffffff);
	gsd_color_func(clr);
	gsd_fringe_vert_poly(bot, surf, 0, 0);
    }

    /* East fringe */
    if (where[1] || where[3]) {
	glNormal3fv(Neast);
	gsd_color_func(clr);
	gsd_zwritemask(0x0);
	gsd_fringe_vert_poly(bot, surf, xcnt - 2, 1);
	gsd_color_func(FRINGE_FORE);
	gsd_fringe_vert_line(bot, surf, xcnt - 2, 1);
	gsd_zwritemask(0xffffffff);
	gsd_color_func(clr);
	gsd_fringe_vert_poly(bot, surf, xcnt - 2, 1);
    }

    return;
}

/*!
   \brief ADD

   \param bot
   \param surf surface (geosurf)
   \param row
   \param side
 */
void gsd_fringe_horiz_poly(float bot, geosurf * surf, int row, int side)
{
    int col;
    int cnt;
    float pt[4];
    typbuff *buff;
    long offset;
    int xcnt;

    GS_set_draw(GSD_FRONT);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    xcnt = VCOLS(surf);

    gsd_bgnpolygon();

    col = 0;
    /* floor left */
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) -
	((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
    GET_MAPATT(buff, offset, pt[Z]);
    pt[Z] = pt[Z] * surf->z_exag;
    gsd_vert_func(pt);

    cnt = 1;
    for (col = 0; col < xcnt - 1; col++) {
	/* bottom right */
	pt[X] = col * (surf->x_mod * surf->xres);
	pt[Y] =
	    ((surf->rows - 1) * surf->yres) -
	    ((row + side) * (surf->y_mod * surf->yres));
	offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
	GET_MAPATT(buff, offset, pt[Z]);
	pt[Z] = pt[Z] * surf->z_exag;
	gsd_vert_func(pt);
	cnt++;
    }

    col--;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) -
	((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    gsd_endpolygon();

    GS_done_draw();
    gsd_popmatrix();
    gsd_flush();

    return;
}

/*!
   \brief ADD

   \param bot
   \param surf surface (geosurf)
   \param row
   \param side
 */
void gsd_fringe_horiz_line(float bot, geosurf * surf, int row, int side)
{
    int col;
    int cnt;
    float pt[4];
    typbuff *buff;
    long offset;
    int xcnt;

    GS_set_draw(GSD_FRONT);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);


    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    xcnt = VCOLS(surf);

    gsd_bgnline();

    col = 0;
    /* floor left */
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) -
	((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
    GET_MAPATT(buff, offset, pt[Z]);
    pt[Z] = pt[Z] * surf->z_exag;
    gsd_vert_func(pt);

    cnt = 1;
    for (col = 0; col < xcnt - 1; col++) {
	/* bottom right */
	pt[X] = col * (surf->x_mod * surf->xres);
	pt[Y] =
	    ((surf->rows - 1) * surf->yres) -
	    ((row + side) * (surf->y_mod * surf->yres));
	offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
	GET_MAPATT(buff, offset, pt[Z]);
	pt[Z] = pt[Z] * surf->z_exag;
	gsd_vert_func(pt);
	cnt++;
    }

    col--;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) -
	((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    col = 0;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) -
	((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    gsd_endline();

    GS_done_draw();
    gsd_popmatrix();
    gsd_flush();

    return;
}

/*!
   \brief ADD

   \param bot
   \param surf surface (geosurf)
   \param col
   \param side [unused]
 */
void gsd_fringe_vert_poly(float bot, geosurf * surf, int col, int side)
{

    int row;
    int cnt;
    float pt[4];
    typbuff *buff;
    long offset;
    int ycnt;

    GS_set_draw(GSD_FRONT);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    gsd_bgnpolygon();

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    ycnt = VROWS(surf);

    row = 0;
    /* floor left */
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) - (row * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
    GET_MAPATT(buff, offset, pt[Z]);
    pt[Z] = pt[Z] * surf->z_exag;
    gsd_vert_func(pt);

    cnt = 1;
    for (row = 0; row < ycnt - 1; row++) {
	/* bottom right */
	pt[X] = col * (surf->x_mod * surf->xres);
	pt[Y] =
	    ((surf->rows - 1) * surf->yres) -
	    (row * (surf->y_mod * surf->yres));
	offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
	GET_MAPATT(buff, offset, pt[Z]);
	pt[Z] = pt[Z] * surf->z_exag;
	gsd_vert_func(pt);
	cnt++;
    }

    row--;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) - (row * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    gsd_endpolygon();

    GS_done_draw();
    gsd_popmatrix();
    gsd_flush();

    return;
}

/*!                                             
   \brief ADD

   \param bot
   \param surf surface (geosurf)
   \param col
   \param side [unused]
 */
void gsd_fringe_vert_line(float bot, geosurf * surf, int col, int side)
{
    int row;
    int cnt;
    float pt[4];
    typbuff *buff;
    long offset;
    int ycnt;

    GS_set_draw(GSD_FRONT);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);


    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    ycnt = VROWS(surf);
    gsd_bgnline();

    row = 0;
    /* floor left */
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) - (row * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
    GET_MAPATT(buff, offset, pt[Z]);
    pt[Z] = pt[Z] * surf->z_exag;
    gsd_vert_func(pt);

    cnt = 1;
    for (row = 0; row < ycnt - 1; row++) {
	/* bottom right */
	pt[X] = col * (surf->x_mod * surf->xres);
	pt[Y] =
	    ((surf->rows - 1) * surf->yres) -
	    (row * (surf->y_mod * surf->yres));
	offset = (row * surf->y_mod * surf->cols) + (col * surf->x_mod);
	GET_MAPATT(buff, offset, pt[Z]);
	pt[Z] = pt[Z] * surf->z_exag;
	gsd_vert_func(pt);
	cnt++;
    }

    row--;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) - (row * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    row = 0;
    pt[X] = col * (surf->x_mod * surf->xres);
    pt[Y] =
	((surf->rows - 1) * surf->yres) - (row * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    gsd_endline();

    GS_done_draw();
    gsd_popmatrix();
    gsd_flush();

    return;
}

/*!
   \brief ADD

   \param bot
   \param surf surface (geosurf)
   \param row
   \param side
 */
void gsd_fringe_horiz_line2(float bot, geosurf * surf, int row, int side)
{
    int col;
    int cnt;
    float pt[4];
    typbuff *buff;
    long offset;
    int xcnt;

    GS_set_draw(GSD_FRONT);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);
    xcnt = VCOLS(surf);
    gsd_bgnline();

    col = 0;
    /* floor left */
    pt[X] = surf->xmin + (col * (surf->x_mod * surf->xres));
    pt[Y] = surf->ymax - ((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    offset = 0;
    GET_MAPATT(buff, offset, pt[Z]);
    pt[Z] = pt[Z] * surf->z_exag;
    gsd_vert_func(pt);

    cnt = 1;
    for (col = 0; col < xcnt - 1; col++) {
	/* bottom right */
	pt[X] = surf->xmin + (col * (surf->x_mod * surf->xres));
	pt[Y] = surf->ymax - ((row + side) * (surf->y_mod * surf->yres));
	offset = col * surf->x_mod;
	GET_MAPATT(buff, offset, pt[Z]);
	pt[Z] = pt[Z] * surf->z_exag;
	gsd_vert_func(pt);
	cnt++;
    }

    col--;
    pt[X] = surf->xmin + (col * (surf->x_mod * surf->xres));
    pt[Y] = surf->ymax - ((row + side) * (surf->y_mod * surf->yres));
    pt[Z] = bot;
    gsd_vert_func(pt);

    gsd_endline();

    GS_done_draw();
    gsd_popmatrix();
    gsd_flush();

    return;
}
