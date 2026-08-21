/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * US Army Construction Engineering Research Lab
 * SPDX-FileCopyrightText: 1993 H. Mitasova (University of Illinois)
 * SPDX-FileCopyrightText: 1993 I. Kosinovsky (USA-CERL)
 * SPDX-FileCopyrightText: 1993 D.Gerdes (USA-CERL)
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/interpf.h>

double IL_dist_square(double *pt1, double *pt2, int dim)
{
    int i;
    double sum = 0, s;

    for (i = 0; i < dim; i++) {
        s = pt1[i] - pt2[i];
        sum += s * s;
    }
    return sum;
}
