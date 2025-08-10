/****************************************************************************
 *
 * MODULE:       simwe library
 * AUTHOR(S):    Helena Mitasova, Jaro Hofierka, Lubos Mitas:
 * PURPOSE:      Hydrologic and sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* erod.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/simlib.h>

/* divergence computation from a given field */

void erod(double **hw, const Setup *setup, const Geometry *geometry,
          Grids *grids)
{
    /* hw = sigma or gamma */

    double dyp, dyn, dya, dxp, dxn, dxa;
    int k, l;
    int l1, lp, k1, kp, ln, kn, k2, l2;

    for (k = 0; k < geometry->my; k++) {
        for (l = 0; l < geometry->mx; l++) {
            lp = max(0, l - 2);
            l1 = lp + 1;
            kp = max(0, k - 2);
            k1 = kp + 1;
            ln = min(geometry->mx - 1, l + 1);
            l2 = ln - 1;
            kn = min(geometry->my - 1, k + 1);
            k2 = kn - 1;

            if (grids->zz[k][l] != UNDEF || grids->zz[k][ln] != UNDEF ||
                grids->zz[kp][l] != UNDEF || grids->zz[k][lp] != UNDEF ||
                grids->zz[k][l1] != UNDEF || grids->zz[k1][l] != UNDEF ||
                grids->zz[kn][l] != UNDEF) { /* jh fix */

                dxp = (grids->v1[k][lp] * hw[k][lp] -
                       grids->v1[k][l1] * hw[k][l1]) /
                      geometry->stepx;
                dxn = (grids->v1[k][l2] * hw[k][l2] -
                       grids->v1[k][ln] * hw[k][ln]) /
                      geometry->stepx;
                dxa = 0.5 * (dxp + dxn);

                dyp = (grids->v2[kp][l] * hw[kp][l] -
                       grids->v2[k1][l] * hw[k1][l]) /
                      geometry->stepy;
                dyn = (grids->v2[k2][l] * hw[k2][l] -
                       grids->v2[kn][l] * hw[kn][l]) /
                      geometry->stepy;
                dya = 0.5 * (dyp + dyn);

                grids->er[k][l] = (dxa + dya) / setup->deltap;
            }
            else
                grids->er[k][l] = UNDEF;
        }
    }
}
