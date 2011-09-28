/*!
   \file cplane.c

   \brief Cutting plane subroutine

   (C) 2011 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Anna Kratochvilova <kratochanna gmail.com> (Google SoC 2010/2011)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Draw cutting planes and set their attributes

   \param params module parameters
   \param data nviz data
 */
void draw_cplane(const struct GParams *params, nv_data * data)
{
    int i, id, ncplanes;
    float trans_x, trans_y, trans_z;
    float rot_x, rot_y, rot_z;
    int fence;

    ncplanes = opt_get_num_answers(params->cplane);
    for (i = 0; i < ncplanes; i++) {
	id = atoi(params->cplane->answers[i]);

	if (id < 0 || id > Nviz_num_cplanes(data))
	    G_fatal_error(_("Cutting plane number <%d> not found"), id);

	Nviz_on_cplane(data, id);

	trans_x = atof(params->cplane_pos->answers[i * 3 + 0]);
	trans_y = atof(params->cplane_pos->answers[i * 3 + 1]);
	trans_z = atof(params->cplane_pos->answers[i * 3 + 2]);
	Nviz_set_cplane_translation(data, id, trans_x, trans_y, trans_z);

	rot_x = 0;
	rot_y = atof(params->cplane_tilt->answers[i]);
	rot_z = atof(params->cplane_rot->answers[i]);
	Nviz_set_cplane_rotation(data, id, rot_x, rot_y, rot_z);
    }

    const char *shading = params->cplane_shading->answers[0];

    if (strcmp(shading, "clear") == 0)
	fence = 0;
    else if (strcmp(shading, "top") == 0)
	fence = 1;
    else if (strcmp(shading, "bottom") == 0)
	fence = 2;
    else if (strcmp(shading, "blend") == 0)
	fence = 3;
    else if (strcmp(shading, "shaded") == 0)
	fence = 4;
    else
	fence = 0;
    Nviz_set_fence_color(data, fence);

    return;
}
