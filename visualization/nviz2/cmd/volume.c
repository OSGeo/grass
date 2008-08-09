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

#include <stdlib.h>

#include <grass/G3d.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Load 3d raster map layers -> volume

   \param params module parameters
   \param data nviz data

   \return number of loaded volumes
 */
int load_rasters3d(const struct GParams *params, nv_data *data)
{
  int i, nvol, id;
    char *mapset;
    
    nvol = opt_get_num_answers(params->volume);

    for (i = 0; i < nvol; i++) {
	mapset = G_find_grid3(params->volume->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("3d raster map <%s> not found"),
			  params->volume->answers[i]);
	}

	id = Nviz_new_map_obj(MAP_OBJ_VOL,
			      G_fully_qualified_name(params->volume->answers[i],
						     mapset),
			      0.0, data);
    }

    return 1;
}

/*!
  \brief Add isosurfaces and set their attributes

  \param params module parameters
  \param data nviz data
  
  \return number of defined isosurfaces
*/
int add_isosurfs(const struct GParams *params, nv_data *data)
{
    int i;
    int num, level, nvols, *vol_list, id, nisosurfs;
    char **tokens;
    
    vol_list = GVL_get_vol_list(&nvols);

    for (i = 0; params->isosurf_level->answers[i]; i++) {
	tokens = G_tokenize(params->isosurf_level->answers[i], ":");
	if (G_number_of_tokens(tokens) != 2) 
	    G_fatal_error(_("Error tokenize '%s'"), 
			  params->isosurf_level->answers[i]);
	num = atoi(tokens[0]);
	level = atoi(tokens[1]);
	G_free_tokens(tokens);

	if (num > nvols) {
	    G_fatal_error(_("Volume set number %d is not available"), 
			  num);
	}

	id = vol_list[num-1];
	if (GVL_isosurf_add(id) < 0) {
	    G_fatal_error(_("Unable to add isosurface (volume set %d)"),
			  id);
	}

	nisosurfs = GVL_isosurf_num_isosurfs(id);

	/* topography (level) */
	if (GVL_isosurf_set_att_const(id, nisosurfs-1, ATT_TOPO, level) < 0) {
	    G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
			  nisosurfs-1, ATT_TOPO, id);
	}

	/* color */
	if (GVL_isosurf_set_att_map(id, nisosurfs-1, ATT_COLOR, params->volume->answers[0]) < 0) {
	    G_fatal_error(_("Unable to set isosurface (%d) attribute (%d) of volume %d"),
			  nisosurfs-1, ATT_COLOR, id);
	}
    }
    
    return 1;
}
