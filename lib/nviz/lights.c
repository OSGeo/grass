/*!
   \file lights.c

   \brief Nviz library -- Change view settings

   COPYRIGHT: (C) 2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   Based on visualization/nviz/src/lights.c

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
 */

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief Set light position

   \param data nviz data
   \param num light num (starts with 0)
   \param x,y,z,w position, model coordinates
 */
int Nviz_set_light_position(nv_data * data, int num,
			    float x, float y, float z, float w)
{
    data->light[num].id = num + 1;
    data->light[num].x = x;
    data->light[num].y = y;
    data->light[num].z = z;
    data->light[num].w = w;

    GS_setlight_position(num + 1, x, y, z, w);

    return 1;
}

/*!
   \brief Set light brightness

   \param data nviz data
   \param num light num (starts with 0)
   \param value brightness value
 */
int Nviz_set_light_bright(nv_data * data, int num, float value)
{
    float r, g, b;

    data->light[num].brt = value;

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    GS_setlight_color(num + 1, r, g, b);

    return 1;
}

/*!
   \brief Set light color

   \param data nviz data
   \param num light num (starts with 0)
   \param red,green,blue rGB values (0-1)
 */
int Nviz_set_light_color(nv_data * data, int num,
			 float red, float green, float blue)
{
    float r, g, b;

    data->light[num].r = red;
    data->light[num].g = green;
    data->light[num].b = blue;

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    GS_setlight_color(num + 1, r, g, b);

    return 1;
}

/*!
   \brief Set light ambient

   \param data nviz data
   \param num light num (starts with 0)
   \param red,green,blue rGB values (0-1)
 */
int Nviz_set_light_ambient(nv_data * data, int num,
			   float red, float green, float blue)
{
    data->light[num].ar = red;
    data->light[num].ag = green;
    data->light[num].ab = blue;

    GS_setlight_ambient(num + 1, red, green, blue);

    return 1;
}

/*!
   \brief Init new light

   \param data nviz data
   \param num light num (starts with 0)
 */
int Nviz_init_light(nv_data * data, int num)
{
    if (num >= MAX_LIGHTS) {
	return 0;
    }

    data->light[num].id = 0;
    data->light[num].brt = 0.8;
    data->light[num].ar = 0.3;
    data->light[num].ag = 0.3;
    data->light[num].ab = 0.3;
    data->light[num].r = 1.0;
    data->light[num].b = 1.0;
    data->light[num].g = 1.0;
    data->light[num].x = 1.0;
    data->light[num].y = 1.0;
    data->light[num].z = 1.0;
    data->light[num].w = 1.0;

    return 1;
}

int Nviz_new_light(nv_data * data)
{
    int num;

    num = GS_new_light();

    if (num < 1) {
	G_warning(_("Unable to define new light"));
	return 0;
    }

    Nviz_init_light(data, num - 1);

    return 1;
}
