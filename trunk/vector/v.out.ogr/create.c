#include <grass/glocale.h>

#include "local_proto.h"

void create_ogr_layer(const char *dsn, const char *format, const char *layer,
		      unsigned int wkbtype, char **papszDSCO,
		      char **papszLCO)
{
    char *pszDriverName;
    dr_t hDriver;
    ds_t hDS;
    OGRLayerH hLayer;

    pszDriverName = G_store(format);
    G_strchg(pszDriverName, '_', ' ');	/* '_' -> ' ' */

    /* start driver */
    hDriver = get_driver_by_name(pszDriverName);
    if (hDriver == NULL) {
	G_fatal_error(_("OGR driver <%s> not available"), pszDriverName);
    }

    /* create datasource */
#if GDAL_VERSION_NUM >= 2020000
    hDS = GDALCreate(hDriver, dsn, 0, 0, 0, GDT_Unknown, papszDSCO);
#else
    hDS = OGR_Dr_CreateDataSource(hDriver, dsn, papszDSCO);
#endif
    if (hDS == NULL) {
	G_fatal_error(_("Creation of output OGR datasource <%s> failed"),
		      dsn);
    }

    G_free(pszDriverName);

    /* create layer */
    /* todo: SRS */
#if GDAL_VERSION_NUM >= 2020000
    hLayer = GDALDatasetCreateLayer(hDS, layer, NULL, wkbtype, papszLCO);
#else
    hLayer = OGR_DS_CreateLayer(hDS, layer, NULL, wkbtype, papszLCO);
#endif
    if (hLayer == NULL) {
	G_fatal_error(_("Creation of OGR layer <%s> failed"), layer);
    }

    ds_close(hDS);
}

OGRwkbGeometryType get_multi_wkbtype(OGRwkbGeometryType wkbtype)
{
    OGRwkbGeometryType multiwkbtype;

    switch (wkbtype) {
    case wkbPoint:
        multiwkbtype = wkbMultiPoint;
        break;
    case wkbLineString:
        multiwkbtype = wkbMultiLineString;
        break;
    case wkbPolygon:
        multiwkbtype = wkbMultiPolygon;
        break;
    default:
        multiwkbtype = wkbGeometryCollection;
        break;
    }

    return multiwkbtype;
}

OGRwkbGeometryType get_wkbtype(int type, int otype)
{
    if (type == GV_POINT || type == GV_KERNEL ||
        (type == GV_CENTROID && otype & GV_CENTROID))
        return wkbPoint;
    else if (type & GV_LINES)
        return wkbLineString;
    else if (type == GV_FACE)
        return wkbPolygon25D;

    return wkbGeometryCollection;
}
