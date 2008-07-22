/*!
  \file draw.c
 
  \brief Nviz library -- Draw map objects to GLX context
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/draw.c and
  visualization/nviz/src/togl_flythrough.c

  \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

  \date 2008
*/

#include <grass/nviz.h>

static int sort_surfs_max(int *, int *, int *, int);

/*!
  \brief Draw all loaded surfaces

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_surf(nv_data *dc)
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

    G_free (surf_list);

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
    float tmp, max=0., tmin, tmax, tmid;

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

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_vect(nv_data *dc)
{
    // GS_set_cancel(0);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    GV_alldraw_vect();

    GS_done_draw();
    
    GS_set_draw(GSD_BACK);

    // GS_set_cancel(0);

    return 1;
}

/*!
  \brief Draw all loaded vector point sets

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_site(nv_data *dc)
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
    G_free (site_list);

    GS_done_draw();

    GS_set_draw(GSD_BACK);

    return 1;
}

/*!
  \brief Draw all loaded volume sets

  \todo To be implement

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_vol(nv_data *dc)
{
    return 1;
}

/*!
  \brief Draw all map objects (in full resolution) and decorations

  \param data nviz data
*/
int Nviz_draw_all(nv_data *data)
{
    int draw_surf, draw_vect, draw_site, draw_vol;
    int draw_north_arrow, arrow_x, draw_label, draw_legend;
    int draw_fringe, draw_scalebar, draw_bar_x;

    draw_surf = 1;
    draw_vect = 1;
    draw_site = 1;
    draw_vol = 0;
    draw_north_arrow = 0;
    arrow_x = 0;
    draw_label = 0;
    draw_legend = 0;
    draw_fringe = 0;
    draw_scalebar = 0;
    draw_bar_x = 0;

    GS_set_draw(GSD_BACK); /* needs to be BACK to avoid flickering */

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

    GS_done_draw();
    GS_set_draw(GSD_BACK);
    
    return 1;
}

/*!
  \brief Draw all surfaces in wireframe

  \param dc nviz data

  \return 1
*/
int Nviz_draw_quick(nv_data *data)
{
    GS_set_draw(GSD_BACK);
    
    GS_ready_draw();

    GS_clear(data->bgcolor);

    /* draw surfaces */
    GS_alldraw_wire();

    /* draw vector */
    /* GV_alldraw_fastvect(); is broken */
    /* GV_alldraw_vect(); */

    /*
    vol_list = GVL_get_vol_list(&max);
    max = GVL_num_vols();
    for (i = 0; i < max; i++) {
	if (check_blank(interp, vol_list[i]) == 0) {
	    GVL_draw_wire(vol_list[i]);
	}
    }
    */

    GS_done_draw();

    // flythrough_postdraw_cb();

    return 1;
}

