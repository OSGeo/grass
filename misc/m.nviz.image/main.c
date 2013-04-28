
/****************************************************************************
 *
 * MODULE:       m.nviz.image
 *               
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 *               Anna Kratochvilova (Google SoC 2011)
 *               
 * PURPOSE:      Renders GIS data in 3D space.
 *               
 * COPYRIGHT:    (C) 2008, 2010-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include <grass/nviz.h>

#include "local_proto.h"

static void swap_gl();

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct GParams *params;

    int i, ret;
    int red, grn, blu;
    float size;
    double vp_height, z_exag;	/* calculated viewpoint height, z-exag */
    int width, height;		/* output image size */
    char *output_name;
    
    nv_data data;
    struct render_window *offscreen;

    /* initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("visualization"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("raster3d"));
    module->label = _("Creates a 3D rendering of GIS data.");
    module->description = _("Renders surfaces (raster data), "
			    "2D/3D vector data, and "
			    "volumes (3D raster data) in 3D.");

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
    if (Nviz_create_render_window(offscreen, NULL, width, height) == -1)
        G_fatal_error(_("Unable to render data"));
    Nviz_make_current_render_window(offscreen);

    /* initialize nviz data */
    Nviz_init_data(&data);

    /* define default attributes for map objects */
    Nviz_set_surface_attr_default();

    /* set background color */
    Nviz_set_bgcolor(&data, Nviz_color_from_str(params->bgcolor->answer));

    /* init view, lights */
    Nviz_init_view(&data);

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
	/* set attributes for points */
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

    /* define slices for displaying volumes */
    if (params->slice->answer) {
	add_slices(params, &data);
    }

    /* focus on loaded data */
    Nviz_set_focus_map(MAP_OBJ_UNDEFINED, -1);

    /* define view point */
    if (params->exag->answer) {
	z_exag = atof(params->exag->answer);
    }
    else {
	z_exag = Nviz_get_exag();
	G_verbose_message(_("Vertical exaggeration not given, using calculated "
			    "value %.0f"),
			  z_exag);
    }
    Nviz_change_exag(&data, z_exag);

    if (params->height->answer) {
	vp_height = atof(params->height->answer);
    }
    else {
	double min, max;
	Nviz_get_exag_height(&vp_height, &min, &max);
	G_verbose_message(_("Viewpoint height not given, using calculated "
			    "value %.0f"),
			  vp_height);
    }
    Nviz_set_viewpoint_height(vp_height);

    Nviz_set_viewpoint_position(atof(params->pos->answers[0]),
				atof(params->pos->answers[1]));
    Nviz_set_viewpoint_twist(atoi(params->twist->answer));
    Nviz_set_viewpoint_persp(atoi(params->persp->answer));
    if (params->focus->answer) {
    Nviz_set_focus(&data, atof(params->focus->answers[0]),
                          atof(params->focus->answers[1]), 
                          atof(params->focus->answers[2]));
    }

    /* set lights */
    Nviz_set_light_position(&data, 1,
			    atof(params->light_pos->answers[0]),
			    atof(params->light_pos->answers[1]),
			    atof(params->light_pos->answers[2]),
			    0.0);
    Nviz_set_light_bright(&data, 1,
			  atoi(params->light_bright->answer) / 100.0);
    if(G_str_to_color(params->light_color->answer, &red, &grn, &blu) != 1) {
	red = grn = blu = 255;
    }
    Nviz_set_light_color(&data, 1, red, grn, blu);
    Nviz_set_light_ambient(&data, 1,
			   atof(params->light_ambient->answer) / 100.0);

    /* define fringes */
    if (params->fringe->answer) {
	int nw, ne, sw, se;
	
	i = 0;
	nw = ne = sw = se = 0;
	while(params->fringe->answers[i]) {
	    const char *edge = params->fringe->answers[i++];
	    if (strcmp(edge, "nw") == 0)
		nw = 1;
	    else if (strcmp(edge, "ne") == 0)
		ne = 1;
	    else if (strcmp(edge, "sw") == 0)
		sw = 1;
	    else if (strcmp(edge, "se") == 0)
		se = 1;
	}
	Nviz_new_fringe(&data, -1, Nviz_color_from_str(params->fringe_color->answer),
			atof(params->fringe_elev->answer), nw, ne, sw, se);
    }

    /* draw north arrow */
    if (params->north_arrow->answer) {
	
	if (!params->north_arrow_size->answer)
	    size = Nviz_get_longdim(&data) / 8.;
	else
	    size = atof(params->north_arrow_size->answer);

	Nviz_set_arrow(&data, atoi(params->north_arrow->answers[0]),
			      atoi(params->north_arrow->answers[1]),
			      size, Nviz_color_from_str(params->north_arrow_color->answer));
	Nviz_draw_arrow(&data);
    }

    GS_clear(data.bgcolor);

    /* cutting planes */
    if(params->cplane->answer)
        draw_cplane(params, &data);
    /* draw */
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

    Nviz_destroy_data(&data);
    Nviz_destroy_render_window(offscreen);

    G_free((void *)output_name);
    G_free((void *)params);

    exit(EXIT_SUCCESS);
}

void swap_gl()
{
    return;
}
