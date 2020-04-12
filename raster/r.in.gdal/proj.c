#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include <gdal.h>
#include <ogr_srs_api.h>

/* keep in sync with r.external, v.in.ogr, v.external */
void check_projection(struct Cell_head *cellhd, GDALDatasetH hDS,
                      char *outloc, int create_only, int override,
		      int check_only)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info, *proj_units, *proj_epsg;
    struct Key_Value *loc_proj_info, *loc_proj_units;
    const char *wkt;
    char error_msg[8096];
    int proj_trouble;

    /* -------------------------------------------------------------------- */
    /*      Fetch the projection in GRASS form.                             */
    /* -------------------------------------------------------------------- */
    proj_info = NULL;
    proj_units = NULL;
    proj_epsg = NULL;
    loc_proj_info = NULL;
    loc_proj_units = NULL;

    wkt = GDALGetProjectionRef(hDS);
    /* proj_trouble:
     * 0: valid srs
     * 1: no srs, default to xy
     * 2: unreadable srs, default to xy
     */

    /* Projection only required for checking so convert non-interactively */
    proj_trouble = 0;
    if (wkt && *wkt) {
	OGRSpatialReferenceH hSRS;

	hSRS = OSRNewSpatialReference(wkt);
	if (hSRS != NULL)
	    GPJ_osr_to_grass(cellhd, &proj_info, &proj_units, hSRS, 0);

	if (!hSRS || (!OSRIsProjected(hSRS) && !OSRIsGeographic(hSRS))) {
	    G_important_message(_("Input contains an invalid SRS. " 
	                          "WKT definition:\n%s"), wkt);

	    proj_trouble = 2;
	}
	else{
	    const char *authkey, *authname, *authcode;

	    if (OSRIsProjected(hSRS))
		authkey = "PROJCS";
	    else /* is geographic */
		authkey = "GEOGCS";

	    authname = OSRGetAuthorityName(hSRS, authkey);
	    if (authname && *authname && strcmp(authname, "EPSG") == 0) {
		authcode = OSRGetAuthorityCode(hSRS, authkey);
		if (authcode && *authcode) {
		    G_debug(1, "found EPSG:%s", authcode);
		    proj_epsg = G_create_key_value();
		    G_set_key_value("epsg", authcode, proj_epsg);
		}
	    }
	}
	if (hSRS)
	    OSRDestroySpatialReference(hSRS);
    }
    else {
	G_important_message(_("No projection information available"));
	cellhd->proj = PROJECTION_XY;
	cellhd->zone = 0;
	proj_trouble = 1;
    }

    /* -------------------------------------------------------------------- */
    /*      Do we need to create a new location?                            */
    /* -------------------------------------------------------------------- */
    if (outloc != NULL) {
	/* do not create a xy location if an existing SRS was unreadable */ 
	if (proj_trouble == 2) {
	    G_fatal_error(_("Unable to convert input map projection to GRASS "
			    "format; cannot create new location."));
	}
	else {
            if (0 != G_make_location_epsg(outloc, cellhd, proj_info,
	                                  proj_units, proj_epsg)) {
                G_fatal_error(_("Unable to create new location <%s>"),
                              outloc);
            }
	    G_message(_("Location <%s> created"), outloc);

	    G_unset_window();	/* new location, projection, and window */
	    G_get_window(cellhd);
	}

        /* If create only, clean up and exit here */
        if (create_only) {
	    GDALClose(hDS);
            exit(EXIT_SUCCESS);
        }
    }
    else {
	int err = 0;
        void (*msg_fn)(const char *, ...);

	if (check_only && override) {
	    /* can't check when over-riding check */
	    override = 0;
	}

	if (proj_trouble == 2) {
	    strcpy(error_msg, _("Unable to convert input map projection information "
		                "to GRASS format."));
            if (override) {
                msg_fn = G_warning;
	    }
            else {
                msg_fn = G_fatal_error;
	    }
            msg_fn(error_msg);
            if (!override) {
                exit(EXIT_FAILURE);
	    }
	}

	/* -------------------------------------------------------------------- */
	/*      Does the projection of the current location match the           */
	/*      dataset?                                                        */
	/* -------------------------------------------------------------------- */
	G_get_default_window(&loc_wind);
	/* fetch LOCATION PROJ info */
	if (loc_wind.proj != PROJECTION_XY) {
	    loc_proj_info = G_get_projinfo();
	    loc_proj_units = G_get_projunits();
	}

	if (override) {
	    cellhd->proj = loc_wind.proj;
	    cellhd->zone = loc_wind.zone;
	    G_message(_("Over-riding projection check"));
	}
	else if (loc_wind.proj != cellhd->proj
		 || (err =
		     G_compare_projections(loc_proj_info, loc_proj_units,
					   proj_info, proj_units)) != 1) {
	    int i_value;

	    strcpy(error_msg,
		   _("Projection of dataset does not"
		     " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if (loc_wind.proj != cellhd->proj || err != -2) {
		/* error in proj_info */
		if (loc_proj_info != NULL) {
		    strcat(error_msg, _("Location PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < loc_proj_info->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_info->key[i_value],
				loc_proj_info->value[i_value]);
		    strcat(error_msg, "\n");
		}
		else {
		    strcat(error_msg, _("Location PROJ_INFO is:\n"));
		    if (loc_wind.proj == PROJECTION_XY)
			sprintf(error_msg + strlen(error_msg),
				"Location proj = %d (unreferenced/unknown)\n",
				loc_wind.proj);
		    else if (loc_wind.proj == PROJECTION_LL)
			sprintf(error_msg + strlen(error_msg),
				"Location proj = %d (lat/long)\n",
				loc_wind.proj);
		    else if (loc_wind.proj == PROJECTION_UTM)
			sprintf(error_msg + strlen(error_msg),
				"Location proj = %d (UTM), zone = %d\n",
				loc_wind.proj, cellhd->zone);
		    else
			sprintf(error_msg + strlen(error_msg),
				"Location proj = %d (unknown), zone = %d\n",
				loc_wind.proj, cellhd->zone);
		}

		if (proj_info != NULL) {
		    strcat(error_msg, _("Dataset PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < proj_info->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_info->key[i_value],
				proj_info->value[i_value]);
		}
		else {
		    strcat(error_msg, _("Dataset PROJ_INFO is:\n"));
		    if (cellhd->proj == PROJECTION_XY)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unreferenced/unknown)\n",
				cellhd->proj);
		    else if (cellhd->proj == PROJECTION_LL)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (lat/long)\n",
				cellhd->proj);
		    else if (cellhd->proj == PROJECTION_UTM)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (UTM), zone = %d\n",
				cellhd->proj, cellhd->zone);
		    else
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unknown), zone = %d\n",
				cellhd->proj, cellhd->zone);
		}
		if (loc_wind.proj != cellhd->proj) {
		    strcat(error_msg, "\nDifference in: proj\n");
		}
		else {
		    strcat(error_msg, "\nDifference in: ");
		    switch (err) {
		    case -1:
			strcat(error_msg, "proj\n");
			break;
		    case -2:
			strcat(error_msg, "units\n");
			break;
		    case -3:
			strcat(error_msg, "datum\n");
			break;
		    case -4:
			strcat(error_msg, "ellps, a, es\n");
			break;
		    case -5:
			strcat(error_msg, "zone\n");
			break;
		    case -6:
			strcat(error_msg, "south\n");
			break;
		    case -7:
			strcat(error_msg, "x_0\n");
			break;
		    case -8:
			strcat(error_msg, "y_0\n");
			break;
		    case -9:
			strcat(error_msg, "lon_0\n");
			break;
		    case -10:
			strcat(error_msg, "lat_0\n");
			break;
		    case -11:
			strcat(error_msg, "lat_1, lat2\n");
			break;
		    }
		}
	    }
	    else {
		/* error in proj_units */
		if (loc_proj_units != NULL) {
		    strcat(error_msg, "Location PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < loc_proj_units->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_units->key[i_value],
				loc_proj_units->value[i_value]);
		    strcat(error_msg, "\n");
		}

		if (proj_units != NULL) {
		    strcat(error_msg, "Dataset PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < proj_units->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_units->key[i_value],
				proj_units->value[i_value]);
		}
	    }
            if (!check_only) {
                strcat(error_msg,
                       _("\nIn case of no significant differences in the projection definitions,"
                         " use the -o flag to ignore them and use"
                         " current location definition.\n"));
                strcat(error_msg,
                       _("Consider generating a new location from the input dataset using "
                         "the 'location' parameter.\n"));
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
	    msg_fn(_("Projection of input dataset and current location "
		     "appear to match"));
	    if (check_only) {
		GDALClose(hDS);
		exit(EXIT_SUCCESS);
	    }
	}
    }
}
