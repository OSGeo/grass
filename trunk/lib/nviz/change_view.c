/*!
   \file lib/nviz/change_view.c

   \brief Nviz library -- Change view settings

   Based on visualization/nviz/src/change_view.c
   
   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <math.h>

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief GL canvas resized

   \param width window width
   \param height window height

   \return 1 on success
   \return 0 on failure (window resized by default to 20x20 px)
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

   \param x_pos x position (model coordinates)
   \param y_pos y position (model coordinates)

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

void Nviz_get_viewpoint_position(double *x_pos, double *y_pos)
{
    float from[3];
    double xpos, ypos;

    GS_get_from(from);
    xpos = (from[X] + RANGE_OFFSET) / RANGE;
    ypos = (from[Y] + RANGE_OFFSET) / RANGE;
    *x_pos = xpos;
    *x_pos = (*x_pos < 0) ? 0 : (*x_pos > 1.0) ? 1.0 : *x_pos;
    *y_pos = 1.0 - ypos;
    *y_pos = (*y_pos < 0) ? 0 : (*y_pos > 1.0) ? 1.0 : *y_pos;

    if (xpos < 0.0 || xpos > 1.0 || ypos < 0.0 || ypos > 1.0) {
	G_debug(3, "Invalid view position coordinates, using %f,%f",
		  *x_pos, 1.0 - *y_pos);
    }
}

/*!
   \brief Change viewpoint height

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

void Nviz_get_viewpoint_height(double *height)
{
    float from[3];

    G_debug(1, "Nviz_get_viewpoint_height():");

    GS_get_from_real(from);

    *height = from[Z];
}
/*!
   \brief Change viewpoint perspective (field of view)

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

   \param twist persp twist value (-180-180, in degrees)

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

/*!
  \brief Get current modelview matrix
*/
void Nviz_get_modelview(double *modelMatrix)
{
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
}

/*!
  \brief Set rotation parameters

  Rotate scene by given parameters related to mouse drag event
  (difference from current state).
  Coordinates determine the second point of rotation axis,
  the first point is (0, 0, 0).

  \param angle angle
  \param x,y,z axis coordinate
*/
void Nviz_set_rotation(double angle, double x, double y, double z)
{
    G_debug(3, "Nviz_set_rotation(): angle = %f, x = %f, y = %f, z = %f", angle, x, y, z); 
    GS_set_rotation(angle, x, y, z);
}

/*!
  \brief Stop scene rotation
*/
void Nviz_unset_rotation(void)
{
    GS_unset_rotation();
}

/*!
  \brief Stop scene rotation
*/
void Nviz_init_rotation(void)
{
    GS_init_rotation();
}

/*!
  \brief Fly through the scene

  Computes parameters needed for moving scene.
  Changes viewpoint and viewdir.
  Based on visualization/nviz/src/togl_flythrough.c and simplified.

  \param data nviz data
  \param fly_info values computed from mouse movement
  \param scale rate of movement
  \param lateral type of movement
  
*/
void Nviz_flythrough(nv_data *data, float *fly_info, int *scale, int lateral)
{
    float dir[3], from[4], cur_from[4], cur_dir[4];
    float speed, h, p, sh, ch, sp, cp;
    float diff_x, diff_y, diff_z;
    float quasi_zero;

    quasi_zero = 0.0001;

    GS_get_from(cur_from);
    GS_get_viewdir(cur_dir);

    p = asin(cur_dir[Z]);
    h = atan2(- cur_dir[X], - cur_dir[Y]);

    speed = scale[0] * fly_info[0];

    h += scale[1] * fly_info[1]; /* change heading */
    if (!lateral)   /* in case of "lateral" doesn't change pitch */
        p -= scale[1] * fly_info[2];

    h = fmod(h + M_PI, 2 * M_PI) - M_PI;

    sh = sin(h);
    ch = cos(h);
    sp = sin(p);
    cp = cos(p);

    dir[X] = -sh * cp;
    dir[Y] = -ch * cp;
    dir[Z] = sp;

    if (lateral) {
        from[X] = cur_from[X] + speed * dir[Y];
        from[Y] = cur_from[Y] - speed * dir[X];
        from[Z] = cur_from[Z] + scale[0] * fly_info[2];
    }
    else {
        from[X] = cur_from[X] + speed * dir[X];
        from[Y] = cur_from[Y] + speed * dir[Y];
        /* not sure how this should behave (change Z coord or not ?)*/
        from[Z] = cur_from[Z]; /* + speed * dir[Z]*/
    }

    diff_x = fabs(cur_dir[X] - dir[X]);
    diff_y = fabs(cur_dir[Y] - dir[Y]);
    diff_z = fabs(cur_dir[Z] - dir[Z]);

    if (    /* something has changed */
        (diff_x > quasi_zero) || (diff_y > quasi_zero) ||
        (diff_z > quasi_zero) || (cur_from[X] != from[X]) ||
        (cur_from[Y] != from[Y]) || (cur_from[Z] != from[Z])
    ) {
    GS_moveto(from);
    GS_set_viewdir(dir);	/* calls gsd_set_view */
    }
}
