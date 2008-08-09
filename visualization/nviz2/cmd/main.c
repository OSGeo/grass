
/****************************************************************************
 *
 * MODULE:       nviz_cmd
 *               
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com> (Google SoC 2008)
 *               
 * PURPOSE:      Experimental NVIZ CLI prototype
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

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct GParams *params;

    int ret;
    float vp_height, z_exag;	/* calculated viewpoint height, z-exag */
    int width, height;		/* output image size */
    char *output_name;

    nv_data data;
    struct render_window *offscreen;

    /* initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("visualization, raster, vector, raster3d");
    module->description = _("Experimental NVIZ CLI prototype.");

    params = (struct GParams *)G_malloc(sizeof(struct GParams));

    /* define options, call G_parser() */
    parse_command(argc, argv, params);

    /* check parameters consistency */
    check_parameters(params);

    width = atoi(params->size->answers[0]);
    height = atoi(params->size->answers[1]);
    G_asprintf(&output_name, "%s.%s", params->output->answer,
	       params->format->answer);

    GS_libinit();
    GVL_libinit();

    GS_set_swap_func(swap_gl);

    /* define render window */
    offscreen = Nviz_new_render_window();
    Nviz_init_render_window(offscreen);
    Nviz_create_render_window(offscreen, NULL, width, height);	/* offscreen display */
    Nviz_make_current_render_window(offscreen);

    /* initialize nviz data */
    Nviz_init_data(&data);

    /* define default attributes for map objects */
    Nviz_set_surface_attr_default();

    /* set background color */
    Nviz_set_bgcolor(&data, Nviz_color_from_str(params->bgcolor->answer));

    /* init view */
    Nviz_init_view();
    /* set lights */
    /* TODO: add options */
    Nviz_set_light_position(&data, 0, 0.68, -0.68, 0.80, 0.0);
    Nviz_set_light_bright(&data, 0, 0.8);
    Nviz_set_light_color(&data, 0, 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(&data, 0, 0.2, 0.2, 0.2);
    Nviz_set_light_position(&data, 1, 0.0, 0.0, 1.0, 0.0);
    Nviz_set_light_bright(&data, 1, 0.5);
    Nviz_set_light_color(&data, 1, 1.0, 1.0, 1.0);
    Nviz_set_light_ambient(&data, 1, 0.3, 0.3, 0.3);

    /* load raster maps (surface topography) & set attributes (map/constant) */
    load_rasters(params, &data);
    /* set draw mode of loaded surfaces */
    surface_set_draw_mode(params);

    /* load line vector maps */
    if (params->vlines->answer) {
	load_vlines(params, &data);
	/* set attributes of 2d lines */
	vlines_set_attrb(params);
    }

    /* load point vector maps */
    if (params->vpoints->answer) {
	load_vpoints(params, &data);
	/* set attributes for 2d lines */
	vpoints_set_attrb(params);
    }

    /* load volumes */
    if (params->volume->answer) {
	load_rasters3d(params, &data);
    }

    /* define isosurfaces for displaying volumes */
    if (params->isosurf_level->answer) {
	add_isosurfs(params, &data);
    }

    /* focus on loaded data */
    Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1);

    /* define view point */
    if (params->exag->answer) {
	z_exag = atof(params->exag->answer);
    }
    else {
	z_exag = Nviz_get_exag();
	G_message(_("Vertical exaggeration not given, using calculated value %.0f"),
		  z_exag);
    }
    Nviz_change_exag(&data, z_exag);

    if (params->height->answer) {
	vp_height = atof(params->height->answer);
    }
    else {
	Nviz_get_exag_height(&vp_height, NULL, NULL);
	G_message(_("Viewpoint height not given, using calculated value %.0f"),
		  vp_height);
    }
    Nviz_set_viewpoint_height(&data, vp_height);

    Nviz_set_viewpoint_position(&data,
				atof(params->pos->answers[0]),
				atof(params->pos->answers[1]));
    Nviz_set_viewpoint_twist(&data, atoi(params->twist->answer));
    Nviz_set_viewpoint_persp(&data, atoi(params->persp->answer));

    GS_clear(data.bgcolor);

    /* draw */
    Nviz_draw_cplane(&data, -1, -1);
    Nviz_draw_all(&data);

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

    G_free((void *)output_name);
    G_free((void *)params);

    exit(EXIT_SUCCESS);
}

void swap_gl()
{
    return;
}
