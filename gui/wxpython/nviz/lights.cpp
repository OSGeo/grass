/**
   \file nviz/lights.cpp
   
   \brief wxNviz extension (3D view mode) - lights manipulation
   
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
   
   (C) 2008-2009 by Martin Landa, and the GRASS development team
   
   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
*/

#include "nviz.h"

/*!
  \brief Set default lighting model
*/
void Nviz::SetLightsDefault()
{
    /* first */
    Nviz_set_light_position(data, 0,
			    0.68, -0.68, 0.80, 0.0);
    Nviz_set_light_bright(data, 0,
			  0.8);
    Nviz_set_light_color(data, 0,
			 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(data, 0,
			   0.2, 0.2, 0.2);

    /* second */
    Nviz_set_light_position(data, 1,
			    0.0, 0.0, 1.0, 0.0);
    Nviz_set_light_bright(data, 1,
			  0.5);
    Nviz_set_light_color(data, 1,
			 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(data, 1,
			   0.3, 0.3, 0.3);

    G_debug(1, "Nviz::SetLightsDefault()");

    return;
}
