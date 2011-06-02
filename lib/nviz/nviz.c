/*!
   \file lib/nviz/nviz.c

   \brief Nviz library -- Data management

   Based on visualization/nviz/src/
   
   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief Initialize Nviz data

   \param data nviz data
 */
void Nviz_init_data(nv_data * data)
{
    unsigned int i;

    /* data range */
    data->zrange = 0;
    data->xyrange = 0;

    /* clip planes, turn off by default */
    data->num_cplanes = 0;
    data->cur_cplane = 0;
    for (i = 0; i < MAX_CPLANES; i++) {
	Nviz_new_cplane(data, i);
	Nviz_off_cplane(data, i);
    }

    /* lights */
    for (i = 0; i < MAX_LIGHTS - 1; i++) {
	Nviz_new_light(data);
    }

    /* fringe */
    data->num_fringes = 0;
    data->fringe = NULL;
    
    return;
}

/*! \brief Free allocated space by nv_data struct

  \param data nviz data
*/
void Nviz_destroy_data(nv_data *data)
{
    int i;
    for (i = 0; data->num_fringes; i++) {
	G_free(data->fringe[i]);
	data->fringe[i] = NULL;
    }
    data->num_fringes = 0;
    data->fringe = NULL;
}

/*!
   \brief Set background color

   \param data nviz data
   \param color color value
 */
void Nviz_set_bgcolor(nv_data * data, int color)
{
    data->bgcolor = color;

    return;
}

/*!
   \brief Get background color

   \param data nviz data

   \return color color value
 */
int Nviz_get_bgcolor(nv_data * data)
{
    return data->bgcolor;
}

/*!
   \brief Get color value from color string (name or RGB triplet)

   \param color_str color string

   \return color value
 */
int Nviz_color_from_str(const char *color_str)
{
    int red, grn, blu;

    if (G_str_to_color(color_str, &red, &grn, &blu) != 1) {
	G_warning(_("Invalid color (%s), using \"white\" as default"),
		  color_str);
	red = grn = blu = 255;
    }

    return (red & RED_MASK) + ((int)((grn) << 8) & GRN_MASK) +
	((int)((blu) << 16) & BLU_MASK);
}

/*! Add new fringe

  \param data nviz data
  \param id surface id
  \param color color
  \param elev fringe elevation
  \param nw,ne,sw,se 1 (turn on) 0 (turn off)

  \return pointer to allocated fringe_data structure
  \return NULL on error
*/
struct fringe_data *Nviz_new_fringe(nv_data *data,
				    int id, unsigned long color,
				    double elev, int nw, int ne, int sw, int se)
{
    int num;
    int *surf;
    struct fringe_data *f;

    if (!GS_surf_exists(id)) {
	/* select first surface from the list */
	surf = GS_get_surf_list(&num);
	if (num < 1)
	    return NULL;
	id = surf[0];
    }
     

    f = (struct fringe_data *) G_malloc(sizeof(struct fringe_data));
    f->id = id;
    f->color = color;
    f->elev = elev;
    f->where[0] = nw;
    f->where[1] = ne;
    f->where[2] = sw;
    f->where[3] = se;

    data->fringe = (struct fringe_data **) G_realloc(data->fringe, data->num_fringes + 1 * sizeof(struct fringe_data *));
    data->fringe[data->num_fringes++] = f;
    
    return f;
}

/*! Set fringe

  \param data nviz data
  \param id surface id
  \param color color
  \param elev fringe elevation
  \param nw,ne,sw,se 1 (turn on) 0 (turn off)

  \return pointer to allocated fringe_data structure
  \return NULL on error
*/
struct fringe_data *Nviz_set_fringe(nv_data *data,
				    int id, unsigned long color,
				    double elev, int nw, int ne, int sw, int se)
{
    int i, num;
    int *surf;
    struct fringe_data *f;

    if (!GS_surf_exists(id)) {
	/* select first surface from the list */
	surf = GS_get_surf_list(&num);
	if (num < 1)
	    return NULL;
	id = surf[0];
    }
    
    for (i = 0; i < data->num_fringes; i++) {
	f = data->fringe[i];
	if (f->id == id) {
	    f->color = color;
	    f->elev  = elev;
	    f->where[0] = nw;
	    f->where[1] = ne;
	    f->where[2] = sw;
	    f->where[3] = se;
	    
	    return f;
	}
    }

    f = Nviz_new_fringe(data,
			id, color,
			elev, nw, ne, sw, se);
    
    return f;
}
