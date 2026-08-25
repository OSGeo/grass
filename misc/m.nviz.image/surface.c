/*!
   \file surface.c

   \brief Surface subroutines

   (C) 2008, 2010 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Load raster maps/constants and set surface attributes

   \param params module parameters
   \param data nviz data
 */
int load_rasters(const struct GParams *params, nv_data *data)
{
    const char *mapset = NULL;
    char *mname = NULL;
    int i;
    int nelevs, nelev_map, nelev_const, ncolor_map, ncolor_const, nmask_map;
    int ntransp_map, ntransp_const, nshine_map, nshine_const;
    int nemit_map, nemit_const;
    int *surf_list, nsurfs;
    int id;

    double x, y, z;

    nelev_map = opt_get_num_answers(params->elev_map);
    nelev_const = opt_get_num_answers(params->elev_const);

    nelevs = nelev_const + nelev_map;
    /* topography (required) */
    for (i = 0; i < nelevs; i++) {
        /* check maps */
        if (i < nelev_map && strcmp(params->elev_map->answers[i], "")) {
            mapset = G_find_raster2(params->elev_map->answers[i], "");
            if (mapset == NULL) {
                G_fatal_error(_("Raster map <%s> not found"),
                              params->elev_map->answers[i]);
            }
            mname =
                G_fully_qualified_name(params->elev_map->answers[i], mapset);
            id = Nviz_new_map_obj(MAP_OBJ_SURF, mname, 0.0, data);
            G_free(mname);
        }
        else {
            if (i - nelev_map < nelev_const &&
                strcmp(params->elev_const->answers[i - nelev_map], "")) {
                id = Nviz_new_map_obj(
                    MAP_OBJ_SURF, NULL,
                    atof(params->elev_const->answers[i - nelev_map]), data);
            }
            else {
                G_fatal_error(_("Missing topography attribute for surface %d"),
                              i + 1);
            }
        }

        /* set position */
        if (opt_get_num_answers(params->surface_pos) != 3 * nelevs) {
            x = atof(params->surface_pos->answers[0]);
            y = atof(params->surface_pos->answers[1]);
            z = atof(params->surface_pos->answers[2]);
        }
        else {
            x = atof(params->surface_pos->answers[i * 3 + 0]);
            y = atof(params->surface_pos->answers[i * 3 + 1]);
            z = atof(params->surface_pos->answers[i * 3 + 2]);
        }

        GS_set_trans(id, x, y, z);
    }

    /* set surface attributes */
    surf_list = GS_get_surf_list(&nsurfs);

    ncolor_map = opt_get_num_answers(params->color_map);
    ncolor_const = opt_get_num_answers(params->color_const);
    nmask_map = opt_get_num_answers(params->mask_map);
    ntransp_map = opt_get_num_answers(params->transp_map);
    ntransp_const = opt_get_num_answers(params->transp_const);
    nshine_map = opt_get_num_answers(params->shine_map);
    nshine_const = opt_get_num_answers(params->shine_const);
    nemit_map = opt_get_num_answers(params->emit_map);
    nemit_const = opt_get_num_answers(params->emit_const);

    for (i = 0; i < nsurfs; i++) {
        id = surf_list[i];
        /* color */
        /* check for color map */
        if (i < ncolor_map && strcmp(params->color_map->answers[i], "")) {
            mapset = G_find_raster2(params->color_map->answers[i], "");
            if (mapset == NULL) {
                G_fatal_error(_("Raster map <%s> not found"),
                              params->color_map->answers[i]);
            }
            mname =
                G_fully_qualified_name(params->color_map->answers[i], mapset);
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT, mname, -1.0,
                          data);
            G_free(mname);
        }
        /* check for color value */
        else if (i - ncolor_map < ncolor_const &&
                 strcmp(params->color_const->answers[i - ncolor_map], "")) {
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, CONST_ATT, NULL,
                          Nviz_color_from_str(
                              params->color_const->answers[i - ncolor_map]),
                          data);
        }
        else { /* use by default elevation map for coloring */
            if (nelev_map > 0) {
                mname = G_fully_qualified_name(params->elev_map->answers[i],
                                               mapset);
                Nviz_set_attr(id, MAP_OBJ_SURF, ATT_COLOR, MAP_ATT, mname, -1.0,
                              data);
                G_verbose_message(
                    _("Color attribute not defined, using default <%s>"),
                    mname);
                G_free(mname);
            }
            else {
                G_fatal_error(_("Missing color attribute for surface %d"),
                              i + 1);
            }
        }
        /* mask */
        if (i < nmask_map && strcmp(params->mask_map->answers[i], "")) {
            mname =
                G_fully_qualified_name(params->mask_map->answers[i], mapset);
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_MASK, MAP_ATT, mname, -1.0,
                          data);
            G_free(mname);
        }

        /* transparency */
        if (i < ntransp_map && strcmp(params->transp_map->answers[i], "")) {
            mname =
                G_fully_qualified_name(params->transp_map->answers[i], mapset);
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, MAP_ATT, mname, -1.0,
                          data);
            G_free(mname);
        }
        else if (i - ntransp_map < ntransp_const &&
                 strcmp(params->transp_const->answers[i - ntransp_map], "")) {
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_TRANSP, CONST_ATT, NULL,
                          atof(params->transp_const->answers[i - ntransp_map]),
                          data);
        }

        /* shininess */
        if (i < nshine_map && strcmp(params->shine_map->answers[i], "")) {
            mname =
                G_fully_qualified_name(params->shine_map->answers[i], mapset);
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, MAP_ATT, mname, -1.0,
                          data);
            G_free(mname);
        }
        else if (i - nshine_map < nshine_const &&
                 strcmp(params->shine_const->answers[i - nshine_map], "")) {
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_SHINE, CONST_ATT, NULL,
                          atof(params->shine_const->answers[i - nshine_map]),
                          data);
        }

        /* emission */
        if (i < nemit_map && strcmp(params->emit_map->answers[i], "")) {
            mname =
                G_fully_qualified_name(params->emit_map->answers[i], mapset);
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, MAP_ATT, mname, -1.0,
                          data);
            G_free(mname);
        }
        else if (i - nemit_map < nemit_const &&
                 strcmp(params->emit_const->answers[i - nemit_map], "")) {
            Nviz_set_attr(id, MAP_OBJ_SURF, ATT_EMIT, CONST_ATT, NULL,
                          atof(params->emit_const->answers[i - nemit_map]),
                          data);
        }

        /*
           if (i > 1)
           set_default_wirecolors(data, i);
         */
    }
    G_free(surf_list);

    return nsurfs;
}

/*!
   \brief Set draw mode for loaded surfaces

   \param params module parameters
 */
void surface_set_draw_mode(const struct GParams *params)
{
    int *surf_list, nsurfs;
    int i, id, draw_mode;
    int resol_fine, resol_coarse;

    char *mode, *style, *shade, *res_fine, *res_coarse, *wire_color;

    surf_list = GS_get_surf_list(&nsurfs);

    for (i = 0; i < nsurfs; i++) {
        draw_mode = 0;
        id = surf_list[i];
        if (!GS_surf_exists(id))
            G_fatal_error(_("Surface id %d doesn't exist"), id);

        if (params->mode_all->answer) { /* use one mode for all surfaces */
            mode = params->mode->answers[0];
            style = params->style->answers[0];
            shade = params->shade->answers[0];
            res_fine = params->res_fine->answers[0];
            res_coarse = params->res_coarse->answers[0];
            wire_color = params->wire_color->answers[0];
        }
        else {
            mode = params->mode->answers[i];
            style = params->style->answers[i];
            shade = params->shade->answers[i];
            res_fine = params->res_fine->answers[i];
            res_coarse = params->res_coarse->answers[i];
            wire_color = params->wire_color->answers[i];
        }

        /* mode */
        if (strcmp(mode, "coarse") == 0) {
            draw_mode |= DM_WIRE;
        }
        else if (strcmp(mode, "fine") == 0) {
            draw_mode |= DM_POLY;
        }
        else { /* both */
            draw_mode |= DM_WIRE_POLY;
        }

        /* style */
        if (strcmp(style, "wire") == 0) {
            draw_mode |= DM_GRID_WIRE;
        }
        else { /* surface */
            draw_mode |= DM_GRID_SURF;
        }

        /* shading */
        if (strcmp(shade, "flat") == 0) {
            draw_mode |= DM_FLAT;
        }
        else { /* gouraud */
            draw_mode |= DM_GOURAUD;
        }

        if (GS_set_drawmode(id, draw_mode) < 0)
            G_fatal_error(_("Unable to set draw mode for surface id %d"), id);

        /* resolution */
        resol_fine = atoi(res_fine);
        resol_coarse = atoi(res_coarse);
        if (GS_set_drawres(id, resol_fine, resol_fine, resol_coarse,
                           resol_coarse) < 0)
            G_fatal_error(_("Unable to set draw mode for surface id %d"), id);

        /* wire color */
        GS_set_wire_color(id, Nviz_color_from_str(wire_color));
    }
    G_free(surf_list);

    return;
}
