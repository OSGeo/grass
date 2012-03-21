#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static char *get_option_pg(char **, const char *);

void make_link(const char *dsn,
	       const char *format,
	       char *option_str, char **options)
{
    int use_ogr;
    char *filename, *pg_schema, *pg_fid, *pg_geom_name;
    FILE *fp;
    
    struct Key_Value *key_val;
    
    key_val = G_create_key_value();

    /* use OGR ? */
    if (strcmp(format, "PostGIS") == 0) {
	use_ogr  = FALSE;
	filename = "PG";
	G_remove("", "OGR");
    }
    else {
	use_ogr  = TRUE;
	filename = "OGR";
	G_remove("", "PG");
    }
    
    /* parse options for PG data format */
    pg_schema = pg_fid = pg_geom_name = NULL;
    if (options && *options && !use_ogr) {
	pg_schema    = get_option_pg(options, "schema");
	pg_fid       = get_option_pg(options, "fid");
	pg_geom_name = get_option_pg(options, "geometry_name");
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
    }
    
    /* save file - OGR or PG */
    fp = G_fopen_new("", filename);
    if (!fp)
	G_fatal_error(_("Unable to create %s file"), filename);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing %s file"), filename);

    fclose(fp);
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
