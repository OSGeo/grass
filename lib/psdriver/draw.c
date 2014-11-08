/*!
  \file lib/psdriver/draw.c

  \brief GRASS PS display driver

  (C) 2007-2008 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements  
*/

#include "psdriver.h"

void PS_Begin(void)
{
    output("NEW\n");
}

void PS_Move(double x, double y)
{
    output("%f %f MOVE\n", x, y);
}

void PS_Cont(double x, double y)
{
    output("%f %f CONT\n", x, y);
}

void PS_Close(void)
{
    output("CLOSE\n");
}

void PS_Stroke(void)
{
    output("STROKE\n");
}

void PS_Fill(void)
{
    output("FILL\n");
}

void PS_Point(double x, double y)
{
    output("%f %f POINT\n", x, y);
}

