#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif
#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif
#include "local_proto.h"

static int cmp(const void *, const void *);
static char **format_list(int *, size_t *);
static char *feature_type(const char *);
#ifdef HAVE_OGR
static int list_layers_ogr(FILE *, const char *, const char *, int);
#endif /* HAVE_OGR */
#ifdef HAVE_POSTGRES
static int list_layers_pg(FILE *, const char *, const char *, int);
#endif /* HAVE_POSTGRES */

int cmp(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

char **format_list(int *count, size_t *len)
{
    int i;
    char **list;

    list = NULL;
    *count = 0;
    if (len)
	*len = 0;

#ifdef HAVE_OGR
    char buf[2000];
    
    OGRSFDriverH Ogr_driver;
    
    /* Open OGR DSN */
    OGRRegisterAll();
    G_debug(2, "driver count = %d", OGRGetDriverCount());
    for (i = 0; i < OGRGetDriverCount(); i++) {
	Ogr_driver = OGRGetDriver(i);
	G_debug(2, "driver %d/%d : %s", i, OGRGetDriverCount(),
		OGR_Dr_GetName(Ogr_driver));
	
	list = G_realloc(list, ((*count) + 1) * sizeof(char *));

	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", OGR_Dr_GetName(Ogr_driver));
	G_strchg(buf, ' ', '_');
	list[(*count)++] = G_store(buf);
	if (len)
	    *len += strlen(buf) + 1; /* + ',' */
    }

    /* order formats by name */
    qsort(list, *count, sizeof(char *), cmp);
#endif
#if defined HAVE_POSTGRES && !defined HAVE_OGR
    list = G_realloc(list, ((*count) + 1) * sizeof(char *));
    list[(*count)++] = G_store("PostgreSQL");
    if (len)
	*len += strlen("PostgreSQL") + 1;
#endif 

    return list;
}

char *feature_type(const char *ftype)
{
    char *ftype_ret;

    ftype_ret = G_str_replace(ftype, " ", "");
    G_str_to_lower(ftype_ret);

    /* let's OS to release the memory */
    return ftype_ret;
}

void list_formats(void)
{
    int i, count;
    char **list;
    
    G_message(_("List of supported formats:"));

    list = format_list(&count, NULL);
    
    for (i = 0; i < count; i++)
	fprintf(stdout, "%s\n", list[i]);
    fflush(stdout);

    G_free(list);
}

int list_layers(FILE *fd, const char *dsn, const char *layer, int print_types, int use_ogr)
{
    if (!use_ogr) {
#ifdef HAVE_POSTGRES
	return list_layers_pg(fd, dsn, layer, print_types);
#else
	G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif
    }
#ifdef HAVE_OGR
    return list_layers_ogr(fd, dsn, layer, print_types);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
#endif

    return -1;
}

void get_table_name(const char *table, char **table_name, char **schema_name)
{
    char **tokens;

    tokens = G_tokenize(table, ".");

    if (G_number_of_tokens(tokens) > 1) {
	*schema_name = G_store(tokens[0]);
	*table_name  = G_store(tokens[1]);
    }
    else {
	*schema_name = NULL;
	*table_name  = G_store(tokens[0]);
    }
    
    G_free_tokens(tokens);
}

#ifdef HAVE_POSTGRES
int list_layers_pg(FILE *fd, const char *conninfo, const char *table, int print_types)
{
    int   row, ntables, ret, print_schema;
    char *value_schema, *value_table;
    char *schema_name, *table_name;
    
    PGconn   *conn;
    PGresult *res;
    
    dbString sql;
    
    ret = -1;
    
    conn = PQconnectdb(conninfo);
    G_debug(1, "PQconnectdb(): %s", conninfo);
    if (PQstatus(conn) == CONNECTION_BAD)
	G_fatal_error("%s\n%s", _("Connection to PostgreSQL database failed."), 
		      PQerrorMessage(conn));
    
    db_init_string(&sql);
    db_set_string(&sql, "SELECT f_table_schema, f_table_name, type "
		  "FROM geometry_columns ORDER BY "
		  "f_table_schema, f_table_name");
    G_debug(2, "SQL: %s", db_get_string(&sql));
    res = PQexec(conn, db_get_string(&sql));
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
	G_fatal_error("%s\n%s", _("No feature tables found in database."),
		      PQresultErrorMessage(res));

    /* get schema & table_name */
    table_name = schema_name = NULL;
    if (table)
	get_table_name(table, &table_name, &schema_name);
    
    ntables = PQntuples(res);
    G_debug(3, "   nrows = %d", ntables);
    if (fd)
	G_message(_("PostGIS database <%s> contains %d feature table(s):"),
		  PQdb(conn), ntables);
    
    /* report also schemas */
    print_schema = FALSE;
    if (fd) {
	for (row = 0; row < ntables; row++) {
	    value_schema = PQgetvalue(res, row, 0);
	    if (G_strcasecmp(value_schema, "public") != 0) {
		print_schema = TRUE;
		break;
	    }
	}
    }
    
    /* report layers */
    for (row = 0; row < ntables; row++) {	
	value_schema = PQgetvalue(res, row, 0);
	value_table = PQgetvalue(res, row, 1);
	if (fd) {
	    if (print_types) {
		if (print_schema && G_strcasecmp(value_schema, "public") != 0)
		    fprintf(fd, "%s.%s,%s,0\n",
			    value_schema, value_table,
			    feature_type(PQgetvalue(res, row, 2)));
		else 
		    fprintf(fd, "%s,%s,0\n", value_table,
			    feature_type(PQgetvalue(res, row, 2)));
	    }
	    else {
		if (print_schema && G_strcasecmp(value_schema, "public") != 0)
		    fprintf(fd, "%s.%s\n", value_schema, value_table);
		else
		    fprintf(fd, "%s\n", value_table);
	    }
	}
	if ((!schema_name || strcmp(value_schema, schema_name) == 0) &&
            table_name && strcmp(value_table, table_name) == 0) {
	    ret = row;
	}
    }
    
    if (table_name)
	G_free(table_name);
    if (schema_name)
	G_free(schema_name);
    
    PQclear(res);
    PQfinish(conn);
    G_debug(1, "PQfinish()");
    
    return ret;
}
#endif /* HAVE_POSTGRES */

#ifdef HAVE_OGR
int list_layers_ogr(FILE *fd, const char *dsn, const char *layer, int print_types)
{
    int i, ret;
    int nlayers;
    char *layer_name;
    
    struct Key_Value *loc_proj_info, *loc_proj_units;
    struct Key_Value *proj_info, *proj_units;
    struct Cell_head loc_wind;

    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRwkbGeometryType Ogr_geom_type;
    
    ret = -1;
    loc_proj_info = loc_proj_units = NULL;
    proj_info = proj_units = NULL;

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
    

    G_get_default_window(&loc_wind);
    if (print_types && loc_wind.proj != PROJECTION_XY) {
	loc_proj_info = G_get_projinfo();
	loc_proj_units = G_get_projunits();
    }
    
    for (i = 0; i < nlayers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	Ogr_geom_type = OGR_FD_GetGeomType(Ogr_featuredefn);
	layer_name = (char *) OGR_FD_GetName(Ogr_featuredefn);

	if (fd) {
	    if (print_types) {
		int proj_same;
		OGRSpatialReferenceH Ogr_projection;

		/* projection check */
		Ogr_projection = OGR_L_GetSpatialRef(Ogr_layer);
		proj_same = 0;
		G_suppress_warnings(TRUE);
		if (GPJ_osr_to_grass(&loc_wind, &proj_info,
				     &proj_units, Ogr_projection, 0) < 0) {
		    G_warning(_("Unable to convert input map projection to GRASS "
				"format. Projection check cannot be provided for "
				"OGR layer <%s>"), layer_name);
		}
		else {
		    if (TRUE == G_compare_projections(loc_proj_info, loc_proj_units,
						      proj_info, proj_units))
			proj_same = 1;
		    else
			proj_same = 0;
		}
		G_suppress_warnings(FALSE);
		fprintf(fd, "%s,%s,%d\n", layer_name,
			feature_type(OGRGeometryTypeToName(Ogr_geom_type)),
			proj_same);
	    }
	    else {
		fprintf(fd, "%s\n", layer_name);
	    }
	}
	if (layer)
	    if (strcmp(layer_name, layer) == 0) {
		ret = i;
	    }
    }

    OGR_DS_Destroy(Ogr_ds);

    return ret;
}
#endif /* HAVE_OGR */
