/*
 * SPDX-FileCopyrightText: 1994 James Darrell McCauley
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdio.h>
#include <grass/gis.h>
#include "zufall.h"

int myrng(double *numbers, int n, int (*rng)(int, double *), double p1,
          double p2)
{
    int i;

    rng(n, numbers);

    if (rng == zufall) /* uniform */
        for (i = 0; i < n; ++i)
            numbers[i] -= 0.5, numbers[i] *= 2 * p1;
    else if (rng == normalen) /* gaussian */
        /* is this how to do transformation? */
        for (i = 0; i < n; ++i)
            numbers[i] *= p2, numbers[i] += p1;

    return 0;
}
