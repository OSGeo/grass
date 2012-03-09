#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif
#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif
#include "local_proto.h"

#ifdef HAVE_OGR
static int list_layers_ogr(FILE *, const char *, const char *, int, int *);
#endif
#ifdef HAVE_POSTGRES
static int list_layers_pg(FILE *, const char *, const char *, int, int *);
#endif

void list_formats(FILE *fd) {
#ifdef HAVE_OGR
    int i;
    OGRSFDriverH Ogr_driver;
    
    G_message(_("Supported OGR formats for reading:"));
    for (i = 0; i < OGRGetDriverCount(); i++) {
	Ogr_driver = OGRGetDriver(i);
	fprintf(fd, "%s\n", OGR_Dr_GetName(Ogr_driver));
    }
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
#endif
}

int list_layers(FILE *fd, const char *dsn, const char *layer, int print_types, int use_postgis, int *is3D)
{
    if (use_postgis) {
#ifdef HAVE_POSTGRES
	return list_layers_pg(fd, dsn, layer, print_types, is3D);
#else
	G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif
    }
#ifdef HAVE_OGR
    return list_layers_ogr(fd, dsn, layer, print_types, is3D);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
#endif

    return -1;
}

int list_layers_pg(FILE *fd, const char *conninfo, const char *table, int print_types, int *is3D)
{
#ifdef HAVE_POSTGRES
    int row, ntables, ret;
    char *value;
    PGconn *conn;
    PGresult *res;
    
    dbString sql;
    
    ret = -1;
    
    conn = PQconnectdb(conninfo);
    G_debug(1, "PQconnectdb(): %s", conninfo);
    if (PQstatus(conn) == CONNECTION_BAD)
	G_fatal_error("%s\n%s", _("Connection to PostgreSQL database failed."), 
		      PQerrorMessage(conn));
    
    db_init_string(&sql);
    db_set_string(&sql, "SELECT f_table_name, type "
		  "FROM geometry_columns");
    G_debug(2, "SQL: %s", db_get_string(&sql));
    res = PQexec(conn, db_get_string(&sql));
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
	G_fatal_error("%s\n%s", _("No feature tables found in database."),
		      PQresultErrorMessage(res));
    
    ntables = PQntuples(res);
    G_debug(3, "   nrows = %d", ntables);
    if (fd)
	G_message(_("PostGIS database <%s> contains %d feature table(s):"),
		  PQdb(conn), ntables);

    for (row = 0; row < ntables; row++) {
	value = PQgetvalue(res, row, 0);
	if (fd) {
	    if (print_types)
		fprintf(fd, "%s (%s)\n", value, PQgetvalue(res, row, 1));
	    else
		fprintf(fd, "%s\n", value);
	}
	if (table && strcmp(value, table) == 0) {
	    ret = row;
	    *is3D = WITHOUT_Z;
	}
    }
    
    PQclear(res);
    PQfinish(conn);
    G_debug(1, "PQfinish()");
    
    return ret;
#else
    G_fatal_error(_("GRASS not compiled with PostgreSQL/PostGIS support"));
#endif
}

#ifdef HAVE_OGR
int list_layers_ogr(FILE *fd, const char *dsn, const char *layer, int print_types, int *is3D)
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
#endif /* HAVE_OGR */
