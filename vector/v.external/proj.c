#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

void check_projection(const char *dsn, int ilayer)
{
#ifdef HAVE_OGR
    int err = 0;
    char error_msg[8192];
    
    struct Cell_head cellhd, loc_wind;
    struct Key_Value *loc_proj_info, *loc_proj_units;
    struct Key_Value *proj_info, *proj_units;
    
    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRSpatialReferenceH Ogr_projection;
    
    proj_info = proj_units = NULL;
    loc_proj_info = G_get_projinfo();
    loc_proj_units = G_get_projunits();
    
    G_get_default_window(&loc_wind);
    G_get_window(&cellhd);
    
    Ogr_ds = OGROpen(dsn, FALSE, NULL);
    if (!Ogr_ds) {
	G_fatal_error(_("Unable to open data source '%s'"), dsn);
    }
    if (ilayer > OGR_DS_GetLayerCount(Ogr_ds))
        return;
    Ogr_layer = OGR_DS_GetLayer(Ogr_ds, ilayer);
    Ogr_projection = OGR_L_GetSpatialRef(Ogr_layer);
    
    if (GPJ_osr_to_grass(&cellhd, &proj_info,
                         &proj_units, Ogr_projection, 0) < 0)
        G_warning(_("Unable to convert input map projection information to "
                    "GRASS format for checking"));
    OGR_DS_Destroy(Ogr_ds);
    
    if ((err =
         G_compare_projections(loc_proj_info, loc_proj_units,
                               proj_info, proj_units)) != TRUE) {
        int i_value;
        
        strcpy(error_msg,
               _("Projection of dataset does not"
                 " appear to match current location.\n\n"));
        
        /* TODO: output this info sorted by key: */
        if (loc_wind.proj != cellhd.proj || err != -2) {
            if (loc_proj_info != NULL) {
                strcat(error_msg, _("GRASS LOCATION PROJ_INFO is:\n"));
                for (i_value = 0; i_value < loc_proj_info->nitems;
                     i_value++)
                    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                            loc_proj_info->key[i_value],
                            loc_proj_info->value[i_value]);
                strcat(error_msg, "\n");
            }
            
            if (proj_info != NULL) {
                strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
                for (i_value = 0; i_value < proj_info->nitems; i_value++)
                    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                            proj_info->key[i_value],
                            proj_info->value[i_value]);
            }
            else {
                strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
                if (cellhd.proj == PROJECTION_XY)
                    sprintf(error_msg + strlen(error_msg),
                            "Dataset proj = %d (unreferenced/unknown)\n",
                            cellhd.proj);
                else if (cellhd.proj == PROJECTION_LL)
                    sprintf(error_msg + strlen(error_msg),
                            "Dataset proj = %d (lat/long)\n",
                            cellhd.proj);
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
                for (i_value = 0; i_value < loc_proj_units->nitems;
                     i_value++)
                    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                            loc_proj_units->key[i_value],
                            loc_proj_units->value[i_value]);
                strcat(error_msg, "\n");
            }
            
            if (proj_units != NULL) {
                strcat(error_msg, "Import dataset PROJ_UNITS is:\n");
                for (i_value = 0; i_value < proj_units->nitems; i_value++)
                    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
                            proj_units->key[i_value],
                            proj_units->value[i_value]);
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
    else {
        G_verbose_message(_("Projection of input dataset and current location "
                            "appear to match"));
    }
#endif
}
