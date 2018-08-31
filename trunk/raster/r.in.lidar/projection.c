/*
 * r.in.lidar metadata-related functions
 *
 * Copyright 2011-2015 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to standalone functions)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>

#include "local_proto.h"


void projection_mismatch_report(struct Cell_head cellhd,
                                struct Cell_head loc_wind,
                                struct Key_Value *loc_proj_info,
                                struct Key_Value *loc_proj_units,
                                struct Key_Value *proj_info,
                                struct Key_Value *proj_units, int err)
{
    int i_value;
    char error_msg[8192];

    strcpy(error_msg,
           _("Projection of dataset does not"
             " appear to match current location.\n\n"));

    /* TODO: output this info sorted by key: */
    if (loc_wind.proj != cellhd.proj || err != -2) {
        if (loc_proj_info != NULL) {
            strcat(error_msg, _("GRASS LOCATION PROJ_INFO is:\n"));
            for (i_value = 0; i_value < loc_proj_info->nitems; i_value++)
                sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                        loc_proj_info->key[i_value],
                        loc_proj_info->value[i_value]);
            strcat(error_msg, "\n");
        }

        if (proj_info != NULL) {
            strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
            for (i_value = 0; i_value < proj_info->nitems; i_value++)
                sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                        proj_info->key[i_value], proj_info->value[i_value]);
        }
        else {
            strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
            if (cellhd.proj == PROJECTION_XY)
                sprintf(error_msg + strlen(error_msg),
                        "Dataset proj = %d (unreferenced/unknown)\n",
                        cellhd.proj);
            else if (cellhd.proj == PROJECTION_LL)
                sprintf(error_msg + strlen(error_msg),
                        "Dataset proj = %d (lat/long)\n", cellhd.proj);
            else if (cellhd.proj == PROJECTION_UTM)
                sprintf(error_msg + strlen(error_msg),
                        "Dataset proj = %d (UTM), zone = %d\n",
                        cellhd.proj, cellhd.zone);
            else
                sprintf(error_msg + strlen(error_msg),
                        "Dataset proj = %d (unknown), zone = %d\n",
                        cellhd.proj, cellhd.zone);
        }
    }
    else {
        if (loc_proj_units != NULL) {
            strcat(error_msg, "GRASS LOCATION PROJ_UNITS is:\n");
            for (i_value = 0; i_value < loc_proj_units->nitems; i_value++)
                sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                        loc_proj_units->key[i_value],
                        loc_proj_units->value[i_value]);
            strcat(error_msg, "\n");
        }

        if (proj_units != NULL) {
            strcat(error_msg, "Import dataset PROJ_UNITS is:\n");
            for (i_value = 0; i_value < proj_units->nitems; i_value++)
                sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                        proj_units->key[i_value], proj_units->value[i_value]);
        }
    }
    sprintf(error_msg + strlen(error_msg),
            _("\nIn case of no significant differences in the projection definitions,"
             " use the -o flag to ignore them and use"
             " current location definition.\n"));
    strcat(error_msg,
           _("Consider generating a new location with 'location' parameter"
             " from input data set.\n"));
    G_fatal_error("%s", error_msg);
}

void projection_check_wkt(struct Cell_head cellhd,
                          struct Cell_head loc_wind,
                          const char *projstr, int override, int verbose)
{
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    struct Key_Value *proj_info, *proj_units;
    int err = 0;

    proj_info = NULL;
    proj_units = NULL;

    /* Projection only required for checking so convert non-interactively */
    if (GPJ_wkt_to_grass(&cellhd, &proj_info, &proj_units, projstr, 0) < 0)
        G_warning(_("Unable to convert input map projection information to "
                    "GRASS format for checking"));

    /* Does the projection of the current location match the dataset? */

    /* fetch LOCATION PROJ info */
    if (loc_wind.proj != PROJECTION_XY) {
        loc_proj_info = G_get_projinfo();
        loc_proj_units = G_get_projunits();
    }

    if (override) {
        cellhd.proj = loc_wind.proj;
        cellhd.zone = loc_wind.zone;
        if (verbose)
            G_message(_("Over-riding projection check"));
    }
    else if (loc_wind.proj != cellhd.proj
             || (err =
                 G_compare_projections(loc_proj_info, loc_proj_units,
                                       proj_info, proj_units)) != TRUE) {
        projection_mismatch_report(cellhd, loc_wind, loc_proj_info,
                                   loc_proj_units,
                                   proj_info, proj_units, err);
    }
    else if (verbose) {
        G_message(_("Projection of input dataset and current location "
                    "appear to match"));
    }
}
