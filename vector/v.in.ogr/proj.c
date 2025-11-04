#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include <ogr_srs_api.h>
#include <cpl_conv.h>
#include "global.h"

#ifdef HAVE_PROJ_H
#include <proj.h>
#else
#include <proj_api.h>
#endif

/* get projection info of OGR layer in GRASS format
 * return 0 on success (some non-xy SRS)
 * return 1 if no SRS available
 * return 2 if SRS available but unreadable */
int get_layer_proj(OGRLayerH Ogr_layer, OGRSpatialReferenceH *hSRS,
                   struct Cell_head *cellhd, struct Key_Value **proj_info,
                   struct Key_Value **proj_units, char **proj_srid,
                   char **proj_wkt, char *geom_col, int verbose)
{
    *hSRS = NULL;
    *proj_info = NULL;
    *proj_units = NULL;
    *proj_srid = NULL;
    *proj_wkt = NULL;

    /* Fetch input layer projection in GRASS form. */
    if (geom_col) {
        int igeom;
        OGRGeomFieldDefnH Ogr_geomdefn;
        OGRFeatureDefnH Ogr_featuredefn;

        Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
        igeom = OGR_FD_GetGeomFieldIndex(Ogr_featuredefn, geom_col);
        if (igeom < 0)
            G_fatal_error(
                _("Geometry column <%s> not found in input layer <%s>"),
                geom_col, OGR_L_GetName(Ogr_layer));
        Ogr_geomdefn = OGR_FD_GetGeomFieldDefn(Ogr_featuredefn, igeom);
        *hSRS = OGR_GFld_GetSpatialRef(Ogr_geomdefn);
    }
    else {
        *hSRS = OGR_L_GetSpatialRef(Ogr_layer);
    }

    /* verbose is used only when comparing input SRS to GRASS projection,
     * not when comparing SRS's of several input layers */
    if (GPJ_osr_to_grass(cellhd, proj_info, proj_units, *hSRS, 0) < 0) {
        /* TODO: GPJ_osr_to_grass() does not return anything < 0
         * check with GRASS 6 and GRASS 5 */
        G_warning(_("Unable to convert input layer projection information to "
                    "GRASS format for checking"));
        if (verbose && *hSRS != NULL) {
            char *wkt = NULL;

            if (OSRExportToPrettyWkt(*hSRS, &wkt, FALSE) != OGRERR_NONE) {
                G_warning(_("Can't get WKT parameter string"));
            }
            else if (wkt) {
                G_important_message(_("WKT definition:\n%s"), wkt);
            }
        }

        return 2;
    }
    /* custom checks because if in doubt GPJ_osr_to_grass() returns a
     * xy CRS */
    if (*hSRS == NULL) {
        if (verbose) {
            G_important_message(
                _("No projection information available for layer <%s>"),
                OGR_L_GetName(Ogr_layer));
        }

        return 1;
    }

    if (!OSRIsProjected(*hSRS) && !OSRIsGeographic(*hSRS)) {
        G_important_message(
            _("Projection for layer <%s> does not contain a valid CRS"),
            OGR_L_GetName(Ogr_layer));

        if (verbose) {
            char *wkt = NULL;

            if (OSRExportToPrettyWkt(*hSRS, &wkt, FALSE) != OGRERR_NONE) {
                G_important_message(_("Can't get WKT parameter string"));
            }
            else if (wkt) {
                G_important_message(_("WKT definition:\n%s"), wkt);
            }
        }

        return 2;
    }
    else {
        const char *authkey, *authname, *authcode;
#if GDAL_VERSION_NUM >= 3000000
        char **papszOptions;

        /* get WKT2 definition */
        papszOptions = G_calloc(3, sizeof(char *));
        papszOptions[0] = G_store("MULTILINE=YES");
        papszOptions[1] = G_store("FORMAT=WKT2");
        OSRExportToWktEx(*hSRS, proj_wkt, (const char **)papszOptions);
        G_free(papszOptions[0]);
        G_free(papszOptions[1]);
        G_free(papszOptions);
#else
        OSRExportToWkt(*hSRS, proj_wkt);
#endif

        if (OSRIsProjected(*hSRS))
            authkey = "PROJCS";
        else /* is geographic */
            authkey = "GEOGCS";

        authname = OSRGetAuthorityName(*hSRS, authkey);
        if (authname && *authname) {
            authcode = OSRGetAuthorityCode(*hSRS, authkey);
            if (authcode && *authcode) {
                G_asprintf(proj_srid, "%s:%s", authname, authcode);
            }
        }
    }

    return 0;
}

/* compare projections of all OGR layers
 * return 0 if all layers have the same projection
 * return 1 if layer projections differ */
int cmp_layer_srs(GDALDatasetH Ogr_ds, int nlayers, int *layers,
                  char **layer_names, char *geom_col)
{
    int layer;
    struct Key_Value *proj_info1, *proj_units1;
    char *proj_srid1, *proj_wkt1;
    struct Key_Value *proj_info2, *proj_units2;
    char *proj_srid2, *proj_wkt2;
    struct Cell_head cellhd1, cellhd2;
    OGRLayerH Ogr_layer;
    OGRSpatialReferenceH hSRS;

    if (nlayers == 1)
        return 0;

    proj_info1 = proj_units1 = NULL;
    proj_srid1 = proj_wkt1 = NULL;
    proj_info2 = proj_units2 = NULL;
    proj_srid2 = proj_wkt2 = NULL;

    G_get_window(&cellhd1);
    layer = 0;
    do {
        /* Get first SRS */
        Ogr_layer = GDALDatasetGetLayer(Ogr_ds, layers[layer]);

        if (get_layer_proj(Ogr_layer, &hSRS, &cellhd1, &proj_info1,
                           &proj_units1, &proj_srid1, &proj_wkt1, geom_col,
                           0) == 0) {
            break;
        }
        layer++;
    } while (layer < nlayers);

    if (layer == nlayers) {
        /* could not get layer proj in GRASS format for any of the layers
         * -> projections of all layers are the same, i.e. unreadable by GRASS
         */
        G_warning(_("Layer projections are unreadable"));
        if (proj_info1)
            G_free_key_value(proj_info1);
        if (proj_units1)
            G_free_key_value(proj_units1);
        if (proj_srid1)
            G_free(proj_srid1);
        if (proj_wkt1)
            CPLFree(proj_wkt1);

        return 0;
    }
    if (layer > 0) {
        /* could not get layer proj in GRASS format for at least one of the
         * layers
         * -> mix of unreadable and readable projections  */
        G_warning(_("Projection for layer <%s> is unreadable"),
                  layer_names[layer]);
        if (proj_info1)
            G_free_key_value(proj_info1);
        if (proj_units1)
            G_free_key_value(proj_units1);
        if (proj_srid1)
            G_free(proj_srid1);
        if (proj_wkt1)
            CPLFree(proj_wkt1);

        return 1;
    }

    for (layer = 1; layer < nlayers; layer++) {
        /* Get SRS of other layer(s) */
        Ogr_layer = GDALDatasetGetLayer(Ogr_ds, layers[layer]);
        G_get_window(&cellhd2);
        if (get_layer_proj(Ogr_layer, &hSRS, &cellhd2, &proj_info2,
                           &proj_units2, &proj_srid2, &proj_wkt2, geom_col,
                           0) != 0) {
            if (proj_info1)
                G_free_key_value(proj_info1);
            if (proj_units1)
                G_free_key_value(proj_units1);
            if (proj_srid1)
                G_free(proj_srid1);
            if (proj_wkt1)
                CPLFree(proj_wkt1);

            return 1;
        }

        if (cellhd1.proj != cellhd2.proj ||
            G_compare_projections(proj_info1, proj_units1, proj_info2,
                                  proj_units2) < 0) {
            if (proj_info1)
                G_free_key_value(proj_info1);
            if (proj_units1)
                G_free_key_value(proj_units1);
            if (proj_srid1)
                G_free(proj_srid1);
            if (proj_wkt1)
                CPLFree(proj_wkt1);
            if (proj_info2)
                G_free_key_value(proj_info2);
            if (proj_units2)
                G_free_key_value(proj_units2);
            if (proj_srid2)
                G_free(proj_srid2);
            if (proj_wkt2)
                CPLFree(proj_wkt2);

            G_warning(_("Projection of layer <%s> is different from "
                        "projection of layer <%s>"),
                      layer_names[layer], layer_names[layer - 1]);

            return 1;
        }
        if (proj_info2)
            G_free_key_value(proj_info2);
        if (proj_units2)
            G_free_key_value(proj_units2);
        if (proj_srid2)
            G_free(proj_srid2);
        if (proj_wkt2)
            CPLFree(proj_wkt2);
    }
    if (proj_info1)
        G_free_key_value(proj_info1);
    if (proj_units1)
        G_free_key_value(proj_units1);
    if (proj_srid1)
        G_free(proj_srid1);
    if (proj_wkt1)
        CPLFree(proj_wkt1);

    return 0;
}

/* keep in sync with r.in.gdal, r.external, v.external */
void check_projection(struct Cell_head *cellhd, GDALDatasetH hDS, int layer,
                      char *geom_col, char *outloc, int create_only,
                      int override, int check_only)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    char *wkt = NULL, *srid = NULL;
    char error_msg[8096];
    int proj_trouble;
    OGRLayerH Ogr_layer;
    OGRSpatialReferenceH hSRS;

    /* Get first layer to be imported to use for projection check */
    Ogr_layer = GDALDatasetGetLayer(hDS, layer);

    /* -------------------------------------------------------------------- */
    /*      Fetch the projection in GRASS form, SRID, and WKT.              */
    /* -------------------------------------------------------------------- */

    /* proj_trouble:
     * 0: valid srs
     * 1: no srs, default to xy
     * 2: unreadable srs, default to xy
     */

    /* Projection only required for checking so convert non-interactively */
    proj_trouble = get_layer_proj(Ogr_layer, &hSRS, cellhd, &proj_info,
                                  &proj_units, &srid, &wkt, geom_col, 1);

    /* -------------------------------------------------------------------- */
    /*      Do we need to create a new location?                            */
    /* -------------------------------------------------------------------- */
    if (outloc != NULL) {
        /* do not create a xy location because this can mean that the
         * real SRS has not been recognized or is missing */
        if (proj_trouble) {
            G_fatal_error(_("Unable to convert input map projection to GRASS "
                            "format; cannot create new project."));
        }
        else {
            if (0 != G_make_location_crs(outloc, cellhd, proj_info, proj_units,
                                         srid, wkt)) {
                G_fatal_error(_("Unable to create new project <%s>"), outloc);
            }
            G_message(_("Project <%s> created"), outloc);

            G_unset_window(); /* new location, projection, and window */
            G_get_window(cellhd);
        }

        /* If create only, clean up and exit here */
        if (create_only) {
            GDALClose(hDS);
            exit(EXIT_SUCCESS);
        }
    }
    else {
        struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
        struct Key_Value *loc_epsg = NULL;
        char *loc_wkt = NULL, *loc_srid = NULL;
        void (*msg_fn)(const char *, ...);
        OGRSpatialReferenceH hSRS_loc = NULL;
        char *papszOptions[2];

        if (check_only && override) {
            /* can't check when over-riding check */
            override = 0;
        }

        if (proj_trouble == 2) {
            strcpy(error_msg,
                   _("Unable to convert input map projection information "
                     "to GRASS format."));
            if (override) {
                msg_fn = G_warning;
            }
            else {
                msg_fn = G_fatal_error;
                GDALClose(hDS);
            }
            msg_fn(error_msg);
            if (!override) {
                exit(EXIT_FAILURE);
            }
        }

        /* ---------------------------------------------------------------- */
        /*      Does the projection of the current location match the       */
        /*      dataset?                                                    */
        /* ---------------------------------------------------------------- */
        G_get_default_window(&loc_wind);
        /* fetch LOCATION PROJ info */
        if (loc_wind.proj != PROJECTION_XY) {
            loc_proj_info = G_get_projinfo();
            loc_proj_units = G_get_projunits();
            loc_srid = G_get_projsrid();
        }

        /* get OGR spatial reference */
#if GDAL_VERSION_MAJOR >= 3 && PROJ_VERSION_MAJOR >= 6
        if (loc_srid && *loc_srid) {
            PJ *obj = NULL;

            if ((obj = proj_create(NULL, loc_srid))) {
                loc_wkt = G_store(proj_as_wkt(NULL, obj, PJ_WKT2_LATEST, NULL));

                if (loc_wkt && !*loc_wkt) {
                    G_free(loc_wkt);
                    loc_wkt = NULL;
                }
            }
        }
        if (!loc_wkt) {
            loc_wkt = G_get_projwkt();
        }
        if (loc_wkt && *loc_wkt) {
            hSRS_loc = OSRNewSpatialReference(loc_wkt);
        }
#endif
        if (!hSRS_loc) {
            loc_epsg = G_get_projepsg();
            hSRS_loc =
                GPJ_grass_to_osr2(loc_proj_info, loc_proj_units, loc_epsg);
        }

        /* ignore data axis mapping, this is handled separately */
        papszOptions[0] = G_store("IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES");
        papszOptions[1] = NULL;

        if (override) {
            cellhd->proj = loc_wind.proj;
            cellhd->zone = loc_wind.zone;
            G_message(_("Over-riding projection check"));
        }
        else if (loc_wind.proj != cellhd->proj ||
                 !OSRIsSameEx(hSRS, hSRS_loc, (const char **)papszOptions)) {

            strcpy(error_msg,
                   _("Coordinate reference system of dataset does not"
                     " appear to match current project.\n"));

            if (G_verbose() >= G_verbose_std()) {
                char *wktstr = NULL;

                CPLSetConfigOption("OSR_WKT_FORMAT", "WKT2");

                OSRExportToPrettyWkt(hSRS, &wktstr, 0);
                /* G_message and G_fatal_error destroy the pretty formatting
                 * thus use fprintf(stderr, ...) */

                if (wktstr && *wktstr) {
                    G_warning(_("Dataset CRS is:\n"));
                    fprintf(stderr, "%s\n\n", wktstr);
                    CPLFree(wktstr);
                    wktstr = NULL;
                }

                OSRExportToPrettyWkt(hSRS_loc, &wktstr, 0);

                if (wktstr && *wktstr) {
                    G_warning(_("Project CRS is:\n"));
                    fprintf(stderr, "%s\n\n", wktstr);
                    CPLFree(wktstr);
                    wktstr = NULL;
                }
            }

            if (!check_only) {
                strcat(error_msg,
                       _("\nIn case of no significant differences "
                         "in the coordinate reference system definitions,"
                         " use the -o flag to ignore them and use"
                         " current project definition.\n"));
                strcat(error_msg, _("Consider generating a new project from "
                                    "the input dataset using "
                                    "the 'project' parameter.\n"));
            }

            if (check_only)
                msg_fn = G_message;
            else
                msg_fn = G_fatal_error;
            msg_fn("%s", error_msg);
            if (check_only) {
                GDALClose(hDS);
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (check_only)
                msg_fn = G_message;
            else
                msg_fn = G_verbose_message;
            msg_fn(_("Coordinate reference system of input dataset and current "
                     "project appear to match"));

            if (check_only) {
                GDALClose(hDS);
                exit(EXIT_SUCCESS);
            }
        }
        G_free_key_value(loc_proj_units);
        G_free_key_value(loc_proj_info);
    }
    G_free_key_value(proj_units);
    G_free_key_value(proj_info);
}
