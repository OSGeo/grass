/*
 * SPDX-FileCopyrightText: 1995 Bill Brown <brown@gis.uiuc.edu>
 * SPDX-FileCopyrightText: 1995 Michael Shapiro
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
int datetime_is_between(int x, int a, int b)
{
    if (a <= b)
        return a <= x && x <= b;
    else
        return b <= x && x <= a;
}
