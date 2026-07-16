/****************************************************************************
 *
 * MODULE:       PNG driver
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * SPDX-FileCopyrightText: 2007 Glynn Clements
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/

#include "psdriver.h"

void PS_Set_window(double t, double b, double l, double r)
{
    output("%.1f %.1f %.1f %.1f %s\n", t, b, l, r,
           ps.encapsulated ? "EPSWINDOW" : "WINDOW");
}

