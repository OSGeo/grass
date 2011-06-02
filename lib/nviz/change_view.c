/*!
   \file lib/nviz/change_view.c

   \brief Nviz library -- Change view settings

   Based on visualization/nviz/src/change_view.c
   
   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief GL canvas resized

   \param width window width
   \param height window height

   \return 1 on success
   \return 0 on failure (window resized by dafault to 20x20 px)
 */
int Nviz_resize_window(int width, int height)
{
    int ret;

    ret = 1;

    if (width < 1 || height < 1) {
	width = 20;
	height = 20;
	ret = 0;
    }

    G_debug(1, "Nviz_resize_window(): width = %d height = %d", width, height);
    GS_set_viewport(0, width, 0, height);

    /*   GS_clear(0x0000FF); causes red flash - debug only */
    GS_set_draw(GSD_BACK);
    GS_ready_draw();
    GS_alldraw_wire();
    GS_done_draw();

    return ret;
}

/*!
   \brief Update ranges

   Call whenever a new surface is added, deleted, or exag changes

   \return 1
 */
int Nviz_update_ranges(nv_data * dc)
{
    float zmin, zmax, exag;

    GS_get_longdim(&(dc->xyrange));

    dc->zrange = 0.;

    /* Zrange is based on a minimum of Longdim */
    if (GS_global_exag()) {
	exag = GS_global_exag();
	dc->zrange = dc->xyrange / exag;
    }
    else {
	exag = 1.0;
    }

    GS_get_zrange_nz(&zmin, &zmax);	/* actual */

    zmax = zmin + (3. * dc->xyrange / exag);
    zmin = zmin - (2. * dc->xyrange / exag);

    if ((zmax - zmin) > dc->zrange)
	dc->zrange = zmax - zmin;

    return 1;
}

/*!
   \brief Change position of view

   \param data nviz data
   \param x_pos,y_pos x,y position (model coordinates)

   \return 1
 */
int Nviz_set_viewpoint_position(double x_pos, double y_pos)
{
    float xpos, ypos, from[3];
    float tempx, tempy;

    xpos = x_pos;
    xpos = (xpos < 0) ? 0 : (xpos > 1.0) ? 1.0 : xpos;
    ypos = 1.0 - y_pos;
    ypos = (ypos < 0) ? 0 : (ypos > 1.0) ? 1.0 : ypos;

    if (x_pos < 0.0 || x_pos > 1.0 || y_pos < 0.0 || y_pos > 1.0) {
	G_debug(3, "Invalid view position coordinates, using %f,%f",
		  xpos, 1.0 - ypos);
    }

    G_debug(1, "Nviz_set_viewpoint_position(): x = %f y = %f", x_pos, y_pos);
    GS_get_from(from);

    tempx = xpos * RANGE - RANGE_OFFSET;
    tempy = ypos * RANGE - RANGE_OFFSET;

    if ((from[X] != tempx) || (from[Y] != tempy)) {

	from[X] = tempx;
	from[Y] = tempy;

	GS_moveto(from);

	/* Nviz_draw_quick(data); */
    }

    return 1;
}

/*!
   \brief Change viewpoint height

   \param data nviz data
   \param height height value (world coordinates)

   \return 1
 */
int Nviz_set_viewpoint_height(double height)
{
    float from[3];

    G_debug(1, "Nviz_set_viewpoint_height(): value = %f", height);

    GS_get_from_real(from);

    if (height != from[Z]) {
	from[Z] = height;

	GS_moveto_real(from);

	/*
	   normalize (from);
	   GS_setlight_position(1, from[X], from[Y], from[Z], 0);
	 */

	/* Nviz_draw_quick(data); */
    }

    return 1;
}

/*!
   \brief Change viewpoint perspective (field of view)

   \param data nviz data
   \param persp perspective value (0-100, in degrees)

   \return 1
 */
int Nviz_set_viewpoint_persp(int persp)
{
    int fov;

    G_debug(1, "Nviz_set_viewpoint_persp(): value = %d", persp);

    fov = (int)(10 * persp);
    GS_set_fov(fov);

    /* Nviz_draw_quick(data); */

    return 1;
}

/*!
   \brief Change viewpoint twist

   \param data nviz data
   \param persp twist value (-180-180, in degrees)

   \return 1
 */
int Nviz_set_viewpoint_twist(int twist)
{
    G_debug(1, "Nviz_set_viewpoint_twist(): value = %d", twist);
    GS_set_twist(10 * twist);

    /* Nviz_draw_quick(data); */

    return 1;
}

/*!
   \brief Change z-exag value

   \param data nviz data
   \param exag exag value

   \return 1
 */
int Nviz_change_exag(nv_data * data, double exag)
{
    double temp;

    G_debug(1, "Nviz_change_exag(): value = %f", exag);
    temp = GS_global_exag();

    if (exag != temp) {
	GS_set_global_exag(exag);
	Nviz_update_ranges(data);

	/* Nviz_draw_quick(data); */
    }

    return 1;
}
/*!
  \brief Change focused point
  
  \param sx,sy screen coordinates
  
  \return 1
*/
int Nviz_look_here(double sx, double sy)
{
     G_debug(1, "Nviz_look_here(): screen coordinates = %f %f", sx, sy); 
     GS_look_here(sx, sy);
     return 1;
}


