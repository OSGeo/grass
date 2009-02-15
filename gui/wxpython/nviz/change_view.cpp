/**
   \file nviz/change_view.cpp
   
   \brief wxNviz extension (3D view mode) - change view settings
   
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
*/

#include "nviz.h"

/*!
  \brief GL canvas resized

  \param width window width
  \param height window height

  \return 1 on success
  \return 0 on failure (window resized by dafault to 20x20 px)
 */
int Nviz::ResizeWindow(int width, int height)
{
    int ret;

    ret = Nviz_resize_window(width, height);

    G_debug(1, "Nviz::ResizeWindow(): width=%d height=%d",
	    width, height);

    return ret;
}

/*!
  \brief Set default view (based on loaded data)

  \return z-exag value, default, min and max height
*/
std::vector<double> Nviz::SetViewDefault()
{
    std::vector<double> ret;

    float hdef, hmin, hmax, z_exag;

    /* determine z-exag */
    z_exag = Nviz_get_exag();
    ret.push_back(z_exag);
    Nviz_change_exag(data,
		     z_exag);

    /* determine height */
    Nviz_get_exag_height(&hdef, &hmin, &hmax);
    ret.push_back(hdef);
    ret.push_back(hmin);
    ret.push_back(hmax);

    G_debug(1, "Nviz::SetViewDefault(): hdef=%f, hmin=%f, hmax=%f",
	    hdef, hmin, hmax);

    return ret;
}

/*!
  \brief Change view settings

  \param x,y position
  \param height
  \param persp perpective
  \param twist

  \return 1 on success
*/
int Nviz::SetView(float x, float y,
		  float height, float persp, float twist)
{
    Nviz_set_viewpoint_height(data,
			      height);
    Nviz_set_viewpoint_position(data,
				x, y);
    Nviz_set_viewpoint_twist(data,
			     twist);
    Nviz_set_viewpoint_persp(data,
			     persp);

    G_debug(1, "Nviz::SetView(): x=%f, y=%f, height=%f, persp=%f, twist=%f",
	    x, y, height, persp, twist);
	
    return 1;
}

/*!
  \brief Set z-exag value

  \param z_exag value

  \return 1
*/
int Nviz::SetZExag(float z_exag)
{
    int ret;
    
    ret = Nviz_change_exag(data, z_exag);

    G_debug(1, "Nviz::SetZExag(): z_exag=%f", z_exag);

    return ret;
}
