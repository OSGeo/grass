/*
 * SPDX-FileCopyrightText: 1994-1995 James Darrell McCauley
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <grass/gis.h>
#include <grass/vector.h>
#include "quaddefs.h"

void qindices(int *cnt, int n, double *fisher, double *david, double *douglas,
              double *lloyd, double *lloydip, double *morisita)
/*
 * Calculates quadrat count indices for measuring departure from complete
 * spatial randomness. See Cressie (1991).
 */
{
    int i;
    double m = 0, s = 0;

    *morisita = 0.0;
    for (i = 0; i < n; ++i) {
        m += cnt[i];
        *morisita += (double)cnt[i] * (cnt[i] - 1.0);
    }
    m /= n;

    for (i = 0; i < n; ++i)
        s += (cnt[i] - m) * (cnt[i] - m);
    s /= (n - 1);

    *fisher = s / m;
    *david = *fisher - 1;
    *douglas = m * m / (s - m);
    *lloyd = m + s / m - 1;
    *lloydip = *lloyd / m;
    *morisita *= n;
    *morisita /= (n * m * (n * m - 1.0)); /* cressie */
    /* *morisita = *lloydip * n * m / (n * m - 1.0);  ripley 1 */
}
