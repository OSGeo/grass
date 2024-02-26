/****************************************************************************
 *
 * MODULE:       r.smooth
 * AUTHOR(S):    Maris Nartiss maris.gis gmail.com
 *
 * PURPOSE:      Provides smoothing with anisotropic diffusion according to:
 *               Perona P. and Malik J. 1990. Scale-space and edge detection
 *               using anisotropic diffusion. IEEE transactions on pattern
 *               analysis and machine intelligence, 12(7).
 *               Tukey's conductance function according to:
 *               Black M.J., Sapiro G., Marimont D.H. and Heeger D. 1998.
 *               Robust anisotropic diffusion. IEEE transactions on image
 *               processing, 7(3).
 *
 *
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include "local_proto.h"

/*!
 * \fn double diffuse(int m, float K, float l, double ij, double n, double e,
 * double s, double w)
 *
 * \brief Calculate anisotropic diffusion value for a single cell
 *
 * Conductance functions 1 & 2 are after Perona P. and Malik J. 1990,
 * 3 is after Black et al. 1998.
 *
 * \param m Conductance f.: 1 - quadratic; 2 - exponential; 3 - Tukey's biweight
 * \param K Gradient magnitude threshold. K > 0.0
 * \param l Rate of diffusion. 0.0 <= l < 1.0
 * \param ij Center cell value I(i,j)
 * \param n,e,s,w Cell values [i+-1][j+-1]
 * \return double New value of I(i,j) cell
 */

double diffuse(int m, float K, float dt, double ij, double n, double e,
               double s, double w, double nw, double ne, double se, double sw)
{
    double gN, gS, gW, gE, gNW, gSW, gNE, gSE;
    double cN, cS, cW, cE, cNW, cSW, cNE, cSE;
    double Iij, K2, S;

    /* Gradient in 8 directions */
    gN = n - ij;
    gE = e - ij;
    gS = s - ij;
    gW = w - ij;
    gNW = (nw - ij) * 1.4142135;
    gSW = (sw - ij) * 1.4142135;
    gNE = (ne - ij) * 1.4142135;
    gSE = (se - ij) * 1.4142135;

    /* Conductance coefficient calculation */
    K2 = K * K;

    /* Perona & Malik 1st conductance function = exponential */
    if (m == 1) {
        cN = exp(-1.0 * ((gN * gN) / K2));
        cE = exp(-1.0 * ((gE * gE) / K2));
        cS = exp(-1.0 * ((gS * gS) / K2));
        cW = exp(-1.0 * ((gW * gW) / K2));
        cNW = exp(-1.0 * ((gNW * gNW) / K2));
        cSW = exp(-1.0 * ((gSW * gSW) / K2));
        cNE = exp(-1.0 * ((gNE * gNE) / K2));
        cSE = exp(-1.0 * ((gSE * gSE) / K2));
    }

    /* Perona & Malik 2nd conductance function = quadratic */
    if (m == 2) {
        cN = 1 / (1 + ((gN * gN) / K2));
        cE = 1 / (1 + ((gE * gE) / K2));
        cS = 1 / (1 + ((gS * gS) / K2));
        cNW = 1 / (1 + ((gNW * gNW) / K2));
        cSW = 1 / (1 + ((gSW * gSW) / K2));
        cNE = 1 / (1 + ((gNE * gNE) / K2));
        cSE = 1 / (1 + ((gSE * gSE) / K2));
    }

    /* Black et al. 1998 Tukey's biweight function */
    if (m == 3) {
        S = K * sqrt(2);
        if (S < fabs(gN) || S < fabs(gE) || S < fabs(gS) || S < fabs(gW) ||
            S < fabs(gNW) || S < fabs(gNE) || S < fabs(gSE) || S < fabs(gSW)) {
            cN = 0;
            cE = 0;
            cS = 0;
            cW = 0;
            cNW = 0;
            cNE = 0;
            cSE = 0;
            cSW = 0;
        }
        else {
            cN = 0.5 *
                 ((1 - ((gN * gN) / (S * S))) * (1 - ((gN * gN) / (S * S))));
            cE = 0.5 *
                 ((1 - ((gE * gE) / (S * S))) * (1 - ((gE * gE) / (S * S))));
            cS = 0.5 *
                 ((1 - ((gS * gS) / (S * S))) * (1 - ((gS * gS) / (S * S))));
            cW = 0.5 *
                 ((1 - ((gW * gW) / (S * S))) * (1 - ((gW * gW) / (S * S))));
            cNW = 0.5 * ((1 - ((gNW * gNW) / (S * S))) *
                         (1 - ((gNW * gNW) / (S * S))));
            cNE = 0.5 * ((1 - ((gNE * gNE) / (S * S))) *
                         (1 - ((gNE * gNE) / (S * S))));
            cSE = 0.5 * ((1 - ((gSE * gSE) / (S * S))) *
                         (1 - ((gSE * gSE) / (S * S))));
            cSW = 0.5 * ((1 - ((gSW * gSW) / (S * S))) *
                         (1 - ((gSW * gSW) / (S * S))));
        }
    }

    /* Calculate new ij value */
    Iij = ij + dt * (gN * cN + gE * cE + gS * cS + gW * cW + gNW * cNW +
                     gNE * cNE + gSE * cSE + gSW * cSW);
    return Iij;
}
