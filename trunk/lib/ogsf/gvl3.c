/*!
   \file lib/ogsf/gvl3.c

   \brief OGSF library - loading volumes (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Tomas Paudits (December 2003)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/ogsf.h>
#include <grass/glocale.h>

/*!
   \brief Load color table

   \param[out] color_data color data buffer
   \param name 3D raster map name

   \return -1 on failure
   \return 1 on success
 */
int Gvl_load_colors_data(void **color_data, const char *name)
{
    const char *mapset;
    struct Colors *colors;

    if (NULL == (mapset = G_find_raster3d(name, ""))) {
	G_warning(_("3D raster map <%s> not found"), name);
	return (-1);
    }

    if (NULL == (colors = (struct Colors *)G_malloc(sizeof(struct Colors))))
	return (-1);

    if (0 > Rast3d_read_colors(name, mapset, colors)) {
	G_free(colors);
	return (-1);
    }

    *color_data = colors;

    return (1);
}

/*!
   \brief Unload color table

   \param color_data color data buffer

   \return -1 on failure
   \return 1 on success
 */
int Gvl_unload_colors_data(void *color_data)
{
    Rast_free_colors(color_data);

    G_free(color_data);

    return (1);
}

/*!
   \brief Get color for value

   \param color_data color data value
   \param value data value

   \return color value
 */
int Gvl_get_color_for_value(void *color_data, float *value)
{
    int r, g, b;

    Rast_get_f_color((FCELL *) value, &r, &g, &b, color_data);
    return ((r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16));
}
