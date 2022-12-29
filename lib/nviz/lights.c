/*!
   \file lib/nviz/lights.c

   \brief Nviz library -- Change lighting settings

   Based on visualization/nviz/src/lights.c

   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief Set light position

   \param data nviz data
   \param num light num (starts at 1)
   \param x,y,z,w position, model coordinates

   \return 1
 */
int Nviz_set_light_position(nv_data * data, int num,
			    double x, double y, double z, double w)
{
    /*
    double xpos, ypos;
    xpos = x;
    xpos = (xpos < 0) ? 0 : (xpos > 1.0) ? 1.0 : xpos;
    ypos = 1.0 - y;
    ypos = (ypos < 0) ? 0 : (ypos > 1.0) ? 1.0 : ypos;
    
    if (x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0) {
	G_debug(1, "Invalid light position coordinates (%f,%f), using %f,%f",
		x, y, xpos, 1.0 - ypos);
    }
    */

    data->light[num].id = num;
    data->light[num].x = x;
    data->light[num].y = y;
    data->light[num].z = z;
    data->light[num].w = w;

    G_debug(1, "Nviz_set_light_position(): num = %d x = %f y = %f z = %f w = %f",
	    num, x, y, z, w);
    GS_setlight_position(num, x, y, z, w);

    return 1;
}

/*!
   \brief Set light brightness

   \param data nviz data
   \param num light num (starts at 1)
   \param value brightness value
 */
int Nviz_set_light_bright(nv_data * data, int num, double value)
{
    double r, g, b;

    data->light[num].brt = value;

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    G_debug(1, "Nviz_set_light_bright(): num = %d value = %f r = %f g = %f b = %f",
	    num, value, r, g, b);
    GS_setlight_color(num, r, g, b);

    return 1;
}

/*!
   \brief Set light color

   \param data nviz data
   \param num light num (starts at 1)
   \param red,green,blue RGB values (0-255)
 */
int Nviz_set_light_color(nv_data * data, int num,
			 int red, int green, int blue)
{
    double r, g, b;

    data->light[num].r = red / 255.;
    data->light[num].g = green / 255.;
    data->light[num].b = blue / 255.;

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    G_debug(1, "Nviz_set_light_color(): num = %d r = %d/%f g = %d/%f b = %d/%f",
	    num, red, r, green, g, blue, b);
    GS_setlight_color(num, r, g, b);

    return 1;
}

/*!
   \brief Set light ambient

   \param data nviz data
   \param num light num (starts at 1)
   \param value ambient value (same for R/G/B) (0-1)
 */
int Nviz_set_light_ambient(nv_data * data, int num, double value)
{
    data->light[num].ar = value;
    data->light[num].ag = value;
    data->light[num].ab = value;

    G_debug(1, "Nviz_set_light_ambient(): num = %d value = %f",
	    num, value);
    GS_setlight_ambient(num, value, value, value);
    
    return 1;
}

/*!
   \brief Init new light

   \param data nviz data
   \param num light num (starts at 1)
 */
int Nviz_init_light(nv_data * data, int num)
{
	G_debug(1, "Nviz_init_light(): num = %d", num);
    if (num > MAX_LIGHTS) {
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

/*!
  \brief Define new light

  \param data nviz data

  \return 1 on success
  \return 0 on failure
*/
int Nviz_new_light(nv_data * data)
{
    int num;

    num = GS_new_light();

    if (num < 1) {
	G_warning(_("Unable to define new light"));
	return 0;
    }

    Nviz_init_light(data, num);

    return 1;
}

/*!
  \brief Draw lighting model

  \param data nviz data
*/
void Nviz_draw_model(nv_data * data)
{
    GS_set_draw(GSD_FRONT);
    GS_ready_draw();
    GS_draw_lighting_model();
    GS_done_draw();
    GS_set_draw(GSD_BACK);
}
