#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include <gdal.h>
#include <ogr_srs_api.h>
#include <cpl_conv.h>

#include <proj.h>

/* keep in sync with r.external, v.in.ogr, v.external */
void check_projection(struct Cell_head *cellhd, GDALDatasetH hDS, char *outloc,
                      int create_only, int override, int check_only)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    char *wkt = NULL, *srid = NULL;
    char error_msg[8096];
    int proj_trouble;
    OGRSpatialReferenceH hSRS;

    /* ---------------------------------------------------------------------- */
    /* Fetch the dataset projection as OGR SRS, in GRASS form, SRID, and WKT. */
    /* ---------------------------------------------------------------------- */

    /* get OGR SRS definition */
    hSRS = GDALGetSpatialRef(hDS);
    if (hSRS) {
        CPLSetConfigOption("OSR_WKT_FORMAT", "WKT2");
        if (OSRExportToPrettyWkt(hSRS, &wkt, FALSE) != OGRERR_NONE) {
            G_important_message(_("Can't get WKT parameter string"));
        }
    }

    /* proj_trouble:
     * 0: valid srs
     * 1: no srs, default to xy
     * 2: unreadable srs, default to xy
     */

    /* Projection only required for checking so convert non-interactively */
    proj_trouble = 0;
    if (hSRS) {
        if ((!OSRIsProjected(hSRS) && !OSRIsGeographic(hSRS))) {
            G_important_message(_("Input contains an invalid CRS."));

            /* WKT description could give a hint what's wrong */
            CPLSetConfigOption("OSR_WKT_FORMAT", "WKT2");
            if (OSRExportToPrettyWkt(hSRS, &wkt, FALSE) != OGRERR_NONE) {
                G_important_message(_("Can't get WKT parameter string"));
            }
            else if (wkt) {
                G_important_message(_("WKT definition:"));
                /* G_message et al. are stripping off whitespaces
                 * at the beginning, destroying the pretty WKT format
                 * and making it far less readable
                 * fprintf(stderr, ...) is not an option
                 * because of potential logging to file */
                G_important_message("%s", wkt);
                CPLFree(wkt);
                wkt = NULL;
            }

            proj_trouble = 2;
        }
        else {
            const char *authkey, *authname, *authcode;

            if (OSRIsProjected(hSRS))
                authkey = "PROJCS";
            else /* is geographic */
                authkey = "GEOGCS";

            authname = OSRGetAuthorityName(hSRS, authkey);
            if (authname && *authname) {
                authcode = OSRGetAuthorityCode(hSRS, authkey);
                if (authcode && *authcode) {
                    G_asprintf(&srid, "%s:%s", authname, authcode);
                }
            }
        }

        GPJ_osr_to_grass(cellhd, &proj_info, &proj_units, hSRS, 0);
    }
    else {
        G_important_message(_("No projection information available"));
        proj_trouble = 1;
    }
    if (proj_trouble) {
        cellhd->proj = PROJECTION_XY;
        cellhd->zone = 0;
    }

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
            CPLSetConfigOption("OSR_WKT_FORMAT", "WKT2");
            if (OSRExportToPrettyWkt(hSRS, &wkt, FALSE) != OGRERR_NONE) {
                G_important_message(_("Can't get WKT parameter string"));
            }

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
        int epsgcode = 0;
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
        /* fetch project CRS info */
        if (loc_wind.proj != PROJECTION_XY) {
            loc_proj_info = G_get_projinfo();
            loc_proj_units = G_get_projunits();
            loc_srid = G_get_projsrid();
            /* also get EPSG code from PROJ_EPSG for backwards compatibility */
            loc_epsg = G_get_projepsg();
        }

        /* get OGR spatial reference for current projection */
        /* 1. from SRID */
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
        /* 2. from WKT */
        if (!loc_wkt) {
            loc_wkt = G_get_projwkt();
        }
        if (loc_wkt && *loc_wkt) {
            hSRS_loc = OSRNewSpatialReference(loc_wkt);
        }
        /* 3. from EPSG */
        if (!hSRS_loc && loc_epsg) {
            const char *epsgstr = G_find_key_value("epsg", loc_epsg);

            if (epsgstr)
                epsgcode = atoi(epsgstr);

            if (epsgcode) {
                hSRS_loc = OSRNewSpatialReference(NULL);
                OSRImportFromEPSG(hSRS_loc, epsgcode);
            }
        }
        /* 4. from GRASS-native proj info */
        if (!hSRS_loc) {
            /* GPJ_grass_to_osr2 needs WKT1 format */
            CPLSetConfigOption("OSR_WKT_FORMAT", "WKT1");
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
                    G_important_message(_("Dataset CRS is:\n"));
                    /* G_message et al. are stripping off whitespaces
                     * at the beginning, destroying the pretty WKT format
                     * and making it far less readable
                     * fprintf(stderr, ...) is not an option
                     * because of potential logging to file */
                    G_important_message("%s\n\n", wktstr);
                    CPLFree(wktstr);
                    wktstr = NULL;
                }

                OSRExportToPrettyWkt(hSRS_loc, &wktstr, 0);

                if (wktstr && *wktstr) {
                    G_important_message(_("Project CRS is:\n"));
                    /* G_message et al. are stripping off whitespaces
                     * at the beginning, destroying the pretty WKT format
                     * and making it far less readable
                     * fprintf(stderr, ...) is not an option
                     * because of potential logging to file */
                    G_important_message("%s\n\n", wktstr);
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
        G_free_key_value(loc_proj_info);
        G_free_key_value(loc_proj_units);
    }
    G_free_key_value(proj_info);
    G_free_key_value(proj_units);
}
