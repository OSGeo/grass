
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

#include <grass/waterglobs.h>

/* divergence computation from a given field */

void erod(double **hw)
{
    /* hw = sigma or gamma */

    double dyp, dyn, dya, dxp, dxn, dxa;
    int k, l;
    int l1, lp, k1, kp, ln, kn, k2, l2;


    for (k = 0; k < my; k++) {
	for (l = 0; l < mx; l++) {
	    lp = max(0, l - 2);
	    l1 = lp + 1;
	    kp = max(0, k - 2);
	    k1 = kp + 1;
	    ln = min(mx - 1, l + 1);
	    l2 = ln - 1;
	    kn = min(my - 1, k + 1);
	    k2 = kn - 1;

	    if (zz[k][l] != UNDEF || zz[k][ln] != UNDEF || zz[kp][l] != UNDEF || zz[k][lp] != UNDEF || zz[k][l1] != UNDEF || zz[k1][l] != UNDEF || zz[kn][l] != UNDEF) {	/* jh fix */

		dxp = (v1[k][lp] * hw[k][lp] - v1[k][l1] * hw[k][l1]) / stepx;
		dxn = (v1[k][l2] * hw[k][l2] - v1[k][ln] * hw[k][ln]) / stepx;
		dxa = 0.5 * (dxp + dxn);


		dyp = (v2[kp][l] * hw[kp][l] - v2[k1][l] * hw[k1][l]) / stepy;
		dyn = (v2[k2][l] * hw[k2][l] - v2[kn][l] * hw[kn][l]) / stepy;
		dya = 0.5 * (dyp + dyn);

		er[k][l] = (dxa + dya) / deltap;

	    }
	    else
		er[k][l] = UNDEF;

	}
    }
}
