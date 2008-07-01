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
    /* surface */
    params->elev_map = G_define_standard_option(G_OPT_R_ELEV);
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

    params->mask_map = G_define_standard_option(G_OPT_R_MAP);
    params->mask_map->multiple = YES;
    params->mask_map->required = NO;
    params->mask_map->description = _("Name of raster map(s) for mask");
    params->mask_map->guisection = _("Surface");
    params->mask_map->key = "mask_map";

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

    /* vector */
    params->vector = G_define_standard_option(G_OPT_V_MAP);
    params->vector->multiple = YES;
    params->vector->required = NO;
    params->vector->description = _("Name of vector overlay map(s)");
    params->vector->guisection = _("Vector");
    params->vector->key = "vector";

    /* misc */
    params->exag = G_define_option();
    params->exag->key = "zexag";
    params->exag->key_desc = "value";
    params->exag->type = TYPE_DOUBLE;
    params->exag->required = NO;
    params->exag->multiple = NO;
    params->exag->description = _("Vertical exaggeration");

    params->bgcolor = G_define_standard_option(G_OPT_C_BG);

    /* viewpoint */
    params->pos = G_define_option();
    params->pos->key = "position";
    params->pos->key_desc = "x,y";
    params->pos->type = TYPE_DOUBLE;
    params->pos->required = NO;
    params->pos->multiple = NO;
    params->pos->description = _("Viewpoint position (x,y model coordinates)");
    params->pos->guisection = _("Viewpoint");
    params->pos->answer = "0.85,0.85";

    params->height = G_define_option();
    params->height->key = "height";
    params->height->key_desc = "value";
    params->height->type = TYPE_INTEGER;
    params->height->required = NO;
    params->height->multiple = NO;
    params->height->description = _("Viewpoint height (in map units)");
    params->height->guisection = _("Viewpoint");

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

    /* image */
    params->output = G_define_standard_option(G_OPT_F_OUTPUT);
    params->output->description = _("Name for output file (do not add extension)");
    params->output->guisection = _("Image");

    params->format = G_define_option();
    params->format->key = "format";
    params->format->type = TYPE_STRING;
    params->format->options = "ppm,tif"; /* TODO: png */
    params->format->answer = "ppm";
    params->format->description = _("Graphics file format");
    params->format->required = YES;
    params->format->guisection = _("Image");

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
