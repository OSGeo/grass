/*!
   \file lib/nviz/draw.c

   \brief Nviz library -- Draw map objects to GLX context

   Based on visualization/nviz/src/draw.c and
   visualization/nviz/src/togl_flythrough.c
   
   (C) 2008, 2010-2011 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
   \author Textures by Anna Kratochvilova 
 */

#include <grass/nviz.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif 

static int sort_surfs_max(int *, int *, int *, int);

/*!
   \brief Draw all loaded surfaces

   \param dc nviz data

   \return 1
 */
int Nviz_draw_all_surf(nv_data * dc)
{
    int i, nsurfs;
    int sortSurfs[MAX_SURFS], sorti[MAX_SURFS];
    int *surf_list;
    float x, y, z;
    int num, w;

    /* Get position for Light 1 */
    num = 1;
    x = dc->light[num].x;
    y = dc->light[num].y;
    z = dc->light[num].z;
    w = dc->light[num].z;

    surf_list = GS_get_surf_list(&nsurfs);

    sort_surfs_max(surf_list, sortSurfs, sorti, nsurfs);

    G_free(surf_list);

    /* re-initialize lights */
    GS_setlight_position(num, x, y, z, w);
    num = 2;
    GS_setlight_position(num, 0., 0., 1., 0);

    for (i = 0; i < nsurfs; i++) {
	GS_draw_surf(sortSurfs[i]);
    }

    /* GS_draw_cplane_fence params will change - surfs aren't used anymore */
    for (i = 0; i < MAX_CPLANES; i++) {
	if (dc->cp_on[i])
	    GS_draw_cplane_fence(sortSurfs[0], sortSurfs[1], i);
    }

    return 1;
}

/*!
   \brief Sorts surfaces by max elevation, lowest to highest.

   Puts ordered id numbers in id_sort, leaving id_orig unchanged.
   Puts ordered indices of surfaces from id_orig in indices.

   \param surf pointer to surface array
   \param id_sort
   \param indices
   \param num

   \return 1
 */
int sort_surfs_max(int *surf, int *id_sort, int *indices, int num)
{
    int i, j;
    float maxvals[MAX_SURFS];
    float tmp, max = 0., tmin, tmax, tmid;

    for (i = 0; i < num; i++) {
	GS_get_zextents(surf[i], &tmin, &tmax, &tmid);
	if (i == 0)
	    max = tmax;
	else
	    max = max < tmax ? tmax : max;
	maxvals[i] = tmax;
    }

    for (i = 0; i < num; i++) {
	tmp = maxvals[0];
	indices[i] = 0;
	for (j = 0; j < num; j++) {
	    if (maxvals[j] < tmp) {
		tmp = maxvals[j];
		indices[i] = j;
	    }
	}

	maxvals[indices[i]] = max + 1;
	id_sort[i] = surf[indices[i]];
    }

    return 1;
}

/*!
   \brief Draw all loaded vector sets (lines)

   \return 1
 */
int Nviz_draw_all_vect()
{
    /* GS_set_cancel(0); */

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    GV_alldraw_vect();

    GS_done_draw();

    GS_set_draw(GSD_BACK);

    /* GS_set_cancel(0); */

    return 1;
}

/*!
   \brief Draw all loaded vector point sets

   \return 1
 */
int Nviz_draw_all_site()
{
    int i;
    int *site_list, nsites;

    site_list = GP_get_site_list(&nsites);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    for (i = 0; i < nsites; i++) {
	GP_draw_site(site_list[i]);
    }
    G_free(site_list);

    GS_done_draw();

    GS_set_draw(GSD_BACK);

    return 1;
}

/*!
   \brief Draw all loaded volume sets

   \return 1
 */
int Nviz_draw_all_vol()
{
    int *vol_list, nvols, i;

    vol_list = GVL_get_vol_list(&nvols);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    for (i = 0; i < nvols; i++) {
	GVL_draw_vol(vol_list[i]);
    }

    G_free(vol_list);

    GS_done_draw();

    GS_set_draw(GSD_BACK);

    return 1;
}

/*!
   \brief Draw all map objects (in full resolution) and decorations

   \param data nviz data
 */
int Nviz_draw_all(nv_data * data)
{
    int i;
    int draw_surf, draw_vect, draw_site, draw_vol;
    
    draw_surf = 1;
    draw_vect = 1;
    draw_site = 1;
    draw_vol = 1;
    /*
    draw_north_arrow = 0;
    arrow_x = 0;
    draw_label = 0;
    draw_legend = 0;
    draw_fringe = 0;
    draw_scalebar = 0;
    draw_bar_x = 0;
    */
    
    GS_set_draw(GSD_BACK);	/* needs to be BACK to avoid flickering */

    GS_ready_draw();

    GS_clear(data->bgcolor);

    if (draw_surf)
	Nviz_draw_all_surf(data);

    if (draw_vect)
	Nviz_draw_all_vect(data);

    if (draw_site)
	Nviz_draw_all_site(data);

    if (draw_vol)
	Nviz_draw_all_vol(data);
	
    for(i = 0; i < data->num_fringes; i++) {
	struct fringe_data * f = data->fringe[i];
	GS_draw_fringe(f->id, f->color, f->elev, f->where);
    }

    /* North Arrow */
    if (data->draw_arrow) {
	gsd_north_arrow(data->arrow->where, data->arrow->size,
			(GLuint)NULL, data->arrow->color, data->arrow->color);
    }

    /* scale bar */
    for (i = 0; i < data->num_scalebars; i++) {
        if (data->scalebar[i]){
            struct scalebar_data *s = data->scalebar[i];
            gsd_scalebar_v2(s->where, s->size, 0, s->color, s->color);
        }
    }
    GS_done_draw();
    GS_set_draw(GSD_BACK);

    return 1;
}

/*!
   \brief Draw all surfaces in wireframe (quick mode)

   Draw modes:
    - DRAW_QUICK_SURFACE
    - DRAW_QUICK_VLINES
    - DRAW_QUICK_VPOINTS
    - DRAW_QUICK_VOLUME

   \param data nviz data
   \param draw_mode draw mode
   
   \return 1
 */
int Nviz_draw_quick(nv_data * data, int draw_mode)
{
    GS_set_draw(GSD_BACK);
    
    GS_ready_draw();
    
    GS_clear(data->bgcolor);
    
    /* draw surfaces */
    if (draw_mode & DRAW_QUICK_SURFACE)
	GS_alldraw_wire();
    
    /* draw vector lines */
    if (draw_mode & DRAW_QUICK_VLINES)
	GV_alldraw_vect();
    
    /* draw vector points */
    if (draw_mode & DRAW_QUICK_VPOINTS)
	GP_alldraw_site();
    
    /* draw volumes */
    if (draw_mode & DRAW_QUICK_VOLUME) {
	GVL_alldraw_wire();
    }
    
    GS_done_draw();
    
    return 1;
}

/*!
  \brief Load image into texture

  \param image_data image data 
  \param width, height image screen size 
  \param alpha has alpha channel 
*/
int Nviz_load_image(GLubyte *image_data, int width, int height, int alpha)
{
    unsigned int texture_id;
    int  in_format;
    GLenum format;

    if (alpha)
    {
	in_format = 4;
	format = GL_RGBA;
    }
    else
    {
	in_format = 3;
	format = GL_RGB;
    }
    glGenTextures( 1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, in_format, width, height, 0,format,
		 GL_UNSIGNED_BYTE, image_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 

    return texture_id;
}

/*!
  \brief Set ortho view for drawing images

  \param width, height image screen size 
*/
void Nviz_set_2D(int width, int height)
{
    glEnable(GL_BLEND); /* images are transparent */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    
    /* set coordinate system from upper left corner */
    glScalef(1, -1, 1);
    glTranslatef(0, -height, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/*!
  \brief Draw image as texture

  \param x, y image coordinates 
  \param width, height image size 
  \param texture_id texture id 
*/
void Nviz_draw_image(int x, int y, int width, int height, int texture_id)
{
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GS_set_draw(GSD_FRONT);

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);

    glTexCoord2d(0.0,1.0);
    glVertex2d(x, y);
    glTexCoord2d(0.0,0.0);
    glVertex2d(x, y + height);
    glTexCoord2d(1.0,0.0);
    glVertex2d(x + width, y + height);
    glTexCoord2d(1.0,1.0);
    glVertex2d(x + width, y);

    glEnd();

    GS_done_draw();
    glDisable(GL_TEXTURE_2D);
}

/*!
  \brief Delete texture

  \param texture_id texture id
*/
void Nviz_del_texture(int texture_id)
{
    GLuint t[1];

    t[0] = texture_id;
    glDeleteTextures(1, t);
}

/*!
  \brief Get maximum texture size

*/
void Nviz_get_max_texture(int *size)
{
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, size);
}
