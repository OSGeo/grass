/*!
   \file lib/nviz/cplanes_obj.c
   
   \brief Nviz library -- Clip planes manipulation
   
   Based on visualization/nviz/src/cutplanes_obj.c

   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <grass/nviz.h>

static void cp_draw(nv_data *, int, int, int);
static geoview Gv;

/*!
   \brief Creates a clip plane object

   The number of clip planes is fixed (MAX_CPLANES) and
   we'll create them all ahead of time anyway we just let
   the user decide on the id for each.

   \param data nviz data
   \param id
 */
int Nviz_new_cplane(nv_data * data, int id)
{
    data->num_cplanes++;
    /* Initialize internal attributes for this cutplane */
    data->cp_rot[id][X] = data->cp_rot[id][Y] = data->cp_rot[id][Z] = 0.0;
    data->cp_trans[id][X] = data->cp_trans[id][Y] = data->cp_trans[id][Z] =
	0.0;
    data->cp_on[id] = 0;

    return 1;
}

/*!
   \brief Turn on (make current) the given clip plane.

   \param data nviz data
   \param cplane id
 */
int Nviz_on_cplane(nv_data * data, int id)
{
    data->cur_cplane = id;
    data->cp_on[id] = 1;
    GS_set_cplane(id);

    return 1;
}

/*!
   \brief Turn off (make inactive) the given clip plane

   \param data nviz data
   \param cplane id
 */
int Nviz_off_cplane(nv_data * data, int id)
{
    data->cp_on[id] = 0;
    GS_unset_cplane(id);

    return 1;
}

/*!
   \brief Draw the clip plane

   \param data nviz data
   \param bound1
   \param bound2
 */
int Nviz_draw_cplane(nv_data * data, int bound1, int bound2)
{
    cp_draw(data, data->cur_cplane, bound1, bound2);

    return 1;
}

/*!
   \brief Draw current clip plane

   \param data nviz data
   \param current id of current clip plane
   \param surf1 first surface id
   \param surf2 second surface id
 */
void cp_draw(nv_data * data, int current, int surf1, int surf2)
{
    int i, nsurfs;
    int surf_min = 0, surf_max = 0, temp;
    int *surf_list;

    GS_set_draw(GSD_BACK);
    GS_clear(data->bgcolor);
    GS_ready_draw();

    /* If surf boundaries present then find them */
    surf_list = GS_get_surf_list(&nsurfs);
    if ((surf1 != -1) && (surf2 != -1)) {
	for (i = 0; i < nsurfs; i++) {
	    if (surf_list[i] == surf1)
		surf_min = i;
	    if (surf_list[i] == surf2)
		surf_max = i;
	}

	if (surf_max < surf_min) {
	    temp = surf_min;
	    surf_min = surf_max;
	    surf_max = temp;
	}

	surf_max++;
    }
    else {
	surf_min = 0;
	surf_max = nsurfs;
    }

    if (nsurfs > 1) {
	for (i = 0; i < MAX_CPLANES; i++) {
	    if (data->cp_on[i])
		GS_draw_cplane_fence(surf_list[0], surf_list[1], i);
	}
    }

    for (i = surf_min; i < surf_max; i++) {
	GS_draw_wire(surf_list[i]);
    }

    GS_done_draw();

    return;
}
/*!
   \brief Return the number of clip planes objects currently allocated.
   
   \param data nviz data
 */
int Nviz_num_cplanes(nv_data * data)
{
	return data->num_cplanes;
}

/*!
   \brief Get the current active cutplane.
   
   \param data nviz data
 */
int Nviz_get_current_cplane(nv_data * data)
{
	return data->cur_cplane;
}

/*!
   \brief Set the rotation for the current clip plane.
   
   \param data nviz data
   \param id id of current clip plane
   \param dx,dy,dz rotation parameters
   
   \return 1 

 */
int Nviz_set_cplane_rotation(nv_data * data, int id, float dx, float dy, float dz)
{
    data->cp_rot[id][X] = dx;
    data->cp_rot[id][Y] = dy;
    data->cp_rot[id][Z] = dz;
    GS_set_cplane_rot(id, data->cp_rot[id][X], data->cp_rot[id][Y],
		      data->cp_rot[id][Z]);

    cp_draw(data, data->cur_cplane, -1, -1);
    
    return 1;
}
/*!
   \brief Get the rotation values for the current clip plane.
   
   \param data nviz data
   \param id id of current clip plane
   \param dx,dy,dz rotation parameters
   
   \return 1
   
 */
int Nviz_get_cplane_rotation(nv_data * data, int id, float *dx, float *dy, float *dz)
{
    *dx = data->cp_rot[id][X];
    *dy = data->cp_rot[id][Y];
    *dz = data->cp_rot[id][Z];

    return 1;
}

/*!
   \brief Set the translation for the current clip plane.

   \param data nviz data
   \param id id of current clip plane
   \param dx,dy,dz values for setting translation
   
   \return 1
 */
int Nviz_set_cplane_translation(nv_data * data, int id, float dx, float dy, float dz)
{
    data->cp_trans[id][X] = dx;
    data->cp_trans[id][Y] = dy;
    data->cp_trans[id][Z] = dz;
    GS_set_cplane_trans(id, data->cp_trans[id][X], data->cp_trans[id][Y],
			data->cp_trans[id][Z]);

    cp_draw(data, data->cur_cplane, -1, -1);
    
    return 1;
}
/*!
   \brief Get the translation values for the current clip plane.
   
   \param data nviz data
   \param id id of current clip plane
   \param dx,dy,dz translation parameters
 */
int Nviz_get_cplane_translation(nv_data * data, int id, float *dx, float *dy, float *dz)
{
    *dx = data->cp_trans[id][X];
    *dy = data->cp_trans[id][Y];
    *dz = data->cp_trans[id][Z];

    return 1;
}
/*!
   \brief Set appropriate fence color
   
   \param type type of fence (FC_ABOVE, FC_BELOW, FC_BLEND, FC_GREY, FC_OFF)
 */
int Nviz_set_fence_color(nv_data * data, int type)
{
	GS_set_fencecolor(type);

    return 1;

}
int Nviz_set_cplane_here(nv_data *data, int cplane, float sx, float sy)
{
    float x, y, z, len, los[2][3];
    float dx, dy, dz;
    float n, s, w, e;
    Point3 realto, dir;
    int id;
    geosurf *gs;

    if (GS_get_selected_point_on_surface(sx, sy, &id, &x, &y, &z)) {
	gs = gs_get_surf(id);
	if (gs) {
	    realto[X] = x - gs->ox + gs->x_trans;
	    realto[Y] = y - gs->oy + gs->y_trans;
	    realto[Z] = z + gs->z_trans;
	}
	else
	    return 0;
    }
    else {
	if (gsd_get_los(los, (short)sx, (short)sy)) {
	    len = GS_distance(Gv.from_to[FROM], Gv.real_to);
	    GS_v3dir(los[FROM], los[TO], dir);
	    GS_v3mult(dir, len);
	    realto[X] = Gv.from_to[FROM][X] + dir[X];
	    realto[Y] = Gv.from_to[FROM][Y] + dir[Y];
	    realto[Z] = Gv.from_to[FROM][Z] + dir[Z];
	}
	else
	    return 0;
    }  
    Nviz_get_cplane_translation(data, cplane, &dx, &dy, &dz);

    GS_get_region(&n, &s, &w, &e);
    dx = realto[X] - (e - w) / 2.;
    dy = realto[Y] - (n - s) / 2.;

    Nviz_set_cplane_translation(data, cplane, dx, dy, dz);

    return 1;
}
