#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include <gdal.h>

void check_projection(struct Cell_head *cellhd, GDALDatasetH hDS, int override)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    int projcomp_error = 0;
    char error_msg[8096];

    /* Projection only required for checking so convert non-interactively */
    if (GPJ_wkt_to_grass(cellhd, &proj_info,
			 &proj_units, GDALGetProjectionRef(hDS), 0) < 0)
	G_warning(_("Unable to convert input raster map projection information to "
		    "GRASS format for checking"));
    else {
	/* -------------------------------------------------------------------- */
	/*      Does the projection of the current location match the           */
	/*      dataset?                                                        */
	/* -------------------------------------------------------------------- */
	G_get_window(&loc_wind);
	if (loc_wind.proj != PROJECTION_XY) {
	    loc_proj_info = G_get_projinfo();
	    loc_proj_units = G_get_projunits();
	}

	if (override) {
	    cellhd->proj = loc_wind.proj;
	    cellhd->zone = loc_wind.zone;
	    G_warning(_("Over-riding projection check"));
	}
	else if (loc_wind.proj != cellhd->proj ||
		 (projcomp_error = G_compare_projections(
		      loc_proj_info, loc_proj_units, proj_info, proj_units)) < 0) {
	    int i_value;

	    strcpy(error_msg,
		   _("Projection of dataset does not"
		     " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if (loc_proj_info != NULL) {
		strcat(error_msg, _("Location PROJ_INFO is:\n"));
		for (i_value = 0;
		     loc_proj_info != NULL &&
			 i_value < loc_proj_info->nitems; i_value++)
		    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
			    loc_proj_info->key[i_value],
			    loc_proj_info->value[i_value]);
		strcat(error_msg, "\n");
	    }

	    if (proj_info != NULL) {
		strcat(error_msg, _("Dataset PROJ_INFO is:\n"));
		for (i_value = 0;
		     proj_info != NULL && i_value < proj_info->nitems;
		     i_value++)
		    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
			    proj_info->key[i_value],
			    proj_info->value[i_value]);
		strcat(error_msg, "\nERROR: ");
		switch (projcomp_error) {
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
		    strcat(error_msg, "ellps\n");
		    break;
		case -5:
		    strcat(error_msg, "zone\n");
		    break;
		}
	    }
	    else {
		strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		if (cellhd->proj == PROJECTION_XY)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (unreferenced/unknown)\n",
			    cellhd->proj);
		else if (cellhd->proj == PROJECTION_LL)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (lat/long)\n", cellhd->proj);
		else if (cellhd->proj == PROJECTION_UTM)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (UTM), zone = %d\n",
			    cellhd->proj, cellhd->zone);
		else if (cellhd->proj == PROJECTION_SP)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (State Plane), zone = %d\n",
			    cellhd->proj, cellhd->zone);
		else
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (unknown), zone = %d\n",
			    cellhd->proj, cellhd->zone);
	    }
	    strcat(error_msg,
		   _("\nYou can use the -o flag to r.external to override this check and "
		     "use the location definition for the dataset.\n"));
	    strcat(error_msg,
		   _("Consider generating a new location from the input dataset using "
		     "the 'location' parameter.\n"));
	    G_fatal_error(error_msg);
	}
	else {
	    G_message(_("Projection of input dataset and current location "
			"appear to match"));
	}
    }
}
