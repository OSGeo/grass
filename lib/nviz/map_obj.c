/*!
   \file lib/nviz/map_obj.c

   \brief Nviz library -- Define creation and interface functions for map objects.

   Map objects are considered to be surfaces, vector plots, or site
   files.

   Based on visualization/nviz/src/map_obj.c
   
   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <stdlib.h>
#include <time.h>

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief Create a new map object which can be one of surf, vect, vol or site.

   This routine creates the object internally in the gsf libraryb.
   Optionally, a logical name may be specified for the new map object.
   If no name is specified, a logical name is assigned to the new
   object automatically.  Note that maintaining unique logical names is
   not the responsibility of the library (currently).

   Initially map objects contain no data, use the attribute commands to
   set attributes such as topology, color, etc.

   \param type map object type
   \param name map name (NULL for constant)
   \param value constant (used if <i>name</i> is NULL)
   \param data nviz data

   \return map object id
   \return -1 on error
 */
int Nviz_new_map_obj(int type, const char *name, double value, nv_data * data)
{
    int new_id, i;
    int num_surfs, *surf_list;

    /*
     * For each type of map obj do the following --
     *   1) Verify we haven't maxed out the number of
     *      allowed objects.
     *   2) Call the internal library to generate a new
     *      map object of the specified type.
     */
    /* raster -> surface */
    if (type == MAP_OBJ_SURF) {
	if (GS_num_surfs() >= MAX_SURFS) {
	    G_warning(_("Maximum surfaces loaded!"));
	    return -1;
	}

	new_id = GS_new_surface();

	if (new_id < 0) {
	    return -1;
	}

	if (name) {
	    /* map */
	    if (!Nviz_set_attr(new_id, MAP_OBJ_SURF, ATT_TOPO,
			       MAP_ATT, name, -1.0, data)) {
		return -1;
	    }
	}
	else {
	    /* constant */
	    if (!Nviz_set_attr(new_id, MAP_OBJ_SURF, ATT_TOPO,
			       CONST_ATT, NULL, value,
		 data)) {
		return -1;
	    }
	}
    }
    /* vector overlay */
    else if (type == MAP_OBJ_VECT) {
	if (GV_num_vects() >= MAX_VECTS) {
	    G_warning(_("Maximum vector line maps loaded!"));
	    return -1;
	}

	new_id = GV_new_vector();

	if (name) {
	    if (GV_load_vector(new_id, name) < 0) {
		GV_delete_vector(new_id);
		G_warning(_("Error loading vector map <%s>"), name);
		return -1;
	    }
	}

	/* initialize display parameters
	   automatically select all surfaces to draw vector */
	GV_set_style(new_id, 1, 0x000000, 2, 0);
	surf_list = GS_get_surf_list(&num_surfs);
	if (num_surfs) {
	    for (i = 0; i < num_surfs; i++) {
		GV_select_surf(new_id, surf_list[i]);
	    }
	}
	G_free(surf_list);
    }
    /* vector points overlay */
    else if (type == MAP_OBJ_SITE) {
	if (GP_num_sites() >= MAX_SITES) {
	    G_warning(_("Maximum vector point maps loaded!"));
	    return -1;
	}

	new_id = GP_new_site();

	/* initizalize site attributes */
	Nviz_set_vpoint_attr_default(new_id);

	/* load vector points */
	if (0 > GP_load_site(new_id, name)) {
	    GP_delete_site(new_id);
	    G_warning(_("Error loading vector map <%s>"), name);
	    return -1;
	}

	/* initialize display parameters */
	GP_set_style(new_id, 0x000000, 2, 100, ST_X);
	surf_list = GS_get_surf_list(&num_surfs);
	for (i = 0; i < num_surfs; i++) {
	    GP_select_surf(new_id, surf_list[i]);
	}
	G_free(surf_list);
    }
    /* 3d raster map -> volume */
    else if (type == MAP_OBJ_VOL) {
	if (GVL_num_vols() >= MAX_VOLS) {
	    G_warning(_("Maximum volumes loaded!"));
	    return -1;
	}
	
	new_id = GVL_new_vol();

	/* load volume */
	if (0 > GVL_load_vol(new_id, name)) {
	    GVL_delete_vol(new_id);
	    G_warning(_("Error loading 3d raster map <%s>"), name);
	    return -1;
	}

	/* initilaze volume attributes */
	Nviz_set_volume_attr_default(new_id);
    }
    else {
	G_warning(_("Nviz_new_map_obj(): unsupported data type"));
	return -1;
    }
    
    return new_id;
}

/*!
   Set map object attribute

   \param id map object id
   \param type map object type (MAP_OBJ_SURF, MAP_OBJ_VECT, ...)
   \param desc attribute descriptor
   \param src attribute source
   \param str_value attribute value as string (if NULL, check for <i>num_value</i>)
   \param num_value attribute value as double 

   \return 1 on success
   \return 0 on failure
 */
int Nviz_set_attr(int id, int type, int desc, int src,
		  const char *str_value, double num_value, nv_data * data)
{
    int ret;
    double value;

    switch (type) {
    case (MAP_OBJ_SURF):{
	    /* Basically two cases, either we are setting to a constant field, or
	     * we are loading an actual file. Setting a constant is the easy part
	     * so we try and do that first.
	     */
	    if (src == CONST_ATT) {
		/* Get the value for the constant
		 * Note that we require the constant to be an integer
		 */
		if (str_value)
		    value = (double)atof(str_value);
		else
		    value = num_value;

		/* Only special case is setting constant color.
		 * In this case we have to decode the constant Tcl
		 * returns so that the gsf library understands it.
		 */
		if (desc == ATT_COLOR) {
		    /* TODO check this - sometimes gets reversed when save state
		       saves a surface with constant color

		       int r, g, b;
		       r = (((int) value) & RED_MASK) >> 16;
		       g = (((int) value) & GRN_MASK) >> 8;
		       b = (((int) value) & BLU_MASK);
		       value = r + (g << 8) + (b << 16);
		     */
		}

		/* Once the value is parsed, set it */
		ret = GS_set_att_const(id, desc, value);
	    }
	    else if (src == MAP_ATT) {
		ret = GS_load_att_map(id, str_value, desc);
	    }
	    else
		ret = -1;

	    /* After we've loaded a constant map or a file,
	     * may need to adjust resolution if we are resetting
	     * topology (for example)
	     */
	    if (0 <= ret) {
		if (desc == ATT_TOPO) {
		    int rows, cols, max;
		    int max2;

		    /* If topology attribute is being set then need to set
		     * resolution of incoming map to some sensible value so we
		     * don't wait all day for drawing.
		     */
		    GS_get_dims(id, &rows, &cols);
		    max = (rows > cols) ? rows : cols;
		    max = max / 50;
		    if (max < 1)
			max = 1;
		    max2 = max / 5;
		    if (max2 < 1)
			max2 = 1;
		    /* reset max to finer for coarse surf drawing */
		    max = max2 + max2 / 2;
		    if (max < 1)
			max = 1;

		    GS_set_drawres(id, max2, max2, max, max);
		    GS_set_drawmode(id, DM_GOURAUD | DM_POLY | DM_GRID_SURF);
		}

		/* Not sure about this next line, should probably just
		 * create separate routines to figure the Z range as well
		 * as the XYrange
		 */
		Nviz_update_ranges(data);

		break;
	    }
    default:{
		return 0;
	    }
	}
    }

    return 1;
}

/*!
   \brief Set default surface attributes
 */
void Nviz_set_surface_attr_default()
{
    float defs[MAX_ATTS];

    defs[ATT_TOPO] = 0;
    defs[ATT_COLOR] = DEFAULT_SURF_COLOR;
    defs[ATT_MASK] = 0;
    defs[ATT_TRANSP] = 0;
    defs[ATT_SHINE] = 60;
    defs[ATT_EMIT] = 0;

    GS_set_att_defaults(defs, defs);

    return;
}

/*!
   \brief Set default vector point attributes

   \param id vector point set id

   \return 1 on success
   \return 0 on failure
 */
int Nviz_set_vpoint_attr_default(int id)
{
    geosite *gp;

    gp = gp_get_site(id);

    if (!gp)
	return 0;
    
    return 1;
}

/*!
   \brief Set default volume attributes

   \param id volume set id

   \return 1 on success
   \return 0 on failure
 */
int Nviz_set_volume_attr_default(int id)
{
    int rows, cols, depths;
    int max;

    GVL_get_dims(id, &rows, &cols, &depths);
    max = (rows > cols) ? rows : cols;
    max = (depths > max) ? depths : max;
    max = max / 35;
    if (max < 1)
	max = 1;
    
    if (max > cols)
	max = cols / 2;
    if (max > rows)
	max = rows / 2;
    if (max > depths)
	max = depths / 2;
    
    /* set default drawres and drawmode for isosurfaces */
    GVL_isosurf_set_drawres(id, max, max, max);
    GVL_isosurf_set_drawmode(id, DM_GOURAUD);
    
    /* set default drawres and drawmode for slices */
    GVL_slice_set_drawres(id, 1, 1, 1);
    GVL_slice_set_drawmode(id, DM_GOURAUD | DM_POLY);

    return 1;
}

/*!
   Unset map object attribute

   \param id map object id
   \param type map object type (MAP_OBJ_SURF, MAP_OBJ_VECT, ...)
   \param desc attribute descriptor

   \return 1 on success
   \return 0 on failure
 */
int Nviz_unset_attr(int id, int type, int desc)
{
    if (type == MAP_OBJ_SURF) {
	return GS_unset_att(id, desc);
    }

    return 0;
}
