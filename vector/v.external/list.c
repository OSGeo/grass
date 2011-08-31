#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "ogr_api.h"
#include "local_proto.h"

void list_formats(FILE *fd) {
    int i;
    
    OGRSFDriverH Ogr_driver;
    
    G_message(_("Supported OGR formats for reading:"));
    for (i = 0; i < OGRGetDriverCount(); i++) {
	Ogr_driver = OGRGetDriver(i);
	fprintf(fd, "%s\n", OGR_Dr_GetName(Ogr_driver));
    }
}

int list_layers(FILE *fd, const char *dsn, const char *layer, int print_types, int *is3D)
{
    int i, ret;
    int nlayers;
    char *layer_name;
    
    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRwkbGeometryType Ogr_geom_type;
    
    ret = -1;
    
    /* open OGR DSN */
    Ogr_ds = OGROpen(dsn, FALSE, NULL);
    if (!Ogr_ds) {
	G_fatal_error(_("Unable to open data source '%s'"), dsn);
    }

    /* Make a list of available layers */
    nlayers = OGR_DS_GetLayerCount(Ogr_ds);

    if (fd)
	G_message(_("Data source <%s> (format '%s') contains %d layers:"),
		  dsn, OGR_Dr_GetName(OGR_DS_GetDriver(Ogr_ds)), nlayers);

    for (i = 0; i < nlayers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	Ogr_geom_type = OGR_FD_GetGeomType(Ogr_featuredefn);
	layer_name = (char *) OGR_FD_GetName(Ogr_featuredefn);

	if (fd) {
	    if (print_types)
		fprintf(fd, "%s (%s)\n", layer_name, OGRGeometryTypeToName(Ogr_geom_type));
	    else
		fprintf(fd, "%s\n", layer_name);
	}
	if (layer)
	    if (strcmp(layer_name, layer) == 0) {
		if (is3D) {
		    switch(Ogr_geom_type) {
		    case wkbPoint25D: case wkbLineString25D: case wkbPolygon25D:
		    case wkbMultiPoint25D: case wkbMultiLineString25D: case wkbMultiPolygon25D:
		    case wkbGeometryCollection25D:
			*is3D = WITH_Z;
			break;
		    default:
			*is3D = WITHOUT_Z;
			break;
		    }
		}
		ret = i;
	    }
    }

    OGR_DS_Destroy(Ogr_ds);

    return ret;
}
