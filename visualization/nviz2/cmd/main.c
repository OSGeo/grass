/****************************************************************************
 *
 * MODULE:       nviz_cmd
 *               
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Experimental NVIZ CLI prototype
 *               Google SoC 2008
 *               
 * COPYRIGHT:    (C) 2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/nviz.h>

#include "local_proto.h"

static void swap_gl();
static int opt_get_num_answers(const struct Option *);

int main (int argc, char *argv[])
{
    struct GModule *module;
    struct GParams *params;

    char *mapset;
    unsigned int i;
    unsigned int ncolor_map, ncolor_const, nmask_map;
    unsigned int ntransp_map, ntransp_const, nshine_map, nshine_const;
    unsigned int nemit_map, nemit_const;
    int *surf_list, nsurfs;
    int id, ret;
    float vp_height, z_exag; /* calculated viewpoint height, z-exag */
    int width, height; /* output image size */
    char *output_name;

    nv_data data;
    struct render_window *offscreen;

    /* initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("visualization, raster, vector");
    module->description = _("Experimental NVIZ CLI prototype.");

    params = (struct GParams*) G_malloc(sizeof (struct GParams));

    /* define options, call G_parser() */
    parse_command(argc, argv, params);

    width = atoi(params->size->answers[0]);
    height = atoi(params->size->answers[1]);
    G_asprintf(&output_name, "%s.%s", params->output->answer, params->format->answer);

    GS_libinit();
    /* GVL_libinit(); TODO */

    GS_set_swap_func(swap_gl);

    /* define render window */
    offscreen = Nviz_new_render_window();
    Nviz_init_render_window(offscreen);
    Nviz_create_render_window(offscreen, NULL, width, height); /* offscreen display */
    Nviz_make_current_render_window(offscreen);

    /* initialize nviz data */
    Nviz_init_data(&data);

    /* define default attributes for map objects */
    Nviz_set_attr_default();

    /* set background color */
    Nviz_set_bgcolor(&data, Nviz_color_from_str(params->bgcolor->answer)); 

    /* init view */
    Nviz_init_view();
    /* set lights */
    /* TODO: add options */
    Nviz_set_light_position(&data, 0,
			    0.68, -0.68, 0.80, 0.0);
    Nviz_set_light_bright(&data, 0,
			  0.8);
    Nviz_set_light_color(&data, 0,
			 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(&data, 0,
			   0.2, 0.2, 0.2);
    Nviz_set_light_position(&data, 1,
			    0.0, 0.0, 1.0, 0.0);
    Nviz_set_light_bright(&data, 1,
			  0.5);
    Nviz_set_light_color(&data, 1,
			 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(&data, 1,
			   0.3, 0.3, 0.3);

    /*
     * load raster maps (surface topography) map/constant
     */
    if (params->elev_map->answer) {
	/* maps */
	for (i = 0; params->elev_map->answers[i]; i++) {
	    mapset = G_find_cell2 (params->elev_map->answers[i], "");
	    if (mapset == NULL) {
		G_fatal_error(_("Raster map <%s> not found"),
			      params->elev_map->answers[i]);
	    }
	    
	    /* topography */
	    id = Nviz_new_map_obj(MAP_OBJ_SURF,
				  G_fully_qualified_name(params->elev_map->answers[i], mapset), 0.0,
				  &data);
	}
    }

    ncolor_map = opt_get_num_answers(params->color_map);
    ncolor_const = opt_get_num_answers(params->color_const);
    if (params->elev_const->answer) {
	/* constants */
	float value;
	surf_list = GS_get_surf_list(&nsurfs);
	for (i = 0; params->elev_const->answers[i]; i++) {
	    value = atof(params->elev_const->answers[i]);
	    /* check for color */
	    if (i + nsurfs >= ncolor_map + ncolor_const) {
		G_fatal_error (_("Missing color settings for elevation value %f"),
			       value);
		/* topography */
		id = Nviz_new_map_obj(MAP_OBJ_SURF,
				      NULL, value,
				      &data);
	    }
	}
    }

    /* set surface attributes */
    surf_list = GS_get_surf_list(&nsurfs);
    nmask_map = opt_get_num_answers(params->mask_map);
    ntransp_map = opt_get_num_answers(params->transp_map);
    ntransp_const = opt_get_num_answers(params->transp_const);
    nshine_map = opt_get_num_answers(params->shine_map);
    nshine_const = opt_get_num_answers(params->shine_const);
    nemit_map = opt_get_num_answers(params->emit_map);
    nemit_const = opt_get_num_answers(params->emit_const);
    for (i = 0; i < (unsigned int) nsurfs; i++) {
	id = surf_list[i];
	/* color */
	if (i < ncolor_map) {
	    mapset = G_find_cell2 (params->color_map->answers[i], "");
	    if (mapset == NULL) {
		G_fatal_error(_("Raster map <%s> not found"),
			      params->color_map->answers[i]);
	    }
	    
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
			  G_fully_qualified_name(params->color_map->answers[i], mapset), -1.0,
			  &data);
	}
	else if (i < ncolor_map + ncolor_const) { /* check for color value */
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT,
			  NULL, Nviz_color_from_str(params->color_const->answers[i]),
			  &data);
	}
	else { /* use by default elevation map for coloring */
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT,
			  G_fully_qualified_name(params->elev_map->answers[i], mapset), -1.0,
			  &data);
	}
	/* mask */
	if (i < nmask_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_MASK, MAP_ATT,
			  G_fully_qualified_name(params->mask_map->answers[i], mapset), -1.0,
			  &data);
	}

	/* transparency */
	if (i < ntransp_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, MAP_ATT,
			  G_fully_qualified_name(params->transp_map->answers[i], mapset), -1.0,
			  &data);
	}
	else if (i < ntransp_map + ntransp_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, CONST_ATT,
			  NULL, atof(params->transp_const->answers[i-ntransp_map]),
			  &data);
	}

	/* shininess */
	if (i < nshine_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, MAP_ATT,
			  G_fully_qualified_name(params->shine_map->answers[i], mapset), -1.0,
			  &data);
	}
	else if (i < nshine_map + nshine_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, CONST_ATT,
			  NULL, atof(params->shine_const->answers[i-nshine_map]),
			  &data);
	}

	/* emission */
	if (i < nemit_map) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, MAP_ATT,
			  G_fully_qualified_name(params->emit_map->answers[i], mapset), -1.0,
			  &data);
	}
	else if (i < nemit_map + nemit_const) {
	    Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, CONST_ATT,
			  NULL, atof(params->emit_const->answers[i-nemit_map]),
			  &data);
	}

        /*
	  if (i > 1)
	  set_default_wirecolors(data, i);
	*/
    }

    /* load vectors */
    if (params->vector->answer) {
	if (!params->elev_map->answer && GS_num_surfs() == 0) { /* load base surface if no loaded */
	    int *surf_list, nsurf;

	    Nviz_new_map_obj(MAP_OBJ_SURF, NULL, 0.0, &data);

	    surf_list = GS_get_surf_list(&nsurf);
	    GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
	}

	for (i = 0; params->vector->answers[i]; i++) {
	    mapset = G_find_vector2 (params->vector->answers[i], "");
	    if (mapset == NULL) {
		G_fatal_error(_("Vector map <%s> not found"),
			      params->vector->answers[i]);
	    }
	    Nviz_new_map_obj(MAP_OBJ_VECT,
			     G_fully_qualified_name(params->vector->answers[i], mapset), 0.0,
			     &data);
	}
    }

    /* focus on loaded data */
    Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1);

    /* define view point */
    if (params->exag->answer) {
	z_exag = atof(params->exag->answer);
    }
    else {
	z_exag = Nviz_get_exag();
	G_message(_("Vertical exaggeration not given, using calculated value %f"),
		  z_exag);
    }
    Nviz_change_exag(&data,
		     z_exag);

    if (params->height->answer) {
	vp_height = atof(params->height->answer);
    }
    else {
	Nviz_get_exag_height(&vp_height, NULL, NULL);
	G_message(_("Viewpoint height not given, using calculated value %f"), vp_height);
    }
    Nviz_set_viewpoint_height(&data,
			      vp_height);
    
    Nviz_set_viewpoint_position(&data,
				atof(params->pos->answers[0]),
				atof(params->pos->answers[1]));
    Nviz_set_viewpoint_twist(&data,
			     atoi(params->twist->answer));
    Nviz_set_viewpoint_persp(&data,
			     atoi(params->persp->answer));

    GS_clear(data.bgcolor);

    /* draw */
    Nviz_draw_cplane(&data, -1, -1);
    Nviz_draw_all (&data, 1); /* clear screen */

    /* write to image */
    ret = 0;
    if (strcmp(params->format->answer, "ppm") == 0)
	ret = write_img(output_name, FORMAT_PPM); 
    if (strcmp(params->format->answer, "tif") == 0)
	ret = write_img(output_name, FORMAT_TIF); 
    
    if (!ret)
	G_fatal_error(_("Unsupported output format"));

    G_done_msg(_("File <%s> created."), output_name);

    Nviz_destroy_render_window(offscreen);

    G_free ((void *) output_name);
    G_free ((void *) params);

    exit(EXIT_SUCCESS);
}

void swap_gl()
{
    return;
}

/*!
  \brief Get number of answers of given option

  \param pointer to option

  \return number
*/
int opt_get_num_answers(const struct Option *opt)
{
    int i, num;
    i = num = 0;
    if (opt->answer) {
	while (opt->answers[i]) {
	    if (strcmp(opt->answers[i++], "")) {
		num++; /* skip empty values */
	    }
	}
    }

    return i;
}
