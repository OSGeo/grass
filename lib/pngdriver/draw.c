/*!
   \file lib/pngdriver/draw.c

   \brief GRASS PNG display driver

   SPDX-FileCopyrightText: 2008 Glynn Clements
   SPDX-FileCopyrightText: Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Glynn Clements
 */

#include <grass/gis.h>
#include "driverlib.h"
#include "path.h"
#include "pngdriver.h"

static struct path path;

void PNG_Begin(void)
{
    path_begin(&path);
}

void PNG_Move(double x, double y)
{
    path_move(&path, x, y);
}

void PNG_Cont(double x, double y)
{
    path_cont(&path, x, y);
}

void PNG_Close(void)
{
    path_close(&path);
}

void PNG_Stroke(void)
{
    path_stroke(&path, png_draw_line);
}

void PNG_Fill(void)
{
    png_polygon(&path);
}
