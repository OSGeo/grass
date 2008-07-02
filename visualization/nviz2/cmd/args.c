/*!
  \file args.c
 
  \brief Parse command
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  \author Martin Landa <landa.martin gmail.com>

  \date 2008
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Parse command

   \param argc number of arguments
   \param argv arguments array
   \param params GRASS parameters

   \return 1
*/
void parse_command(int argc, char* argv[], struct GParams *params)
{
    params->mode_all = G_define_flag();
    params->mode_all->key = 'a';
    params->mode_all->description = _("Use draw mode for all loaded surfaces");

    /*
      surface attributes
    */
    /* topography */
    params->elev_map = G_define_standard_option(G_OPT_R_ELEV);
    params->elev_map->key = "elevation_map";
    params->elev_map->required = NO;
    params->elev_map->multiple = YES;
    params->elev_map->description = _("Name of raster map(s) for elevation");
    params->elev_map->guisection = _("Surface");

    params->elev_const = G_define_option();
    params->elev_const->key = "elevation_value";
    params->elev_const->key_desc = "value";
    params->elev_const->type = TYPE_INTEGER;
    params->elev_const->required = NO;
    params->elev_const->multiple = YES;
    params->elev_const->description = _("Elevation value(s)");
    params->elev_const->guisection = _("Surface");

    /* color */
    params->color_map = G_define_standard_option(G_OPT_R_MAP);
    params->color_map->multiple = YES;
    params->color_map->required = NO;
    params->color_map->description = _("Name of raster map(s) for color");
    params->color_map->guisection = _("Surface");
    params->color_map->key = "color_map";

    params->color_const = G_define_standard_option(G_OPT_C_FG);
    params->color_const->multiple = YES;
    params->color_const->label = _("Color value(s)");
    params->color_const->guisection = _("Surface");
    params->color_const->key = "color_value";
    params->color_const->answer = NULL;

    /* mask */
    params->mask_map = G_define_standard_option(G_OPT_R_MAP);
    params->mask_map->multiple = YES;
    params->mask_map->required = NO;
    params->mask_map->description = _("Name of raster map(s) for mask");
    params->mask_map->guisection = _("Surface");
    params->mask_map->key = "mask_map";

    /* transparency */
    params->transp_map = G_define_standard_option(G_OPT_R_MAP);
    params->transp_map->multiple = YES;
    params->transp_map->required = NO;
    params->transp_map->description = _("Name of raster map(s) for transparency");
    params->transp_map->guisection = _("Surface");
    params->transp_map->key = "transparency_map";

    params->transp_const = G_define_option();
    params->transp_const->key = "transparency_value";
    params->transp_const->key_desc = "value";
    params->transp_const->type = TYPE_INTEGER;
    params->transp_const->required = NO;
    params->transp_const->multiple = YES;
    params->transp_const->description = _("Transparency value(s)");
    params->transp_const->guisection = _("Surface");
    params->transp_const->options = "0-255";

    /* shininess */
    params->shine_map = G_define_standard_option(G_OPT_R_MAP);
    params->shine_map->multiple = YES;
    params->shine_map->required = NO;
    params->shine_map->description = _("Name of raster map(s) for shininess");
    params->shine_map->guisection = _("Surface");
    params->shine_map->key = "shininess_map";

    params->shine_const = G_define_option();
    params->shine_const->key = "shininess_value";
    params->shine_const->key_desc = "value";
    params->shine_const->type = TYPE_INTEGER;
    params->shine_const->required = NO;
    params->shine_const->multiple = YES;
    params->shine_const->description = _("Shininess value(s)");
    params->shine_const->guisection = _("Surface");
    params->shine_const->options = "0-255";

    /* emission */
    params->emit_map = G_define_standard_option(G_OPT_R_MAP);
    params->emit_map->multiple = YES;
    params->emit_map->required = NO;
    params->emit_map->description = _("Name of raster map(s) for emission");
    params->emit_map->guisection = _("Surface");
    params->emit_map->key = "emission_map";

    params->emit_const = G_define_option();
    params->emit_const->key = "emission_value";
    params->emit_const->key_desc = "value";
    params->emit_const->type = TYPE_INTEGER;
    params->emit_const->required = NO;
    params->emit_const->multiple = YES;
    params->emit_const->description = _("Emission value(s)");
    params->emit_const->guisection = _("Surface");
    params->emit_const->options = "0-255";

    /*
      draw
    */
    /* mode */
    params->mode = G_define_option();
    params->mode->key = "mode";
    params->mode->key_desc = "string";
    params->mode->type = TYPE_STRING;
    params->mode->required = YES;
    params->mode->multiple = YES;
    params->mode->description = _("Draw mode");
    params->mode->options = "coarse,fine,both";
    params->mode->answer = "fine";
    params->mode->guisection = _("Draw");

    /* resolution fine */
    params->res_fine = G_define_option();
    params->res_fine->key = "resolution_fine";
    params->res_fine->key_desc = "value";
    params->res_fine->type = TYPE_INTEGER;
    params->res_fine->required = YES;
    params->res_fine->multiple = YES;
    params->res_fine->description = _("Fine resolution");
    params->res_fine->answer = "6";
    params->res_fine->guisection = _("Draw");

    /* resolution coarse */
    params->res_coarse = G_define_option();
    params->res_coarse->key = "resolution_coarse";
    params->res_coarse->key_desc = "value";
    params->res_coarse->type = TYPE_INTEGER;
    params->res_coarse->required = YES;
    params->res_coarse->multiple = YES;
    params->res_coarse->description = _("Coarse resolution");
    params->res_coarse->answer = "9";
    params->res_coarse->guisection = _("Draw");

    /* style */
    params->style = G_define_option();
    params->style->key = "style";
    params->style->key_desc = "string";
    params->style->type = TYPE_STRING;
    params->style->required = YES;
    params->style->multiple = YES;
    params->style->description = _("Draw style");
    params->style->options = "wire,surface";
    params->style->answer = "surface";
    params->style->guisection = _("Draw");

    /* shading */
    params->shade = G_define_option();
    params->shade->key = "shading";
    params->shade->key_desc = "string";
    params->shade->type = TYPE_STRING;
    params->shade->required = YES;
    params->shade->multiple = YES;
    params->shade->description = _("Shading");
    params->shade->options = "flat,gouraud";
    params->shade->answer = "gouraud";
    params->shade->guisection = _("Draw");

    /* wire color */
    params->wire_color = G_define_standard_option(G_OPT_C_FG);
    params->wire_color->multiple = YES;
    params->wire_color->required = YES;
    params->wire_color->label = _("Wire color");
    params->wire_color->key = "wire_color";
    params->wire_color->answer = "136:136:136";
    params->wire_color->guisection = _("Draw");

    /*
      vector
    */
    params->vector = G_define_standard_option(G_OPT_V_MAP);
    params->vector->multiple = YES;
    params->vector->required = NO;
    params->vector->description = _("Name of vector overlay map(s)");
    params->vector->guisection = _("Vector");
    params->vector->key = "vector";

    /*
      misc
    */
    /* background color */
    params->bgcolor = G_define_standard_option(G_OPT_C_BG);

    /*
      viewpoint
    */
    /* position */
    params->pos = G_define_option();
    params->pos->key = "position";
    params->pos->key_desc = "x,y";
    params->pos->type = TYPE_DOUBLE;
    params->pos->required = NO;
    params->pos->multiple = NO;
    params->pos->description = _("Viewpoint position (x,y model coordinates)");
    params->pos->guisection = _("Viewpoint");
    params->pos->answer = "0.85,0.85";

    /* height */
    params->height = G_define_option();
    params->height->key = "height";
    params->height->key_desc = "value";
    params->height->type = TYPE_INTEGER;
    params->height->required = NO;
    params->height->multiple = NO;
    params->height->description = _("Viewpoint height (in map units)");
    params->height->guisection = _("Viewpoint");

    /* perspective */
    params->persp = G_define_option();
    params->persp->key = "perspective";
    params->persp->key_desc = "value";
    params->persp->type = TYPE_INTEGER;
    params->persp->required = NO;
    params->persp->multiple = NO;
    params->persp->description = _("Viewpoint field of view (in degrees)");
    params->persp->guisection = _("Viewpoint");
    params->persp->answer = "40";
    params->persp->options = "1-100";

    /* twist */
    params->twist = G_define_option();
    params->twist->key = "twist";
    params->twist->key_desc = "value";
    params->twist->type = TYPE_INTEGER;
    params->twist->required = NO;
    params->twist->multiple = NO;
    params->twist->description = _("Viewpoint twist angle (in degrees)");
    params->twist->guisection = _("Viewpoint");
    params->twist->answer = "0";
    params->twist->options = "-180-180";

    /* z-exag */
    params->exag = G_define_option();
    params->exag->key = "zexag";
    params->exag->key_desc = "value";
    params->exag->type = TYPE_DOUBLE;
    params->exag->required = NO;
    params->exag->multiple = NO;
    params->exag->description = _("Vertical exaggeration");


    /*
      image
    */
    /* output */
    params->output = G_define_standard_option(G_OPT_F_OUTPUT);
    params->output->description = _("Name for output file (do not add extension)");
    params->output->guisection = _("Image");

    /* format */
    params->format = G_define_option();
    params->format->key = "format";
    params->format->type = TYPE_STRING;
    params->format->options = "ppm,tif"; /* TODO: png */
    params->format->answer = "ppm";
    params->format->description = _("Graphics file format");
    params->format->required = YES;
    params->format->guisection = _("Image");

    /* size */
    params->size = G_define_option();
    params->size->key = "size";
    params->size->type = TYPE_INTEGER;
    params->size->key_desc = "width,height";
    params->size->answer = "640,480";
    params->size->description = _("Width and height of output image");
    params->size->required = YES;
    params->size->guisection = _("Image");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

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
	    if (strcmp(opt->answers[i], "")) {
		num++; /* skip empty values */
	    }
	    i++;
	}
    }

    return i;
}

/*!
  \brief Check parameters consistency

  \param params module parameters
*/
void check_parameters(const struct GParams * params)
{
    int nelevs;
    int ncolor_map, ncolor_const, nmasks, ntransps;
    int nshines, nemits;

    nelevs = opt_get_num_answers(params->elev_map);
    nelevs += opt_get_num_answers(params->elev_const);

    if (nelevs < 1)
	G_fatal_error(_("At least one <%s> or <%s> required"),
		      params->elev_map->key, params->elev_const->key);

    ncolor_map = opt_get_num_answers(params->color_map);
    ncolor_const = opt_get_num_answers(params->color_const);

    if (nelevs != ncolor_map + ncolor_const)
	G_fatal_error(_("Invalid number of color attributes (<%s> %d, <%s> %d"),
		      params->color_map->key, ncolor_map,
		      params->color_const->key, ncolor_const);

    nmasks = opt_get_num_answers(params->mask_map);
    ntransps = opt_get_num_answers(params->transp_map);
    ntransps += opt_get_num_answers(params->transp_const);
    nshines = opt_get_num_answers(params->shine_map);
    nshines += opt_get_num_answers(params->shine_const);
    nemits = opt_get_num_answers(params->emit_map);
    nemits += opt_get_num_answers(params->emit_const);


    return;
}
