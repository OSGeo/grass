/**
   \file nviz/draw.cpp
   
   \brief wxNviz extension (3D view mode) - draw map objects to GLX context
   
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
   
   (C) 2008-2009 by Martin Landa, and the GRASS development team
   
   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
*/

#include "nviz.h"

/*!
  \brief Draw map

  Draw quick mode:
   - DRAW_QUICK_SURFACE
   - DRAW_QUICK_VLINES
   - DRAW_QUICK_VPOINTS
   - DRAW_QUICK_VOLUME
   
  \param quick if true draw in wiremode
  \param quick_mode quick mode
*/
void Nviz::Draw(bool quick, int quick_mode)
{
    Nviz_draw_cplane(data, -1, -1); // ?

    if (quick) {
	Nviz_draw_quick(data, quick_mode);
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
