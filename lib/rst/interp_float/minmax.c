/*-
 * Written by H. Mitasova, L. Mitas, I. Kosinovsky, D. Gerdes Fall 1994
 * University of Illinois
 * US Army Construction Engineering Research Lab
 * SPDX-FileCopyrightText: 1994 H. Mitasova (University of Illinois)
 * SPDX-FileCopyrightText: 1994 I. Kosinovsky (USA-CERL)
 * SPDX-FileCopyrightText: 1994 D.Gerdes (USA-CERL)
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995
 *
 */

#include <stdio.h>
#include <math.h>

int min1(int arg1, int arg2)
{
    int res;

    if (arg1 <= arg2) {
        res = arg1;
    }
    else {
        res = arg2;
    }
    return res;
}

int max1(
    /*
     * L. Mitas (University of Illinois),
     * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)
     *
     * modified by McCauley in August 1995
     * modified by Mitasova in August 1995
     *
     */
    int arg1, int arg2)
{
    int res;

    if (arg1 >= arg2) {
        res = arg1;
    }
    else {
        res = arg2;
    }
    return res;
}

double amax1(double arg1, double arg2)
{
    double res;

    if (arg1 >= arg2) {
        res = arg1;
    }
    else {
        res = arg2;
    }
    return res;
}

double amin1(double arg1, double arg2)
{
    double res;

    if (arg1 <= arg2) {
        res = arg1;
    }
    else {
        res = arg2;
    }
    return res;
}
