/*!
  \file draw.cpp
 
  \brief Experimental C++ wxWidgets Nviz prototype -- Draw map objects to GLX context
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/draw.c and
  visualization/nviz/src/togl_flythrough.c

  \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

  \date 2008
*/

#include "nviz.h"

/*!
  \brief Draw map

  \param quick true for forcing coarse draw mode 
*/
void Nviz::Draw(bool quick)
{
    Nviz_draw_cplane(data, -1, -1); // ?

    if (quick) {
	Nviz_draw_quick(data); 
    }
    else {
      Nviz_draw_all (data); 
    }

    G_debug(1, "Nviz::Draw(): quick=%d",
	    quick);
    
    return;
}

/*!
  \brief Erase map display (with background color)
*/
void Nviz::EraseMap()
{
    GS_clear(data->bgcolor);

    G_debug(1, "Nviz::EraseMap()");

    return;
}
