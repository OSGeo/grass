/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from:
 *                         prof. Giulio Antoniol - antoniol@ieee.org
 *                         prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *               Markus Metz (optimization and bug fixes)
 *
 * PURPOSE:      Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2003 by University of Sannio (BN) - Italy
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "h_measure.h"

float h_measure(int t_m, struct matvec *mv)
{
    switch (t_m) {
        /* Angular Second Moment */
    case 1:
        return (f1_asm(mv));
        break;

        /* Contrast */
    case 2:
        return (f2_contrast(mv));
        break;

        /* Correlation */
    case 3:
        return (f3_corr(mv));
        break;

        /* Variance */
    case 4:
        return (f4_var(mv));
        break;

        /* Inverse Diff Moment */
    case 5:
        return (f5_idm(mv));
        break;

        /* Sum Average */
    case 6:
        return (f6_savg(mv));
        break;

        /* Sum Variance */
    case 7:
        return (f7_svar(mv));
        break;

        /* Sum Entropy */
    case 8:
        return (f8_sentropy(mv));
        break;

        /* Entropy */
    case 9:
        return (f9_entropy(mv));
        break;

        /* Difference Variance */
    case 10:
        return (f10_dvar(mv));
        break;

        /* Difference Entropy */
    case 11:
        return (f11_dentropy(mv));
        break;

        /* Measure of Correlation-1 */
    case 12:
        return (f12_icorr(mv));
        break;

        /* Measure of Correlation-2 */
    case 13:
        return (f13_icorr(mv));
        break;
    }

    return 0;
}

/* Angular Second Moment */
/*
 * The angular second-moment feature (ASM) f1 is a measure of homogeneity
 * of the image. In a homogeneous image, there are very few dominant
 * gray-tone transitions. Hence the P matrix for such an image will have
 * fewer entries of large magnitude.
 */
float f1_asm(struct matvec *mv)
{
    int i, j;
    float sum = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;

    /*
       for (i = 0; i < Ng; i++)
       for (j = 0; j < Ng; j++)
       sum += P[i][j] * P[i][j];
     */

    for (i = 0; i < Ng; i++) {
        sum += P[i][i] * P[i][i];
        for (j = 0; j < i; j++)
            sum += 2 * P[i][j] * P[i][j];
    }

    return sum;
}

/* Contrast */
/*
 * The contrast feature is a difference moment of the P matrix and is a
 * measure of the contrast or the amount of local variations present in an
 * image.
 */
float f2_contrast(struct matvec *mv)
{
    int i, j /*, n */;
    float /* sum, */ bigsum = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    int *tone = mv->tone;

    /* the three-loop version does not work
     * when gray tones that do not occur in the current window
     * have been removed in tone and P* */
    /*
       for (n = 0; n < Ng; n++) {
       sum = 0;
       for (i = 0; i < Ng; i++) {
       for (j = 0; j < Ng; j++) {
       if ((i - j) == n ||
       (j - i) == n) {
       sum += P[i][j];
       }
       }
       }
       bigsum += n * n * sum;
       }
     */

    /* two-loop version */
    for (i = 0; i < Ng; i++) {
        for (j = 0; j < i; j++) {
            bigsum += 2 * P[i][j] * (tone[i] - tone[j]) * (tone[i] - tone[j]);
        }
    }

    return bigsum;
}

/* Correlation */
/*
 * This correlation feature is a measure of gray-tone linear-dependencies
 * in the image.
 */
float f3_corr(struct matvec *mv)
{
    int i, j;
    float sum_sqr = 0, tmp = 0;
    float mean = 0, stddev;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    int *tone = mv->tone;
    float *px = mv->px;

    /* Now calculate the means and standard deviations of px and py */

    /*- fix supplied by J. Michael Christensen, 21 Jun 1991 */

    /*- further modified by James Darrell McCauley, 16 Aug 1991
     *     after realizing that meanx=meany and stddevx=stddevy
     */
    for (i = 0; i < Ng; i++) {
        mean += px[i] * tone[i];
        sum_sqr += px[i] * tone[i] * tone[i];

        for (j = 0; j < Ng; j++)
            tmp += tone[i] * tone[j] * P[i][j];
    }
    stddev = sqrt(sum_sqr - (mean * mean));

    if (stddev == 0) /* stddev < GRASS_EPSILON or similar ? */
        return 0;

    return (tmp - mean * mean) / (stddev * stddev);
}

/* Sum of Squares: Variance */
float f4_var(struct matvec *mv)
{
    int i, j;
    float mean = 0, var = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    int *tone = mv->tone;

    /*- Corrected by James Darrell McCauley, 16 Aug 1991
     *  calculates the mean intensity level instead of the mean of
     *  cooccurrence matrix elements
     */
    for (i = 0; i < Ng; i++)
        for (j = 0; j < Ng; j++)
            mean += tone[i] * P[i][j];

    for (i = 0; i < Ng; i++)
        for (j = 0; j < Ng; j++)
            var += (tone[i] - mean) * (tone[i] - mean) * P[i][j];

    return var;
}

/* Inverse Difference Moment */
float f5_idm(struct matvec *mv)
{
    int i, j;
    float idm = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    int *tone = mv->tone;

    /*
       for (i = 0; i < Ng; i++)
       for (j = 0; j < Ng; j++)
       idm += P[i][j] / (1 + (tone[i] - tone[j]) * (tone[i] - tone[j]));
     */

    for (i = 0; i < Ng; i++) {
        idm += P[i][i];
        for (j = 0; j < i; j++)
            idm +=
                2 * P[i][j] / (1 + (tone[i] - tone[j]) * (tone[i] - tone[j]));
    }

    return idm;
}

/* Sum Average */
float f6_savg(struct matvec *mv)
{
    int i, j, k;
    float savg = 0;
    float *P = mv->Pxpys;
    int Ng = mv->Ng;
    int *tone = mv->tone;

    /*
       for (i = 0; i < 2 * Ng - 1; i++)
       savg += (i + 2) * Pxpys[i];
     */

    for (i = 0; i < Ng; i++) {
        for (j = 0; j < Ng; j++) {
            k = i + j;
            savg += (tone[i] + tone[j]) * P[k];
        }
    }

    return savg;
}

/* Sum Variance */
float f7_svar(struct matvec *mv)
{
    int i, j, k;
    float var = 0;
    float *P = mv->Pxpys;
    int Ng = mv->Ng;
    int *tone = mv->tone;
    float savg = f6_savg(mv);
    float tmp;

    /*
       for (i = 0; i < 2 * Ng - 1; i++)
       var += (i + 2 - savg) * (i + 2 - savg) * Pxpys[i];
     */

    for (i = 0; i < Ng; i++) {
        for (j = 0; j < Ng; j++) {
            k = i + j;
            tmp = tone[i] + tone[j] - savg;
            var += tmp * tmp * P[k];
        }
    }

    return var;
}

/* Sum Entropy */
float f8_sentropy(struct matvec *mv)
{
    int i;
    float sentr = 0;
    float *P = mv->Pxpys;
    int Ng = mv->Ng;

    for (i = 0; i < 2 * Ng - 1; i++) {
        if (P[i] > 0)
            sentr -= P[i] * log2(P[i]);
    }

    return sentr;
}

/* Entropy */
float f9_entropy(struct matvec *mv)
{
    int i, j;
    float entropy = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;

    /*
       for (i = 0; i < Ng; i++) {
       for (j = 0; j < Ng; j++) {
       if (P[i][j] > 0)
       entropy += P[i][j] * log2(P[i][j]);
       }
       }
     */

    for (i = 0; i < Ng; i++) {
        if (P[i][i] > 0)
            entropy += P[i][i] * log2(P[i][i]);
        for (j = 0; j < i; j++) {
            if (P[i][j] > 0)
                entropy += 2 * P[i][j] * log2(P[i][j]);
        }
    }

    return -entropy;
}

/* Difference Variance */
float f10_dvar(struct matvec *mv)
{
    int i, tmp;
    float sum = 0, sum_sqr = 0, var = 0;
    float *P = mv->Pxpyd;
    int Ng = mv->Ng;
    int *tone = mv->tone;

    /* Now calculate the variance of Pxpy (Px-y) */
    for (i = 0; i < Ng; i++) {
        sum += P[i];
        sum_sqr += P[i] * P[i];
    }
    /* tmp = Ng * Ng; */
    if (Ng > 1) {
        tmp = (tone[Ng - 1] - tone[0]) * (tone[Ng - 1] - tone[0]);
        var = ((tmp * sum_sqr) - (sum * sum)) / (tmp * tmp);
    }

    return var;
}

/* Difference Entropy */
float f11_dentropy(struct matvec *mv)
{
    int i;
    float sum = 0;
    float *P = mv->Pxpyd;
    int Ng = mv->Ng;

    for (i = 0; i < Ng; i++) {
        if (P[i] > 0)
            sum += P[i] * log2(P[i]);
    }

    return -sum;
}

/* Information Measures of Correlation */
float f12_icorr(struct matvec *mv)
{
    int i, j;
    float hx = 0, hy = 0, hxy = 0, hxy1 = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    float *px = mv->px;
    float *py = mv->py;

    for (i = 0; i < Ng; i++) {
        for (j = 0; j < Ng; j++) {
            if (px[i] * py[j] > 0)
                hxy1 -= P[i][j] * log2(px[i] * py[j]);
            if (P[i][j] > 0)
                hxy -= P[i][j] * log2(P[i][j]);
        }

        /* Calculate entropies of px and py - is this right? */
        if (px[i] > 0)
            hx -= px[i] * log2(px[i]);
        if (py[i] > 0)
            hy -= py[i] * log2(py[i]);
    }

    /* fprintf(stderr,"hxy1=%f\thxy=%f\thx=%f\thy=%f\n",hxy1,hxy,hx,hy); */
    if (hx == 0 && hy == 0)
        return 0;

    return ((hxy - hxy1) / (hx > hy ? hx : hy));
}

/* Information Measures of Correlation */
float f13_icorr(struct matvec *mv)
{
    int i, j;
    float hxy = 0, hxy2 = 0;
    float **P = mv->P_matrix;
    int Ng = mv->Ng;
    float *px = mv->px;
    float *py = mv->py;

    for (i = 0; i < Ng; i++) {
        for (j = 0; j < Ng; j++) {
            if (px[i] * py[j] > 0)
                hxy2 -= px[i] * py[j] * log2(px[i] * py[j]);
            if (P[i][j] > 0)
                hxy -= P[i][j] * log2(P[i][j]);
        }
    }

    /* fprintf(stderr,"hx=%f\thxy2=%f\n",hx,hxy2); */
    return (sqrt(fabs(1 - exp(-2.0 * (hxy2 - hxy)))));
}
