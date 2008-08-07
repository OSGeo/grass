/*!
   \file volume.c

   \brief Volume subroutines

   (C) 2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
 */

#include <grass/G3d.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Load 3d raster map layers -> volume

   \param params module parameters
   \param data nviz data

   \return number of loaded volumes
 */
int load_volume(const struct GParams *params, nv_data *data)
{
    int i, nvol;
    char *mapset;
    
    nvol = opt_get_num_answers(params->volume);

    for (i = 0; i < nvol; i++) {
	mapset = G_find_grid3(params->volume->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("3d raster map <%s> not found"),
			  params->volume->answers[i]);
	}

	Nviz_new_map_obj(MAP_OBJ_VOL,
			 G_fully_qualified_name(params->volume->answers[i],
						mapset),
			 0.0, data);
    }

    return 1;
}
