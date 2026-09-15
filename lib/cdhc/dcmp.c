// SPDX-FileCopyrightText: 2002-2014 GRASS Development Team
// SPDX-License-Identifier: GPL-2.0-or-later
int Cdhc_dcmp(const void *i, const void *j)
{
    double x = *(double *)i;
    double y = *(double *)j;

    if (x < y)
        return -1;

    if (x > y)
        return 1;

    return 0;
}
