#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static char *get_option_pg(char **, const char *);
static void check_option_on_off(const char *, char **);

void make_link(const char *dsn_opt,
	       const char *format,
	       char *option_str, char **options)
{
    int use_ogr;
    char *filename, *pg_schema, *pg_fid, *pg_geom_name, *dsn;
    char *pg_spatial_index, *pg_primary_key, *pg_topo, *pg_sf;
    FILE *fp;
    
    struct Key_Value *key_val;
    
    key_val = G_create_key_value();

    /* check for weird options */
    if (G_strncasecmp(dsn_opt, "PG:", 3) == 0 &&
        strcmp(format, "PostgreSQL") != 0)
        G_warning(_("Data source starts with \"PG:\" prefix, expecting \"PostgreSQL\" "
                    "format (\"%s\" given)"), format);
    
    /* use OGR ? */
    if (strcmp(format, "PostgreSQL") == 0) {
#if defined HAVE_OGR && defined HAVE_POSTGRES
      if (getenv("GRASS_VECTOR_OGR")) {
	  use_ogr  = TRUE;
	  filename = "OGR";
	  G_remove("", "PG");
      }
      else {
	  use_ogr  = FALSE;
	  filename = "PG";
	  G_remove("", "OGR");
	  
      }
#else
#ifdef HAVE_POSTGRES
      if (getenv("GRASS_VECTOR_OGR"))
	  G_warning(_("Environment variable GRASS_VECTOR_OGR defined, "
		      "but GRASS is compiled with OGR support. "
		      "Using GRASS-PostGIS data driver instead."));
      use_ogr  = FALSE;
      filename = "PG";
      G_remove("", "OGR");
#else /* -> force using OGR */
      G_warning(_("GRASS is not compiled with PostgreSQL support. "
		  "Using OGR-PostgreSQL driver instead of native "
		  "GRASS-PostGIS data driver."));
      use_ogr  = TRUE;
      filename = "OGR";
      G_remove("", "PG");
#endif /* HAVE_POSTRES */
#endif /* HAVE_OGR && HAVE_POSTGRES */
    } /* format=PostgreSQL */
    else {
	use_ogr  = TRUE;
	filename = "OGR";
	G_remove("", "PG");
    }

    /* be friendly, ignored 'PG:' prefix for GRASS-PostGIS data driver */
    if (!use_ogr && strcmp(format, "PostgreSQL") == 0 &&
	G_strncasecmp(dsn_opt, "PG:", 3) == 0) {
	int i, length;
	
	length = strlen(dsn_opt);
	dsn = (char *) G_malloc(length - 3);
	for (i = 3; i < length; i++)
	    dsn[i-3] = dsn_opt[i];
	dsn[length-3] = '\0';
    }
    else {
	dsn = G_store(dsn_opt);
    }
        
    /* parse options for PG data format */
    pg_schema = pg_fid = pg_geom_name = NULL;
    pg_spatial_index = pg_primary_key = pg_topo = NULL;
    if (options && *options && !use_ogr) {
	pg_schema    = get_option_pg(options, "schema");
	pg_fid       = get_option_pg(options, "fid");
	pg_geom_name = get_option_pg(options, "geometry_name");
	pg_spatial_index = get_option_pg(options, "spatial_index");
	if (pg_spatial_index) {
	    check_option_on_off("spatial_index", &pg_spatial_index);
	}
	pg_primary_key = get_option_pg(options, "primary_key");
	if (pg_primary_key) {
	    check_option_on_off("primary_key", &pg_primary_key);
	}
        pg_topo = get_option_pg(options, "topology");
        if (pg_topo) {
	    check_option_on_off("topology", &pg_topo);
	}
        pg_sf = get_option_pg(options, "simple_feature");
        if (pg_sf) {
	    check_option_on_off("simple_feature", &pg_topo);
	}
    }
    /* add key/value items */
    if (dsn) {
	if (use_ogr)
	    G_set_key_value("dsn", dsn, key_val);
	else
	    G_set_key_value("conninfo", dsn, key_val);
    }
    
    if (format && use_ogr)
	G_set_key_value("format", format, key_val);
    
    if (use_ogr && option_str)
	G_set_key_value("options", option_str, key_val);
    
    if (!use_ogr) {
	if (pg_schema)
	    G_set_key_value("schema", pg_schema, key_val);
	if (pg_fid)
	    G_set_key_value("fid", pg_fid, key_val);
	if (pg_geom_name)
	    G_set_key_value("geometry_name", pg_geom_name, key_val);
	if (pg_spatial_index)
	    G_set_key_value("spatial_index", pg_spatial_index, key_val);
	if (pg_primary_key)
	    G_set_key_value("primary_key", pg_primary_key, key_val);
	if (pg_topo)
	    G_set_key_value("topology", pg_topo, key_val);
	if (pg_sf)
	    G_set_key_value("simple_feature", pg_topo, key_val);
    }
    
    /* save file - OGR or PG */
    fp = G_fopen_new("", filename);
    if (!fp)
	G_fatal_error(_("Unable to create <%s> file"), filename);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing <%s> file"), filename);

    fclose(fp);

    if (use_ogr)
        G_verbose_message(_("Switched to OGR format (%s)"),
                          G_find_key_value("format", key_val));
    else
        G_verbose_message(_("Switched to PostGIS format"));

    G_free_key_value(key_val);
}

char *get_option_pg(char **options, const char *key)
{
    int   i, opt_len, key_len;
    char *opt, *value;
    
    key_len = strlen(key);
    /* parse options for PG data provider*/
    opt = value = NULL;
    for (i = 0; options[i] && !opt; i++) {
	if (G_strncasecmp(key, options[i], key_len) == 0)
	    opt = options[i];
    }
	
    if (!opt)
	return NULL;
    
    opt_len = strlen(opt);
    value = G_malloc(opt_len - key_len);
    key_len++;
    for (i = key_len; i < opt_len; i++) {
	value[i - key_len] = opt[i];
    }
    value[opt_len - key_len] = '\0';

    return value;
}

void check_option_on_off(const char *key, char **value)
{
    if(G_strcasecmp(*value, "yes") != 0 &&
       G_strcasecmp(*value, "no") != 0) {
	G_warning(_("Invalid option '%s=%s' ignored (allowed values: '%s', '%s')"),
		  key, *value, "YES", "NO");
	G_free(*value);
	*value = NULL;
    }
}
