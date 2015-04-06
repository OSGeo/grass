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
    GS_set_light_reset(1);
    
    for (i = 0; i < MAX_LIGHTS - 1; i++) {
	Nviz_new_light(data);
    }

    /* fringe */
    data->num_fringes = 0;
    data->fringe = NULL;

    /* north arrow */
    data->draw_arrow = 0;
    data->arrow = NULL;

    /* scale bar*/
    data->num_scalebars = 0;
    data->scalebar = NULL;

    return;
}

/*! \brief Free allocated space by nv_data struct

  \param data nviz data
*/
void Nviz_destroy_data(nv_data *data)
{
    int i;
    for (i = 0; i < data->num_fringes; i++) {
	G_free(data->fringe[i]);
	data->fringe[i] = NULL;
    }
    data->num_fringes = 0;
    data->fringe = NULL;
    
    if (data->arrow) {
	G_free(data->arrow);
	data->arrow = NULL;
	data->draw_arrow = 0;
    }

    for (i = 0; i < data->num_scalebars; i++) {
	G_free(data->scalebar[i]);
	data->scalebar[i] = NULL;
    }
    data->num_scalebars = 0;
    data->scalebar = NULL;
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
	G_free(surf);
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
	G_free(surf);
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
/*! Draw fringe

   \param data nviz data
 */
void Nviz_draw_fringe(nv_data *data)
{
    int i;

    for (i = 0; i < data->num_fringes; i++) {
	struct fringe_data *f = data->fringe[i];

	GS_draw_fringe(f->id, f->color, f->elev, f->where);
    }
}
/*!
   \brief Sets the North Arrow position and return world coords

   \param data nviz data
   \param sx,sy screen coordinates
   \param size arrow length
   \param color arrow/text color
 */
int Nviz_set_arrow(nv_data *data,
		   int sx, int sy, float size,
		   unsigned int color)
{
    int id, pt[2];
    int *surf_list, num_surfs;
    float coords[3];
    struct arrow_data *arw;

    if (GS_num_surfs() > 0) {
	surf_list = GS_get_surf_list(&num_surfs);
	id = surf_list[0];
	G_free(surf_list);

	pt[0] = sx;
	pt[1] = sy;

	GS_set_Narrow(pt, id, coords);

	if (data->arrow) {
	    data->arrow->color = color;
	    data->arrow->size  = size;
	    data->arrow->where[0]  = coords[0];
	    data->arrow->where[1]  = coords[1];
	    data->arrow->where[2]  = coords[2];
	}    
	else {
	    arw = (struct arrow_data *) G_malloc(sizeof(struct arrow_data));
	    arw->color = color;
	    arw->size  = size;
	    arw->where[0]  = coords[0];
	    arw->where[1]  = coords[1];
	    arw->where[2]  = coords[2];

	    data->arrow = arw;
	}
	return 1;
    }
    return 0;
}


/*!
   \brief Draws the North Arrow

   \param data nviz data
 */
int Nviz_draw_arrow(nv_data *data)
{

    struct arrow_data *arw = data->arrow;
    GLuint FontBase = 0; /* don't know how to get fontbase*/

    data->draw_arrow = 1;
    gsd_north_arrow(arw->where, arw->size, FontBase, arw->color, arw->color);

    return 1;
}

/*!
   \brief Deletes the North Arrow

   \param data nviz data
 */
void Nviz_delete_arrow(nv_data *data)
{
    data->draw_arrow = 0;

    return;
}

/*! Add new scalebar

  \param data nviz data
  \param bar_id scale bar id
  \param coords real(?) coordinates
  \param size scale bar length
  \param color scalebar/text color

  \return pointer to allocated scalebar_data structure
  \return NULL on error
*/

struct scalebar_data *Nviz_new_scalebar(nv_data *data,
		      int bar_id, float *coords, float size,
		      unsigned int color)
{
    struct scalebar_data *s;
     

    s = (struct scalebar_data *) G_malloc(sizeof(struct scalebar_data));
    s->id = bar_id;
    s->color = color;
    s->size = size;
    s->where[0] = coords[0];
    s->where[1] = coords[1];
    s->where[2] = coords[2];

    data->scalebar = (struct scalebar_data **) G_realloc(data->scalebar,
		      (data->num_scalebars + 1) * sizeof(struct scalebar_data *));
    data->scalebar[data->num_scalebars++] = s;

    return s;

}
/*!
   \brief Sets the scale bar position and return world coords

   \param data nviz data
   \param bar_id scale bar id
   \param sx,sy screen coordinates
   \param size scale bar length
   \param color scalebar/text color

   \return pointer to allocated scalebar_data structure
   \return NULL when there's no surface
 */
struct scalebar_data *Nviz_set_scalebar(nv_data *data, int bar_id,
		      int sx, int sy, float size,
		      unsigned int color)
{
    int i, id, pt[2];
    int *surf_list, num_surfs;
    float coords[3];
    struct scalebar_data *s;

    if (GS_num_surfs() > 0) {
	surf_list = GS_get_surf_list(&num_surfs);
	id = surf_list[0];
	G_free(surf_list);

	pt[0] = sx;
	pt[1] = sy;

	GS_set_Narrow(pt, id, coords); /* the same like arrow */

	for (i = 0; i < data->num_scalebars; i++) {
        if (data->scalebar[i]) {
            s = data->scalebar[i];
            if (s->id == bar_id) {
                s->color = color;
                s->size = size;
                s->where[0] = coords[0];
                s->where[1] = coords[1];
                s->where[2] = coords[2];

            return s;
            }
        }
	}
	
	s = Nviz_new_scalebar(data, bar_id, coords, size, color);

	return s;
    }
    return NULL;
}
/*!
   \brief Draws the Scale bar

   \param data nviz data
 */
void Nviz_draw_scalebar(nv_data *data)
{
    int i;

    GLuint FontBase = 0; /* don't know how to get fontbase*/

    for (i = 0; i < data->num_scalebars; i++) {
        if (data->scalebar[i]) {
            struct scalebar_data *s = data->scalebar[i];

            gsd_scalebar_v2(s->where, s->size, FontBase, s->color, s->color);
        }
    }
}

/*!
   \brief Deletes scale bar
   
   When scalebar is freed, array then contains NULL,
   which must be tested during drawing.

   \param data nviz data
 */
void Nviz_delete_scalebar(nv_data *data, int bar_id)
{
    if (bar_id < data->num_scalebars && data->scalebar[bar_id] != NULL) {
	G_free(data->scalebar[bar_id]);
	data->scalebar[bar_id] = NULL;
    }
}
