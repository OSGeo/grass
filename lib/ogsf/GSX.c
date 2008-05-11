/*!
  \file GSX.c
 
  \brief OGSF library - loading and manipulating surfaces
 
  GRASS OpenGL gsurf OGSF Library 
 
  (C) 1999-2008 by the GRASS Development Team
 
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Bill Brown USACERL (December 1993)
*/

#include <grass/gstypes.h>

void (*Cxl_func) ();
void (*Swap_func) ();

static int Cxl = 0;

int GS_check_cancel(void)
{
    Cxl_func();

    return (Cxl);
}

void GS_set_cancel(int c)
{
    Cxl = c;

    return;
}

void GS_set_cxl_func(void (*f) (void))
{
    Cxl_func = f;

    return;
}


void GS_set_swap_func(void (*f) (void))
{
    Swap_func = f;

    return;
}
