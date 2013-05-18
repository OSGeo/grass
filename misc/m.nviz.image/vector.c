/*!
  \file vector.c
  
  \brief Vector subroutines
  
  (C) 2008, 2010-2011 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
*/

#include <stdlib.h>
#include <string.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int load_vectors(const struct Option *, const struct Option *,
			const struct Option *, const struct Option *, int, nv_data *);
static void error_handler_vector(void *);
static void error_handler_db(void *);

/*!
  \brief Load vector maps (lines)
  
  \param params module parameters
  \param data nviz data
  
  \return number of loaded vectors
*/
int load_vlines(const struct GParams *params, nv_data * data)
{
    return load_vectors(params->elev_map, params->elev_const,
			params->vlines, params->vline_pos,
			MAP_OBJ_VECT, data);
}

/*!
  \brief Load vector maps (points)
  
  \param params module parameters
  \param data nviz data
  
  \return number of loaded vectors
*/
int load_vpoints(const struct GParams *params, nv_data * data)
{
    return load_vectors(params->elev_map, params->elev_const,
			params->vpoints, params->vpoint_pos,
			MAP_OBJ_SITE, data);
}

int load_vectors(const struct Option *elev_map,
		 const struct Option *elev_const, const struct Option *vect,
		 const struct Option *position,
		 int map_obj_type, nv_data * data)
{
    int i, id;
    int nvects;

    const char *mapset;

    double x, y, z;
    
    if ((!elev_map->answer || elev_const->answer) && GS_num_surfs() == 0) {	/* load base surface if no loaded */
	int *surf_list, nsurf;

	Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, data);

	surf_list = GS_get_surf_list(&nsurf);
	GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
    }

    nvects = 0;

    for (i = 0; vect->answers[i]; i++) {
	mapset = G_find_vector2(vect->answers[i], "");
	if (mapset == NULL) {
	    G_fatal_error(_("Vector map <%s> not found"), vect->answers[i]);
	}
	id = Nviz_new_map_obj(map_obj_type,
			      G_fully_qualified_name(vect->answers[i], mapset),
			      0.0, data);

	/* set position */
	x = atof(position->answers[i*3+0]);
	y = atof(position->answers[i*3+1]);
	z = atof(position->answers[i*3+2]);

	if (map_obj_type == MAP_OBJ_VECT)
	    GV_set_trans(id, x, y, z);
	else
	    GP_set_trans(id, x, y, z);

	nvects++;
    }

    return nvects;
}

/*!
  \brief Set vector lines mode
  
  \param params parameters
  
  \return 1 on success
  \return 0 on failure
*/
int vlines_set_attrb(const struct GParams *params)
{
    int i, layer, color, width, flat, height;
    int *vect_list, nvects;
    int have_colors;

    char *color_column, *width_column;
    struct Colors colors;
    
    vect_list = GV_get_vect_list(&nvects);

    for (i = 0; i < nvects; i++) {
	layer = check_thematic(params, TRUE);
	
	color = Nviz_color_from_str(params->vline_color->answers[i]);
	color_column = params->vline_color_column->answers ?
	    params->vline_color_column->answers[i] : NULL;
	width = atoi(params->vline_width->answers[i]);
	width_column = params->vline_width_column->answers ?
	    params->vline_width_column->answers[i] : NULL;
	
	if (strcmp(params->vline_mode->answers[i], "flat") == 0)
	    flat = 1;
	else
	    flat = 0;
	
	/* style (mode -- use memory by default) */
	if (GV_set_style(vect_list[i], TRUE, color, width, flat) < 0)
	    return 0;
	
	/* check for vector color table */
	have_colors = Vect_read_colors(params->vlines->answers[i], "",
				       &colors);
	
	if (have_colors || color_column || width_column)
	    if (GV_set_style_thematic(vect_list[i], layer, color_column,
				      width_column, have_colors ? &colors : NULL) < 0)
		return 0;

	/* height */
	height = atoi(params->vline_height->answers[i]);
	if (height > 0)
	    GV_set_trans(vect_list[i], 0.0, 0.0, height);
    }

    return 1;
}

/*!
  \brief Set vector points style
  
  \param params parameters
  
  \return 1 on success
  \return 0 on failure
*/
int vpoints_set_attrb(const struct GParams *params)
{
    int i, layer, have_colors;
    int *site_list, nsites;
    int marker, color, width;
    float size;
    char *marker_str, *color_column, *size_column, *width_column, *marker_column;

    struct Colors colors;
    
    site_list = GP_get_site_list(&nsites);

    for (i = 0; i < nsites; i++) {
	layer = check_thematic(params, FALSE);
	color = Nviz_color_from_str(params->vpoint_color->answers[i]);
	color_column = params->vpoint_color_column->answers ?
	    params->vpoint_color_column->answers[i] : NULL;
	size = atof(params->vpoint_size->answers[i]);
	size_column = params->vpoint_size_column->answers ?
	    params->vpoint_size_column->answers[i] : NULL;
	width = atoi(params->vpoint_width->answers[i]);
	width_column = params->vpoint_width_column->answers ?
	    params->vpoint_width_column->answers[i] : NULL;
	marker_str = params->vpoint_marker->answers[i];
	marker_column = params->vpoint_marker_column->answers ?
	    params->vpoint_marker_column->answers[i] : NULL;
	marker = GP_str_to_marker(marker_str);
	
	if (GP_set_style(site_list[i], color, width, size, marker) < 0)
	    return 0;

	/* check for vector color table */
	have_colors = Vect_read_colors(params->vpoints->answers[i], "",
				       &colors);
	
	if (have_colors || color_column || width_column ||
	    size_column || marker_column) {
	    if (GP_set_style_thematic(site_list[i], layer, color_column,
				      width_column, size_column, marker_column,
				      have_colors ? &colors : NULL) < 0)
		return 0;
	}
    }

    return 1;
}

int check_thematic(const struct GParams *params, int vlines)
{
    int i, type;
    struct Map_info Map;
    struct Option *map, *layer, *color, *size, *width, *marker;
    struct field_info *Fi;

    dbDriver *driver;
    dbColumn *column;
    
    Fi = NULL;
    
    if (vlines) {
	map    = params->vlines;
	layer  = params->vline_layer;
	color  = params->vline_color_column;
	size   = NULL;
	width  = params->vline_width_column;
	marker = NULL;
    }
    else {
	map    = params->vpoints;
	layer  = params->vpoint_layer;
	color  = params->vpoint_color_column;
	size   = params->vpoint_size_column;
	width  = params->vpoint_width_column;
	marker = params->vpoint_marker_column;
    }
    
    driver = NULL;
    for (i = 0; map->answers[i]; i++) {
	if (1 > Vect_open_old(&Map, map->answers[i], ""))
	    G_fatal_error(_("Unable to open vector map <%s>"), map->answers[i]);
        G_add_error_handler(error_handler_vector, &Map);
        
	Fi = Vect_get_field2(&Map, layer->answers[i]);
	if (!Fi)
	    continue;
	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (!driver)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	G_add_error_handler(error_handler_db, driver);

	if (color->answers && color->answers[i]) {
	    db_get_column(driver, Fi->table, color->answers[i], &column);
	    if (!column)
		G_fatal_error(_("Column <%s> in table <%s> not found"),
			      color->answers[i], Fi->table);
	    
	    if (db_column_Ctype(driver, Fi->table, color->answers[i]) != DB_C_TYPE_STRING)
		G_fatal_error(_("Data type of color column must be character"));
	}
	if (size && size->answers && size->answers[i]) {
	    db_get_column(driver, Fi->table, size->answers[i], &column);
	    if (!column)
		G_fatal_error(_("Column <%s> in table <%s> not found"),
			      size->answers[i], Fi->table);
	    
	    type = db_column_Ctype(driver, Fi->table, size->answers[i]);
	    if (type != DB_C_TYPE_INT && type != DB_C_TYPE_DOUBLE)
		G_fatal_error(_("Data type of size column must be numeric"));
	}
	if (width->answers && width->answers[i]) {
	    db_get_column(driver, Fi->table, width->answers[i], &column);
	    if (!column)
		G_fatal_error(_("Column <%s> in table <%s> not found"),
			      width->answers[i], Fi->table);
	    
	    type = db_column_Ctype(driver, Fi->table, width->answers[i]);
	    if (type != DB_C_TYPE_INT && type != DB_C_TYPE_DOUBLE)
		G_fatal_error(_("Data type of width column must be numeric"));
	}
	if (marker && marker->answers && marker->answers[i]) {
	    db_get_column(driver, Fi->table, marker->answers[i], &column);
	    if (!column)
		G_fatal_error(_("Column <%s> in table <%s> not found"),
			      marker->answers[i], Fi->table);
	  
	  type = db_column_Ctype(driver, Fi->table, marker->answers[i]);
	    if (db_column_Ctype(driver, Fi->table, marker->answers[i]) != DB_C_TYPE_STRING)
		G_fatal_error(_("Data type of marker column must be character"));
	}

        G_remove_error_handler(error_handler_db, driver);        
        db_close_database_shutdown_driver(driver);
    }

    G_remove_error_handler(error_handler_vector, &Map);
    Vect_close(&Map);

    if (Fi) 
	return Fi->number;
    
    return 1;
}

void error_handler_vector(void *p)
{
    struct Map_info *Map;

    Map = (struct Map_info *)p;

    Vect_close(Map);
}

void error_handler_db(void *p)
{
    dbDriver *driver;

    driver = (dbDriver *)p;
    if (driver) 
        db_close_database_shutdown_driver(driver);
}
