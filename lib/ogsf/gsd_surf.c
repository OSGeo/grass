/*!
   \file lib/ogsf/gsd_surf.c

   \brief OGSF library - loading and manipulating surfaces

   GRASS OpenGL gsurf OGSF Library

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the
   GNU General Public License (>=v2).
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (October 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

#include "gsget.h"
#include "rowcol.h"

/*
   #define CALC_AREA
 */

/*
   #define DO_ARROW_SOLID
   #define DEBUG_ARROW ((row && !(row%surf->y_modw))&&(col &&
   !(col%surf->x_modw)))
 */

/*
   #define DO_ARROW
 */

#define DEBUG_ARROW (0)

/*!
   \brief MACROS for use in gsd_ortho_wall ONLY !!!
 */
#define SET_SCOLOR(sf)                                             \
    if (check_color[sf]) {                                         \
        tx = points[sf][i][X] - gsurfs[sf]->x_trans;               \
        ty = points[sf][i][Y] - gsurfs[sf]->y_trans;               \
        offset = XY2OFF(gsurfs[sf], tx, ty);                       \
        colors[sf] = gs_mapcolor(cobuf[sf], coloratt[sf], offset); \
    }

static int transpoint_is_masked(geosurf *, Point3);
static int get_point_below(Point3 **, geosurf **, int, int, int, int *);

static int FCmode;

/************************************************************************/
/* Notes on exaggeration:
   vertical exaggeration is of two forms:
   1) global exaggeration (from geoview struct)
   2) vertical exaggeration for each surface (UN-IMPLEMENTED)
 */

/************************************************************************/
/* may need to add more parameters to tell it which window or off_screen
 * pixmap to draw into.
 */

/*!
   \brief ADD

   \param surf surface (geosurf)

   \return
   \return -1 on error
 */
int gsd_surf(geosurf *surf)
{
    int desc, ret;

    G_debug(5, "gsd_surf(): id=%d", surf->gsurf_id);

    desc = ATT_TOPO;

    /* won't recalculate if update not needed, but may want to check
       to see if lights are on */
    gs_calc_normals(surf);

    switch (gs_get_att_src(surf, desc)) {
    case NOTSET_ATT:
        ret = (-1);

        break;

    case MAP_ATT:
        ret = (gsd_surf_map(surf)); /* changed to use test draw routine */

#ifdef DO_ARROW
        gsd_norm_arrows(surf);

        /* Not ready yet - need to recalc normals for proper res
           gsd_wire_arrows(surf);
         */
#endif

        break;

    case CONST_ATT:
        ret = (gsd_surf_const(surf, surf->att[desc].constant));

        break;

    case FUNC_ATT:
        ret = (gsd_surf_func(surf, surf->att[desc].user_func));

        break;

    default:
        ret = (-1);

        break;
    }

    return (ret);
}

/*!
   \brief ADD

   Using tmesh - not confident with qstrips portability

   \param surf surface (geosurf)

   \return
 */
int gsd_surf_map_old(geosurf *surf)
{
    int check_mask, check_color, check_transp;
    int check_material, check_emis, check_shin;
    typbuff *buff, *cobuff, *trbuff, *embuff, *shbuff;
    int xmod, ymod, row, col, cnt, xcnt, ycnt;
    long offset, y1off, y2off;
    float x1, x2, y1, y2, tx, ty, tz, ttr;
    float n[3], pt[4], xres, yres, ymax, zexag;
    int em_src, sh_src, trans_src, col_src, curcolor;
    gsurf_att *ematt, *shatt, *tratt, *coloratt;

    /* Viewport variables for accelerated drawing */
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

#ifdef CALC_AREA
    float sz, mag, tedge1[3], tedge2[3], crossp[3], triv[3][3];
    double asurf = 0.0, axsurf = 0.0;
#endif

    int zeros, dr1, dr2, dr3, dr4;
    int datarow1, datacol1, datarow2, datacol2;

    float kem, ksh, pkem, pksh;
    unsigned int ktrans;

    G_debug(3, "gsd_surf_map_old");

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

    /* Get viewport */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);
    /* adjust window */
    window[0] += (int)(yres * 2);
    window[1] -= (int)(yres * 2);
    window[2] -= (int)(xres * 2);
    window[3] += (int)(xres * 2);

    gsd_colormode(CM_DIFFUSE);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);
    zexag = surf->z_exag;

    /* CURRENTLY ALWAYS 1.0 */
#ifdef CALC_AREA
    sz = GS_global_exag();
#endif

    /* TESTING */
    /*
       fprintf(stderr, "This machine has %d alpha bits\n",
       getgdesc(GD_BITS_NORM_DBL_ALPHA)); fprintf(stderr, "GD_BLEND = %d \n",
       getgdesc(GD_BLEND)); fprintf(stderr, "GD_CLIPPLANES = %d \n",
       getgdesc(GD_CLIPPLANES));
     */

    /* TODO: get rid of (define) these magic numbers scaling the attribute vals
     */
    check_transp = 0;
    tratt = &(surf->att[ATT_TRANSP]);
    ktrans = (255U << 24);
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
    cnt = 0;

    for (row = 0; row < ycnt; row++) {
        if (GS_check_cancel()) {
            gsd_popmatrix();
            gsd_blend(0);
            gsd_zwritemask(0xffffffff);

            return (-1);
        }

        datarow1 = row * ymod;
        datarow2 = (row + 1) * ymod;

        y1 = ymax - row * yres;
        y2 = ymax - (row + 1) * yres;
        y1off = row * ymod * surf->cols;
        y2off = (row + 1) * ymod * surf->cols;

        gsd_bgntmesh();

        zeros = 0;
        dr1 = dr2 = dr3 = dr4 = 1;

        if (check_mask) {
            if (BM_get(surf->curmask, 0, datarow1)) {
                /*TL*/ ++zeros;
                dr1 = 0;
            }

            if (BM_get(surf->curmask, 0, datarow2)) {
                /*BL*/ ++zeros;
                dr2 = 0;
            }
        }

        if (dr1 && dr2) {
            offset = y1off; /* TL */
            FNORM(surf->norms[offset], n);
            pt[X] = 0;
            pt[Y] = y1;
            GET_MAPATT(buff, offset, pt[Z]);
            pt[Z] *= zexag;

            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            if (check_transp) {
                GET_MAPATT(trbuff, offset, ttr);
                ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                ktrans = (char)(255U - ktrans) << 24;
            }

            gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
            GS_v3eq(triv[cnt % 3], pt);
#endif

            cnt++;

            offset = y2off; /* BL */
            FNORM(surf->norms[offset], n);
            pt[X] = 0;
            pt[Y] = y2;
            GET_MAPATT(buff, offset, pt[Z]);
            pt[Z] *= zexag;

            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            if (check_transp) {
                GET_MAPATT(trbuff, offset, ttr);
                ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                ktrans = (char)(255U - ktrans) << 24;
            }

            if (check_material) {
                if (check_emis) {
                    GET_MAPATT(embuff, offset, kem);
                    kem = SCALE_ATT(ematt, kem, 0., 1.);
                }

                if (check_shin) {
                    GET_MAPATT(shbuff, offset, ksh);
                    ksh = SCALE_ATT(shatt, ksh, 0., 1.);
                }

                if (pksh != ksh || pkem != kem || (kem && check_color)) {
                    /* expensive */
                    pksh = ksh;
                    pkem = kem;
                    gsd_set_material(check_shin, check_emis, ksh, kem,
                                     curcolor);
                }
            }

            gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
            GS_v3eq(triv[cnt % 3], pt);
#endif

            cnt++;
        }

        for (col = 0; col < xcnt; col++) {
            datacol1 = col * xmod;
            datacol2 = (col + 1) * xmod;

            x1 = col * xres;
            x2 = (col + 1) * xres;

            zeros = 0;
            dr1 = dr2 = dr3 = dr4 = 1;

            if (check_mask) {
                if (BM_get(surf->curmask, datacol1, datarow1)) {
                    /*TL*/ ++zeros;
                    dr1 = 0;
                }

                if (BM_get(surf->curmask, datacol1, datarow2)) {
                    /*BL*/ ++zeros;
                    dr2 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow2)) {
                    /*BR*/ ++zeros;
                    dr3 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow1)) {
                    /*TR*/ ++zeros;
                    dr4 = 0;
                }

                if ((zeros > 1) && cnt) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }
            }

            if (cnt > 252) {
                /* not needed! - no limit for tmesh */
                cnt = 0;
                gsd_endtmesh();
                gsd_bgntmesh();

                if (dr1) {
                    offset = y1off + datacol1; /* TL */
                    FNORM(surf->norms[offset], n);
                    pt[X] = x1;
                    pt[Y] = y1;
                    GET_MAPATT(buff, offset, pt[Z]);
                    pt[Z] *= zexag;

                    if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                       projMatrix)) {
                        gsd_endtmesh();
                        cnt = 0;
                        gsd_bgntmesh();
                        continue;
                    }

                    if (check_color) {
                        curcolor = gs_mapcolor(cobuff, coloratt, offset);
                    }

                    if (check_transp) {
                        GET_MAPATT(trbuff, offset, ttr);
                        ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                        ktrans = (char)(255U - ktrans) << 24;
                    }

                    if (check_material) {
                        if (check_emis) {
                            GET_MAPATT(embuff, offset, kem);
                            kem = SCALE_ATT(ematt, kem, 0., 1.);
                        }

                        if (check_shin) {
                            GET_MAPATT(shbuff, offset, ksh);
                            ksh = SCALE_ATT(shatt, ksh, 0., 1.);
                        }

                        if (pksh != ksh || pkem != kem ||
                            (kem && check_color)) {
                            pksh = ksh;
                            pkem = kem;
                            gsd_set_material(check_shin, check_emis, ksh, kem,
                                             curcolor);
                        }
                    }

                    gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
                    GS_v3eq(triv[cnt % 3], pt);
#endif

                    cnt++;
                }

                if (dr2) {
                    offset = y2off + datacol1; /* BL */
                    FNORM(surf->norms[offset], n);
                    pt[X] = x1;
                    pt[Y] = y2;
                    GET_MAPATT(buff, offset, pt[Z]);
                    pt[Z] *= zexag;

                    if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                       projMatrix)) {
                        gsd_endtmesh();
                        cnt = 0;
                        gsd_bgntmesh();
                        continue;
                    }

                    if (check_color) {
                        curcolor = gs_mapcolor(cobuff, coloratt, offset);
                    }

                    if (check_transp) {
                        GET_MAPATT(trbuff, offset, ttr);
                        ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                        ktrans = (char)(255U - ktrans) << 24;
                    }

                    if (check_material) {
                        if (check_emis) {
                            GET_MAPATT(embuff, offset, kem);
                            kem = SCALE_ATT(ematt, kem, 0., 1.);
                        }

                        if (check_shin) {
                            GET_MAPATT(shbuff, offset, ksh);
                            ksh = SCALE_ATT(shatt, ksh, 0., 1.);
                        }

                        if (pksh != ksh || pkem != kem ||
                            (kem && check_color)) {
                            pksh = ksh;
                            pkem = kem;
                            gsd_set_material(check_shin, check_emis, ksh, kem,
                                             curcolor);
                        }
                    }

                    gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
                    GS_v3eq(triv[cnt % 3], pt);
#endif

                    cnt++;
                }
            }

            if (dr4) {
                offset = y1off + datacol2; /* TR */
                FNORM(surf->norms[offset], n);
                pt[X] = x2;
                pt[Y] = y1;
                GET_MAPATT(buff, offset, pt[Z]);
                pt[Z] *= zexag;

                if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                   projMatrix)) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }

                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                if (check_transp) {
                    GET_MAPATT(trbuff, offset, ttr);
                    ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                    ktrans = (char)(255U - ktrans) << 24;
                }

                if (check_material) {
                    if (check_emis) {
                        GET_MAPATT(embuff, offset, kem);
                        kem = SCALE_ATT(ematt, kem, 0., 1.);
                    }

                    if (check_shin) {
                        GET_MAPATT(shbuff, offset, ksh);
                        ksh = SCALE_ATT(shatt, ksh, 0., 1.);
                    }

                    if (pksh != ksh || pkem != kem || (kem && check_color)) {
                        pksh = ksh;
                        pkem = kem;
                        gsd_set_material(check_shin, check_emis, ksh, kem,
                                         curcolor);
                    }
                }

                gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
                GS_v3eq(triv[cnt % 3], pt);

                if (cnt > 1) {
                    GS_v3eq(tedge1, triv[1]);
                    GS_v3eq(tedge2, triv[2]);
                    GS_v3sub(tedge1, triv[0]);
                    GS_v3sub(tedge2, triv[1]);
                    GS_v3cross(tedge1, tedge2, crossp);
                    GS_v3mag(crossp, &mag);
                    asurf += .5 * mag;
                    tedge1[Z] *= sz;
                    tedge2[Z] *= sz;
                    GS_v3cross(tedge1, tedge2, crossp);
                    GS_v3mag(crossp, &mag);
                    axsurf += .5 * mag;
                }
#endif

                cnt++;
            }

            if (dr3) {
                offset = y2off + datacol2; /* BR */
                FNORM(surf->norms[offset], n);
                pt[X] = x2;
                pt[Y] = y2;
                GET_MAPATT(buff, offset, pt[Z]);
                pt[Z] *= zexag;

                if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                   projMatrix)) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }

                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                if (check_transp) {
                    GET_MAPATT(trbuff, offset, ttr);
                    ktrans = (char)SCALE_ATT(tratt, ttr, 0, 255);
                    ktrans = (char)(255U - ktrans) << 24;
                }

                if (check_material) {
                    if (check_emis) {
                        GET_MAPATT(embuff, offset, kem);
                        kem = SCALE_ATT(ematt, kem, 0., 1.);
                    }

                    if (check_shin) {
                        GET_MAPATT(shbuff, offset, ksh);
                        ksh = SCALE_ATT(shatt, ksh, 0., 1.);
                    }

                    if (pksh != ksh || pkem != kem || (kem && check_color)) {
                        pksh = ksh;
                        pkem = kem;
                        gsd_set_material(check_shin, check_emis, ksh, kem,
                                         curcolor);
                    }
                }

                gsd_litvert_func(n, ktrans | curcolor, pt);

#ifdef CALC_AREA
                GS_v3eq(triv[cnt % 3], pt);

                if (cnt > 1) {
                    GS_v3eq(tedge1, triv[1]);
                    GS_v3eq(tedge2, triv[2]);
                    GS_v3sub(tedge1, triv[0]);
                    GS_v3sub(tedge2, triv[1]);
                    GS_v3cross(tedge1, tedge2, crossp);
                    GS_v3mag(crossp, &mag);
                    asurf += .5 * mag;
                    tedge1[Z] *= sz;
                    tedge2[Z] *= sz;
                    GS_v3cross(tedge1, tedge2, crossp);
                    GS_v3mag(crossp, &mag);
                    axsurf += .5 * mag;
                }
#endif

                cnt++;
            }
        } /* ea col */

        gsd_endtmesh();
    } /* ea row */

    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    show_colormode();

#ifdef CALC_AREA
    G_debug(5, "  Surface Area: %.12lf", asurf);
    G_debug(5, "  Exaggerated Surface Area: %.12lf", axsurf);
#endif

    return (0);
}

/*!
   \brief

   Using tmesh - not confident with qstrips portability

   \todo FIX: do_diff won't work right - needs normals - maybe
   calculate on the fly

   \param surf surface (geosurf)
   \param k

   \return
 */
int gsd_surf_const(geosurf *surf, float k)
{
    int do_diff, check_mask, check_color;
    typbuff *cobuff;
    int xmod, ymod, row, col, cnt, xcnt, ycnt;
    long offset, y1off, y2off;
    float x1, x2, y1, y2, tx, ty, tz;
    float n[3], pt[4], xres, yres, ymax, zexag;
    int col_src, curcolor;
    gsurf_att *coloratt;

    /* Viewport variables */
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    int zeros, dr1, dr2, dr3, dr4;
    int datarow1, datacol1, datarow2, datacol2;

    unsigned int ktrans = 255;

    G_debug(5, "gsd_surf_const(): id=%d", surf->gsurf_id);

    if (GS_check_cancel()) {
        return (-1);
    }

    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);

    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

    /*
       checks ATT_TOPO & ATT_COLOR no_zero flags, make a mask from each,
       combine it/them with any current mask, put in surf->curmask:
     */
    do_diff = (NULL != gsdiff_get_SDref());
    xmod = surf->x_mod;
    ymod = surf->y_mod;
    xres = xmod * surf->xres;
    yres = ymod * surf->yres;

    xcnt = VCOLS(surf);
    ycnt = VROWS(surf);
    ymax = (surf->rows - 1) * surf->yres;

    /* Get Viewport */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);
    /* adjust window */
    window[0] += (int)(yres * 2);
    window[1] -= (int)(yres * 2);
    window[2] -= (int)(xres * 2);
    window[3] += (int)(xres * 2);

    gsd_colormode(CM_DIFFUSE);
    gsd_pushmatrix();

    /* avoid scaling by zero */
    GS_get_scale(&tx, &ty, &tz, 1);

    if (tz == 0.0) {
        k = 0.0;
        gsd_do_scale(0);
    }
    else {
        gsd_do_scale(1);
    }

    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);
    zexag = surf->z_exag;

    if (CONST_ATT == surf->att[ATT_TRANSP].att_src) {
        gsd_blend(1);
        ktrans = 255 - (int)surf->att[ATT_TRANSP].constant;
        gsd_zwritemask(0x0);
    }

    ktrans = (ktrans << 24);

    /* will need to check for color source of FUNC_ATT & NOTSET_ATT,
       or else use more general and inefficient gets */

    check_color = 1;
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

    /* CONSTANTS */
    pt[Z] = k * zexag;
    n[X] = n[Y] = 0.0;
    n[Z] = 1.0;

    /* just draw one polygon if no color mapped */
    /* fast, but specular reflection will prob. be poor */
    if (!check_color && !check_mask && !do_diff) {
        gsd_bgnpolygon();

        pt[X] = pt[Y] = 0;
        gsd_litvert_func(n, ktrans | curcolor, pt);

        pt[X] = xcnt * xres;
        gsd_litvert_func(n, ktrans | curcolor, pt);

        pt[Y] = ycnt * yres;
        gsd_litvert_func(n, ktrans | curcolor, pt);

        pt[X] = 0;
        gsd_litvert_func(n, ktrans | curcolor, pt);

        gsd_endpolygon();
        gsd_popmatrix();
        gsd_blend(0);
        gsd_zwritemask(0xffffffff);

        return (0);
    }

    cnt = 0;

    for (row = 0; row < ycnt; row++) {
        if (GS_check_cancel()) {
            gsd_popmatrix();
            gsd_blend(0);
            gsd_zwritemask(0xffffffff);

            return (-1);
        }

        datarow1 = row * ymod;
        datarow2 = (row + 1) * ymod;

        y1 = ymax - row * yres;
        y2 = ymax - (row + 1) * yres;
        y1off = row * ymod * surf->cols;
        y2off = (row + 1) * ymod * surf->cols;

        gsd_bgntmesh();

        zeros = 0;
        dr1 = dr2 = dr3 = dr4 = 1;

        if (check_mask) {
            if (BM_get(surf->curmask, 0, datarow1)) {
                /*TL*/ ++zeros;
                dr1 = 0;
            }

            if (BM_get(surf->curmask, 0, datarow2)) {
                /*BL*/ ++zeros;
                dr2 = 0;
            }
        }

        if (dr1 && dr2) {
            offset = y1off; /* TL */
            pt[X] = 0;
            pt[Y] = y1;

            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            if (do_diff) {
                pt[Z] = gsdiff_do_SD(k * zexag, offset);
            }

            gsd_litvert_func(n, ktrans | curcolor, pt);
            cnt++;

            offset = y2off; /* BL */
            pt[X] = 0;
            pt[Y] = y2;

            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            if (do_diff) {
                pt[Z] = gsdiff_do_SD(k * zexag, offset);
            }

            gsd_litvert_func(n, ktrans | curcolor, pt);
            cnt++;
        }

        for (col = 0; col < xcnt; col++) {
            datacol1 = col * xmod;
            datacol2 = (col + 1) * xmod;

            x1 = col * xres;
            x2 = (col + 1) * xres;

            zeros = 0;
            dr1 = dr2 = dr3 = dr4 = 1;

            if (check_mask) {
                if (BM_get(surf->curmask, datacol1, datarow1)) {
                    /*TL*/ ++zeros;
                    dr1 = 0;
                }

                if (BM_get(surf->curmask, datacol1, datarow2)) {
                    /*BL*/ ++zeros;
                    dr2 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow2)) {
                    /*BR*/ ++zeros;
                    dr3 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow1)) {
                    /*TR*/ ++zeros;
                    dr4 = 0;
                }

                if ((zeros > 1) && cnt) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }
            }

            if (cnt > 250) {
                cnt = 0;
                gsd_endtmesh();
                gsd_bgntmesh();

                if (dr1) {
                    offset = y1off + datacol1; /* TL */
                    pt[X] = x1;
                    pt[Y] = y1;

                    if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                       projMatrix)) {
                        gsd_endtmesh();
                        cnt = 0;
                        gsd_bgntmesh();
                        continue;
                    }

                    if (check_color) {
                        curcolor = gs_mapcolor(cobuff, coloratt, offset);
                    }

                    if (do_diff) {
                        pt[Z] = gsdiff_do_SD(k * zexag, offset);
                    }

                    gsd_litvert_func(n, ktrans | curcolor, pt);
                    cnt++;
                }

                if (dr2) {
                    offset = y2off + datacol1; /* BL */
                    pt[X] = x1;
                    pt[Y] = y2;

                    if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                       projMatrix)) {
                        gsd_endtmesh();
                        cnt = 0;
                        gsd_bgntmesh();
                        continue;
                    }

                    if (check_color) {
                        curcolor = gs_mapcolor(cobuff, coloratt, offset);
                    }

                    if (do_diff) {
                        pt[Z] = gsdiff_do_SD(k * zexag, offset);
                    }

                    gsd_litvert_func(n, ktrans | curcolor, pt);
                    cnt++;
                }
            }

            if (dr4) {
                offset = y1off + datacol2; /* TR */
                pt[X] = x2;
                pt[Y] = y1;

                if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                   projMatrix)) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }

                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                if (do_diff) {
                    pt[Z] = gsdiff_do_SD(k * zexag, offset);
                }

                gsd_litvert_func(n, ktrans | curcolor, pt);
                cnt++;
            }

            if (dr3) {
                offset = y2off + datacol2; /* BR */
                pt[X] = x2;
                pt[Y] = y2;

                if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                   projMatrix)) {
                    gsd_endtmesh();
                    cnt = 0;
                    gsd_bgntmesh();
                    continue;
                }

                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                if (do_diff) {
                    pt[Z] = gsdiff_do_SD(k * zexag, offset);
                }

                gsd_litvert_func(n, ktrans | curcolor, pt);
                cnt++;
            }
        } /* ea col */

        gsd_endtmesh();
    } /* ea row */

    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (0);
}

/*!
   \brief Define user function

   Not yet supported

   \param gs surface (geosurf) [unused]
   \param user_func user function [unused]

   \return 1
 */
int gsd_surf_func(geosurf *gs UNUSED, int (*user_func)(void) UNUSED)
{

    return (1);
}

/*!
   \brief ADD

   \param npts1
   \param npts2
   \param surf1 first surface (geosurf)
   \param surf2 second surface (geosurf)
   \param points1
   \param points2
   \param norm

   \return 1
 */
int gsd_triangulated_wall(int npts1, int npts2, geosurf *surf1, geosurf *surf2,
                          Point3 *points1, Point3 *points2, float *norm)
{
    int i, i1, i2, nlong, offset, col_src;
    int check_color1, check_color2, color1, color2;
    typbuff *cobuf1, *cobuf2;
    gsurf_att *coloratt1, *coloratt2;

    check_color1 = check_color2 = 1;

    col_src = surf1->att[ATT_COLOR].att_src;

    if (col_src != MAP_ATT) {
        if (col_src == CONST_ATT) {
            color1 = (int)surf1->att[ATT_COLOR].constant;
        }
        else {
            color1 = surf1->wire_color;
        }

        check_color1 = 0;
    }

    coloratt1 = &(surf1->att[ATT_COLOR]);
    cobuf1 = gs_get_att_typbuff(surf1, ATT_COLOR, 0);

    col_src = surf2->att[ATT_COLOR].att_src;
    if (col_src != MAP_ATT) {
        if (col_src == CONST_ATT) {
            color2 = (int)surf2->att[ATT_COLOR].constant;
        }
        else {
            color2 = surf2->wire_color;
        }

        check_color2 = 0;
    }

    coloratt2 = &(surf2->att[ATT_COLOR]);
    cobuf2 = gs_get_att_typbuff(surf2, ATT_COLOR, 0);

    gsd_colormode(CM_DIFFUSE);
    gsd_pushmatrix();
    gsd_do_scale(1);

    gsd_bgntmesh();

    for (nlong = (npts1 > npts2 ? npts1 : npts2), i = 0; i < nlong; i++) {
        i1 = i * npts1 / nlong;
        i2 = i * npts2 / nlong;
        offset = XY2OFF(surf1, points1[i1][X], points1[i1][Y]);

        if (check_color1) {
            color1 = gs_mapcolor(cobuf1, coloratt1, offset);
        }

        offset = XY2OFF(surf1, points2[i2][X], points2[i2][Y]);

        if (check_color2) {
            color2 = gs_mapcolor(cobuf2, coloratt2, offset);
        }

        /* start with long line to ensure triangle */
        if (npts1 > npts2) {
            points1[i1][X] += surf1->x_trans;
            points1[i1][Y] += surf1->y_trans;
            points1[i1][Z] += surf1->z_trans;
            gsd_litvert_func(norm, color1, points1[i1]);
            points2[i2][X] += surf2->x_trans;
            points2[i2][Y] += surf2->y_trans;
            points2[i2][Z] += surf2->z_trans;
            gsd_litvert_func(norm, color2, points2[i2]);
        }
        else {
            points2[i2][X] += surf2->x_trans;
            points2[i2][Y] += surf2->y_trans;
            points2[i2][Z] += surf2->z_trans;
            gsd_litvert_func(norm, color2, points2[i2]);
            points1[i1][X] += surf1->x_trans;
            points1[i1][Y] += surf1->y_trans;
            points1[i1][Z] += surf1->z_trans;
            gsd_litvert_func(norm, color1, points1[i1]);
        }
    }

    gsd_endtmesh();
    gsd_popmatrix();

    return (1);
}

/*!
   \brief ADD

   \param mode
 */
void gsd_setfc(int mode)
{
    FCmode = mode;

    return;
}

/*!
   \brief ADD

   \return
 */
int gsd_getfc(void)
{
    return (FCmode);
}

/*!
   \brief ADD

   \param surf surface (geosurf)
   \param point

   \return
 */
static int transpoint_is_masked(geosurf *surf, Point3 point)
{
    Point3 tp;

    tp[X] = point[X] - surf->x_trans;
    tp[Y] = point[Y] - surf->y_trans;

    return (gs_point_is_masked(surf, tp));
}

/*!
   \brief ADD

   \param points
   \param gsurf
   \param ptn
   \param cursurf
   \param numsurfs
   \param belowsurf

   \return 0 if there is no surface below the current,
   \return -1 if the current surface is masked,
   \return 1 if the surface below the current surface is not masked
   (belowsurf is assigned)
 */
static int get_point_below(Point3 **points, geosurf **gsurfs, int ptn,
                           int cursurf, int numsurfs, int *belowsurf)
{
    int n, found = -1;
    float nearz = 0.0, diff;

    if (gsurfs[cursurf]->curmask) {
        if (transpoint_is_masked(gsurfs[cursurf], points[cursurf][ptn])) {
            return (-1);
        }
    }

    for (n = 0; n < numsurfs; ++n) {
        diff = points[cursurf][ptn][Z] - points[n][ptn][Z];

        if (diff > 0) {
            if (!nearz || diff < nearz) {
                if (gsurfs[n]->curmask) {
                    if (transpoint_is_masked(gsurfs[n], points[n][ptn])) {
                        continue;
                    }
                }

                nearz = diff;
                found = n;
            }
        }
        /* else if (diff == 0.0 && n != cursurf)
           {
           if (gsurfs[n]->curmask)
           {
           if (transpoint_is_masked(gsurfs[n], points[n][ptn]))
           {
           continue;
           }
           }

           nearz=diff;
           found = n;
           break;
           }
         */
    }

    if (found != -1) {
        *belowsurf = found;

        return (1);
    }

    return (0);
}

/*
   #define CPDEBUG
 */

/*!
   \brief ADD

   \param np
   \param ns
   \param gsurfs
   \param points
   \param norm

   \return
 */
int gsd_ortho_wall(int np, int ns, geosurf **gsurfs, Point3 **points,
                   float *norm)
{
    int n, i, offset, col_src, check_color[MAX_SURFS];
    int color, colors[MAX_SURFS], nocolor;
    typbuff *cobuf[MAX_SURFS];
    gsurf_att *coloratt[MAX_SURFS];

    nocolor = FCmode == FC_GREY ? 1 : 0;

    if (!nocolor) {
        for (n = 0; n < ns; ++n) {
            check_color[n] = 1;

            col_src = gsurfs[n]->att[ATT_COLOR].att_src;

            if (col_src != MAP_ATT) {
                if (col_src == CONST_ATT) {
                    colors[n] = (int)gsurfs[n]->att[ATT_COLOR].constant;
                }
                else {
                    colors[n] = gsurfs[n]->wire_color;
                }

                check_color[n] = 0;
            }

            coloratt[n] = &(gsurfs[n]->att[ATT_COLOR]);
            cobuf[n] = gs_get_att_typbuff(gsurfs[n], ATT_COLOR, 0);
        }
    }

#ifdef CPDEBUG
    {
        GS_set_draw(GSD_BOTH);
    }
#endif

    /* changed from CM_DIFFUSE - July 25, 2005
     * should display proper color for cut planes
     */
    gsd_colormode(CM_COLOR);

    /* actually ought to write a GS_set_fencetransp() */
    if (nocolor) {
        color = 0x80808080;
        gsd_blend(1);
        gsd_zwritemask(0x0);
    }

    gsd_pushmatrix();
    gsd_do_scale(1);

    /* using segs_intersect here with segments projected to
       the 2d clipping plane */
    {
        float tx, ty;
        int bn, bnl, ctop, cbot, ctopl, cbotl, bsret;
        Point3 xing;

        if (nocolor) {
            ctop = cbot = ctopl = cbotl = color;
        }

        for (n = 0; n < ns; ++n) {
            for (i = 0; i < np; i++) {
                if (0 <
                    (bsret = get_point_below(points, gsurfs, i, n, ns, &bn))) {
                    gsd_bgntmesh();

                    if (!nocolor) {
                        SET_SCOLOR(n);
                        SET_SCOLOR(bn);

                        if (FCmode == FC_ABOVE) {
                            ctop = cbot = colors[n];
                        }
                        else if (FCmode == FC_BELOW) {
                            ctop = cbot = colors[bn];
                        }
                        else {
                            cbot = colors[bn];
                            ctop = colors[n];
                        }
                    }

                    if (i) {
                        /* need to find crossing? */
                        if (!transpoint_is_masked(gsurfs[n],
                                                  points[n][i - 1]) &&
                            !transpoint_is_masked(gsurfs[bn],
                                                  points[bn][i - 1])) {
                            if (1 == segs_intersect(0.0, points[n][i - 1][Z],
                                                    1.0, points[n][i][Z], 0.0,
                                                    points[bn][i - 1][Z], 1.0,
                                                    points[bn][i][Z], &tx,
                                                    &ty)) {
                                xing[Z] = ty;
                                xing[Y] = points[n][i - 1][Y] +
                                          tx * (points[n][i][Y] -
                                                points[n][i - 1][Y]);
                                xing[X] = points[n][i - 1][X] +
                                          tx * (points[n][i][X] -
                                                points[n][i - 1][X]);
                                gsd_litvert_func(norm, ctop, xing);
                                xing[Z] = points[bn][i - 1][Z] +
                                          tx * (points[bn][i][Z] -
                                                points[bn][i - 1][Z]);
                                gsd_litvert_func(norm, cbot, xing);
                            }
                        }
                    }

                    gsd_litvert_func(norm, ctop, points[n][i]);
                    gsd_litvert_func(norm, cbot, points[bn][i]);
                    i++;

                    bnl = -1;

                    while (i < np && 0 < (bsret = get_point_below(
                                              points, gsurfs, i, n, ns, &bn))) {
#ifdef CPDEBUG
                        {
                            int lower = 0;

                            if (GS_check_cancel()) {
                                break;
                            }
                        }
#endif

                        if (!nocolor) {
                            ctopl = ctop;
                            cbotl = cbot;
                            SET_SCOLOR(n);
                            SET_SCOLOR(bn);

                            if (FCmode == FC_ABOVE) {
                                ctop = cbot = colors[n];
                            }
                            else if (FCmode == FC_BELOW) {
                                ctop = cbot = colors[bn];
                            }
                            else {
                                cbot = colors[bn];
                                ctop = colors[n];
                            }
                        }

                        /*
                           IF UPPER crossing :
                           (crossing is between current & new lower surf)
                           IF XING going DOWN:
                           - plot crossing point (color previous upper)
                           - endtmesh/bgntmesh
                           - plot crossing point (color current upper)
                           - plot "equivalent" point below (color current lower)
                           IF XING going UP:
                           - plot crossing point (color previous upper)
                           - plot "equivalent" point below (color previous
                           lower)
                           - endtmesh/bgntmesh
                           - plot crossing point (color current upper)
                           ELSE IF LOWER crossing:
                           (crossing between new & previous lower surfs):
                           - plot "equivalent" point above (color previous
                           upper)
                           - plot crossing below (color previous lower)
                           - endtmesh/bgntmesh
                           - plot "equivalent" point above (color current upper)
                           - plot crossing below (color current lower)
                         */
                        if (bnl >= 0 && bnl != bn) {
                            /* crossing */
                            float z1, z2;
                            int upper = 0;

                            if (!transpoint_is_masked(gsurfs[n],
                                                      points[n][i - 1]) &&
                                !transpoint_is_masked(gsurfs[bnl],
                                                      points[bnl][i - 1]) &&
                                !transpoint_is_masked(gsurfs[bn],
                                                      points[bn][i - 1])) {

                                if (1 == segs_intersect(
                                             0.0, points[n][i - 1][Z], 1.0,
                                             points[n][i][Z], 0.0,
                                             points[bn][i - 1][Z], 1.0,
                                             points[bn][i][Z], &tx, &ty)) {
                                    /* crossing going up */

                                    G_debug(
                                        5,
                                        "crossing going up at surf %d no. %d",
                                        n, i);

                                    upper = 1;
                                    xing[Z] = ty;
                                    xing[Y] = points[n][i - 1][Y] +
                                              tx * (points[n][i][Y] -
                                                    points[n][i - 1][Y]);
                                    xing[X] = points[n][i - 1][X] +
                                              tx * (points[n][i][X] -
                                                    points[n][i - 1][X]);
                                    gsd_litvert_func(norm, ctopl, xing);
                                    z1 = xing[Z];
                                    xing[Z] = points[bnl][i - 1][Z] +
                                              tx * (points[bnl][i][Z] -
                                                    points[bnl][i - 1][Z]);
                                    gsd_litvert_func(norm, cbotl, xing);
                                    xing[Z] = z1;
                                    gsd_endtmesh();
                                    gsd_bgntmesh();
                                    gsd_litvert_func(norm, ctop, xing);
                                }
                                else if (1 == segs_intersect(
                                                  0.0, points[n][i - 1][Z], 1.0,
                                                  points[n][i][Z], 0.0,
                                                  points[bnl][i - 1][Z], 1.0,
                                                  points[bnl][i][Z], &tx,
                                                  &ty)) {
                                    /* crossing going down */

                                    G_debug(
                                        5,
                                        "crossing going down at surf %d no. %d",
                                        n, i);

                                    upper = 1;
                                    xing[Z] = ty;
                                    xing[Y] = points[n][i - 1][Y] +
                                              tx * (points[n][i][Y] -
                                                    points[n][i - 1][Y]);
                                    xing[X] = points[n][i - 1][X] +
                                              tx * (points[n][i][X] -
                                                    points[n][i - 1][X]);
                                    gsd_litvert_func(norm, ctopl, xing);
                                    z1 = xing[Z];
                                    xing[Z] = points[bnl][i - 1][Z] +
                                              tx * (points[bnl][i][Z] -
                                                    points[bnl][i - 1][Z]);
                                    gsd_litvert_func(norm, cbotl, xing);
                                    xing[Z] = z1;
                                    gsd_endtmesh();
                                    gsd_bgntmesh();
                                    gsd_litvert_func(norm, ctop, xing);
                                    xing[Z] = points[bn][i - 1][Z] +
                                              tx * (points[bn][i][Z] -
                                                    points[bn][i - 1][Z]);
                                    gsd_litvert_func(norm, cbot, xing);
                                }
                            }

                            if (!upper &&
                                !transpoint_is_masked(gsurfs[bn],
                                                      points[bn][i - 1]) &&
                                !transpoint_is_masked(gsurfs[bnl],
                                                      points[bnl][i - 1])) {

                                if (1 == segs_intersect(
                                             0.0, points[bn][i - 1][Z], 1.0,
                                             points[bn][i][Z], 0.0,
                                             points[bnl][i - 1][Z], 1.0,
                                             points[bnl][i][Z], &tx, &ty)) {
#ifdef CPDEBUG
                                    {
                                        lower = 1;
                                    }
#endif
                                    G_debug(5,
                                            "lower crossing at surf %d no. %d "
                                            "between surfs %d & %d",
                                            n, i, bn, bnl);

                                    xing[Z] = ty;
                                    xing[Y] = points[bn][i - 1][Y] +
                                              tx * (points[bn][i][Y] -
                                                    points[bn][i - 1][Y]);
                                    xing[X] = points[bn][i - 1][X] +
                                              tx * (points[bn][i][X] -
                                                    points[bn][i - 1][X]);
                                    z2 = xing[Z];
                                    z1 = xing[Z] = points[n][i - 1][Z] +
                                                   tx * (points[n][i][Z] -
                                                         points[n][i - 1][Z]);
                                    gsd_litvert_func(norm, ctopl, xing);
                                    xing[Z] = z2;
                                    gsd_litvert_func(norm, cbotl, xing);
                                    gsd_endtmesh();
                                    gsd_bgntmesh();
                                    xing[Z] = z1;
                                    gsd_litvert_func(norm, ctop, xing);
                                    xing[Z] = z2;
                                    gsd_litvert_func(norm, cbot, xing);
                                }
                            }

#ifdef CPDEBUG
                            {
                                if (!upper && !lower) {
                                    G_debug(5, "Crossing NOT found or masked:");
                                    G_debug(5,
                                            "  current surf: %d [ %.2f %.2f "
                                            "%.2f -> %.2f %.2f %f",
                                            n, points[n][i - 1][X],
                                            points[n][i - 1][Y],
                                            points[n][i - 1][Z],
                                            points[n][i][X], points[n][i][Y],
                                            points[n][i][Z]);
                                    G_debug(5,
                                            "  below surf: %d [ %.2f %.2f %.2f "
                                            "-> %.2f %.2f %f\n",
                                            bn, points[bn][i - 1][X],
                                            points[bn][i - 1][Y],
                                            points[bn][i - 1][Z],
                                            points[bn][i][X], points[bn][i][Y],
                                            points[bn][i][Z]);
                                    G_debug(5,
                                            gs "  last below surf: %d [ %.2f "
                                               "%.2f %.2f -> %.2f %.2f %f\n",
                                            bnl, points[bnl][i - 1][X],
                                            points[bnl][i - 1][Y],
                                            points[bnl][i - 1][Z],
                                            points[bnl][i][X],
                                            points[bnl][i][Y],
                                            points[bnl][i][Z]);
                                }
                            }
#endif
                        }

                        gsd_litvert_func(norm, ctop, points[n][i]);
                        gsd_litvert_func(norm, cbot, points[bn][i]);
                        bnl = bn;
                        i++;
                    }

                    if (i < np) {
                        /* need to find crossing? */
                        if (!transpoint_is_masked(gsurfs[n],
                                                  points[n][i - 1]) &&
                            !transpoint_is_masked(gsurfs[bn],
                                                  points[bn][i - 1])) {
                            if (1 == segs_intersect(0.0, points[n][i - 1][Z],
                                                    1.0, points[n][i][Z], 0.0,
                                                    points[bn][i - 1][Z], 1.0,
                                                    points[bn][i][Z], &tx,
                                                    &ty)) {
                                xing[Z] = ty;
                                xing[Y] = points[n][i - 1][Y] +
                                          tx * (points[n][i][Y] -
                                                points[n][i - 1][Y]);
                                xing[X] = points[n][i - 1][X] +
                                          tx * (points[n][i][X] -
                                                points[n][i - 1][X]);
                                gsd_litvert_func(norm, ctop, xing);
                            }

                            i--;
                        }
                    }

                    gsd_endtmesh();
                }
            }
        }
    }

    gsd_colormode(CM_DIFFUSE); /* set colormode back to DIFFUSE */
    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (1);
}

/*!
   \brief ADD

   bgn,end should already be in world modeling coords, but have to
   be reverse-translated to apply to each surface

   \param bgn,end  2d line for cutting plane
   \param norm indicates which way wall faces

   \return
 */
int gsd_wall(float *bgn, float *end, float *norm)
{
    geosurf *gsurfs[MAX_SURFS];
    Point3 *points[MAX_SURFS], *tmp;
    int nsurfs, ret, npts, npts1, n, i, err = 0;
    float bgn1[2], end1[2];

    if (norm[Z] > 0.0001 || norm[Z] < -.0001) {
        return (0); /* can't do tilted wall yet */
    }

    if (FCmode == FC_OFF) {
        return (0);
    }

    nsurfs = gs_getall_surfaces(gsurfs);

    for (n = 0; n < nsurfs; n++) {
        /* get drape points for surf */
        bgn1[X] = bgn[X] - gsurfs[n]->x_trans;
        bgn1[Y] = bgn[Y] - gsurfs[n]->y_trans;
        end1[X] = end[X] - gsurfs[n]->x_trans;
        end1[Y] = end[Y] - gsurfs[n]->y_trans;
        tmp = gsdrape_get_allsegments(gsurfs[n], bgn1, end1, &npts1);

        if (n) {
            if (npts != npts1) {
                G_warning(_("Cut-plane points mismatch between surfaces. "
                            "Check resolution(s)."));
                err = 1;
                nsurfs = n;

                break;
            }
        }

        npts = npts1;

        if (n == nsurfs - 1) {
            /* last surf - don't need to copy */
            points[n] = tmp;

            for (i = 0; i < npts1; i++) {
                /* DOING translation here! */
                points[n][i][X] += gsurfs[n]->x_trans;
                points[n][i][Y] += gsurfs[n]->y_trans;
                points[n][i][Z] += gsurfs[n]->z_trans;
            }

            break;
        }

        /* allocate space in points and copy tmp to points */
        points[n] =
            (Point3 *)G_calloc(npts1, sizeof(Point3)); /* G_fatal_error */

        for (i = 0; i < npts1; i++) {
            GS_v3eq(points[n][i], tmp[i]);

            /* DOING translation here! */
            points[n][i][X] += gsurfs[n]->x_trans;
            points[n][i][Y] += gsurfs[n]->y_trans;
            points[n][i][Z] += gsurfs[n]->z_trans;
        }
    } /* done for */

    if (err) {
        for (n = 0; n < nsurfs; n++) {
            if (points[n]) {
                G_free(points[n]);
            }
        }
        return (0);
    }

    ret = gsd_ortho_wall(npts, nsurfs, gsurfs, points, norm);

    for (n = 0; n < nsurfs - 1; n++) {
        /* don't free last - it's constant */
        G_free(points[n]);
    }

    return (ret);
}

/*!
   \brief ADD

   Need to do Zexag scale of normal for arrow direction, drawing
   routine unexags z for arrow

   \param surf surface (geosurf)

   \return
 */
int gsd_norm_arrows(geosurf *surf)
{
    typbuff *buff;
    int check_mask;
    int xmod, ymod, row, col, cnt, xcnt, ycnt;
    long offset, y1off, y2off;
    float /* x1, */ x2, y1, y2, tx, ty, tz, sz;
    float n[3], pt[4], xres, yres, ymax, zexag;

#ifdef DO_ARROW_SOLID
    int col_src;
    typbuff *cobuff;
    gsurf_att *coloratt;
    int curcolor;
    int check_color = 1;
#endif
    int zeros, dr1, dr2, dr3, dr4;
    int datarow1, datacol1, datarow2, datacol2;

    G_debug(3, "gsd_norm_arrows");

    /* avoid scaling by zero */
    GS_get_scale(&tx, &ty, &tz, 1);

    if (tz == 0.0) {
        return (0);
    }

    sz = GS_global_exag();

    /*
       checks ATT_TOPO & ATT_COLOR no_zero flags, make a mask from each,
       combine it/them with any current mask, put in surf->curmask:
     */
    gs_update_curmask(surf);
    check_mask = surf->curmask ? 1 : 0;

#ifdef DO_ARROW_SOLID
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

    cobuff = gs_get_att_typbuff(surf, ATT_COLOR, 0);
#endif

    buff = gs_get_att_typbuff(surf, ATT_TOPO, 0);

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
    /* CURRENTLY ALWAYS 1.0 */

#ifdef DO_ARROW_SOLID
    gsd_colormode(CM_DIFFUSE);
#else
    gsd_colormode(CM_COLOR);
#endif

    cnt = 0;

    for (row = 0; row < ycnt; row++) {
        if (GS_check_cancel()) {
            gsd_popmatrix();

            return (-1);
        }

        datarow1 = row * ymod;
        datarow2 = (row + 1) * ymod;

        y1 = ymax - row * yres;
        y2 = ymax - (row + 1) * yres;
        y1off = row * ymod * surf->cols;
        y2off = (row + 1) * ymod * surf->cols;

        zeros = 0;
        dr1 = dr2 = dr3 = dr4 = 1;

        if (check_mask) {
            if (BM_get(surf->curmask, 0, datarow1)) {
                /*TL*/ ++zeros;
                dr1 = 0;
            }

            if (BM_get(surf->curmask, 0, datarow2)) {
                /*BL*/ ++zeros;
                dr2 = 0;
            }
        }

        if (dr1 && dr2) {
            offset = y1off; /* TL */
            FNORM(surf->norms[offset], n);
            pt[X] = 0;
            pt[Y] = y2;
            GET_MAPATT(buff, offset, pt[Z]);
            pt[Z] *= zexag;

#ifdef DO_ARROW_SOLID
            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            gsd_3darrow(pt, curcolor, xres * 2, xres / 2, n, sz);
#else
            if (DEBUG_ARROW) {
                gsd_arrow(pt, 0x000000, xres * 2, n, sz, surf);
            }
#endif

            cnt++;

            offset = y2off; /* BL */
            FNORM(surf->norms[offset], n);
            pt[X] = 0;
            pt[Y] = y2;
            GET_MAPATT(buff, offset, pt[Z]);
            pt[Z] *= zexag;

#ifdef DO_ARROW_SOLID
            if (check_color) {
                curcolor = gs_mapcolor(cobuff, coloratt, offset);
            }

            gsd_3darrow(pt, curcolor, xres * 2, xres / 2, n, sz);
#else
            if (DEBUG_ARROW) {
                gsd_arrow(pt, 0x000000, xres * 2, n, sz, surf);
            }
#endif

            cnt++;
        }

        for (col = 0; col < xcnt; col++) {
            datacol1 = col * xmod;
            datacol2 = (col + 1) * xmod;

            /* x1 = col * xres; */
            x2 = (col + 1) * xres;

            zeros = 0;
            dr1 = dr2 = dr3 = dr4 = 1;

            if (check_mask) {
                if (BM_get(surf->curmask, datacol1, datarow1)) {
                    /*TL*/ ++zeros;
                    dr1 = 0;
                }

                if (BM_get(surf->curmask, datacol1, datarow2)) {
                    /*BL*/ ++zeros;
                    dr2 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow2)) {
                    /*BR*/ ++zeros;
                    dr3 = 0;
                }

                if (BM_get(surf->curmask, datacol2, datarow1)) {
                    /*TR*/ ++zeros;
                    dr4 = 0;
                }

                if ((zeros > 1) && cnt) {
                    cnt = 0;
                    continue;
                }
            }

            if (dr4) {
                offset = y1off + datacol2; /* TR */
                FNORM(surf->norms[offset], n);
                pt[X] = x2;
                pt[Y] = y1;
                GET_MAPATT(buff, offset, pt[Z]);
                pt[Z] *= zexag;

#ifdef DO_ARROW_SOLID
                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                gsd_3darrow(pt, curcolor, xres * 2, xres / 2, n, sz);
#else
                if (DEBUG_ARROW) {
                    gsd_arrow(pt, 0x000000, xres * 2, n, sz, surf);
                }
#endif

                cnt++;
            }

            if (dr3) {
                offset = y2off + datacol2; /* BR */
                FNORM(surf->norms[offset], n);
                pt[X] = x2;
                pt[Y] = y2;
                GET_MAPATT(buff, offset, pt[Z]);
                pt[Z] *= zexag;

#ifdef DO_ARROW_SOLID
                if (check_color) {
                    curcolor = gs_mapcolor(cobuff, coloratt, offset);
                }

                gsd_3darrow(pt, curcolor, xres * 2, xres / 2, n, sz);
#else
                if (DEBUG_ARROW) {
                    gsd_arrow(pt, 0x000000, xres * 2, n, sz, surf);
                }
#endif

                cnt++;
            }
        } /* ea col */
    } /* ea row */
    gsd_popmatrix();

    return (1);
}

/*!
   \brief Draw surface using triangle fan instead of strip

   Optimized by getting rid of BM_get mask check - GET_MAPPATT does same
   and returns zero if masked

   Only do in window check on Fan center(v0) to further optimize -- this runs
   the risk of trimming points in view !!

   \param surf surface (geosurf)

   \return
 */
int gsd_surf_map(geosurf *surf)
{
    int /* check_mask, */ check_color, check_transp;
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

    /* Viewport variables for accelerated drawing */
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    int datacol1, datacol2, datacol3;

    /* int datarow1, datarow2, datarow3; */

    float kem, ksh, pkem, pksh;
    unsigned int ktrans;

    int step_val = 2;  /* should always be factor of 2 for fan */
    int start_val = 1; /* one half of step_val */

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
    /* check_mask = surf->curmask ? 1 : 0; */

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

    /* Get viewport */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);

    gsd_colormode(CM_DIFFUSE);
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(surf->x_trans, surf->y_trans, surf->z_trans);
    zexag = surf->z_exag;

    /* adjust window */
    window[0] += (int)(yres * 4 * zexag);
    window[1] -= (int)(yres * 4 * zexag);
    window[2] -= (int)(xres * 4 * zexag);
    window[3] += (int)(xres * 4 * zexag);

    /* CURRENTLY ALWAYS 1.0 */
#ifdef CALC_AREA
    sz = GS_global_exag();
#endif

    /* TODO: get rid of (define) these magic numbers scaling the attribute vals
     */
    check_transp = 0;
    tratt = &(surf->att[ATT_TRANSP]);
    ktrans = (255U << 24);
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

    for (row = start_val; row < ycnt; row += step_val) {
        if (GS_check_cancel()) {
            gsd_popmatrix();
            gsd_blend(0);
            gsd_zwritemask(0xffffffff);

            return (-1);
        }

        /*
           if (row == 201 && new_fan == 0) {
           xmod *= 2;
           ymod *= 2;
           xres = xmod * surf->xres;
           yres = ymod * surf->yres;
           step_val *= 2;
           new_fan = 1;
           row -= 1;
           row /= 2;
           }
         */
        /*
           datarow1 = row * ymod;
           datarow2 = (row - (step_val / 2)) * ymod;
           datarow3 = (row + (step_val / 2)) * ymod;
         */

        y1 = ymax - row * yres;
        y2 = ymax - (row - (step_val / 2)) * yres;
        y3 = ymax - (row + (step_val / 2)) * yres;

        y1off = row * ymod * surf->cols;
        y2off = (row - (step_val / 2)) * ymod * surf->cols;
        y3off = (row + (step_val / 2)) * ymod * surf->cols;

        for (col = start_val; col < xcnt; col += step_val) {
            datacol1 = col * xmod;
            datacol2 = (col - (step_val / 2)) * xmod;
            datacol3 = (col + (step_val / 2)) * xmod;

            x1 = col * xres;
            x2 = (col - (step_val / 2)) * xres;
            x3 = (col + (step_val / 2)) * xres;

            /* 0 */
            /*
               if (check_mask) {
               if (BM_get(surf->curmask, datacol1, datarow1))
               continue;
               }
             */

            /* Do not need BM_get because GET_MAPATT calls
             * same and returns zero if masked
             */
            offset2[0] = y1off + datacol1; /* fan center */
            pt2[0][X] = x1;
            pt2[0][Y] = y1; /* fan center */
            pt[X] = pt2[0][X];
            pt[Y] = pt2[0][Y];
            if (!GET_MAPATT(buff, offset2[0], pt[Z]))
                continue; /* masked */
            else {
                pt[Z] *= zexag;
                if (gsd_checkpoint(pt, window, viewport, modelMatrix,
                                   projMatrix))
                    continue;
            }

            offset2[1] = y2off + datacol2;
            offset2[2] = y2off + datacol1;
            offset2[3] = y2off + datacol3;
            offset2[4] = y1off + datacol3;
            offset2[5] = y3off + datacol3;
            offset2[6] = y3off + datacol1;
            offset2[7] = y3off + datacol2;
            offset2[8] = y1off + datacol2;
            offset2[9] = y2off + datacol2; /* repeat 1st corner to close */

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
            pt2[9][Y] = y2; /* repeat 1st corner to close */

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
                    ktrans = (char)(255U - ktrans) << 24;
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
                        gsd_set_material(check_shin, check_emis, ksh, kem,
                                         curcolor);
                    }
                }

                gsd_litvert_func(n, ktrans | curcolor, pt);

            } /* close ii loop */
            gsd_endtfan();
        } /* end col */
    } /* end row */

    gsd_popmatrix();
    gsd_blend(0);
    gsd_zwritemask(0xffffffff);

    return (0);
}
