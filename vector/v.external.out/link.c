#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

void make_link(const char *dsn,
	       const char *format,
	       char *option_str, char **options)
{
    int i, use_ogr;
    char *filename, *pg_schema;
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
    
    /* parse options */
    pg_schema = NULL;
    if (options && *options && !use_ogr) {
	int   opt_len;
	char *opt_schema;
	
	opt_schema = NULL;
	for (i = 0; options[i]; i++) {
	    if (G_strncasecmp("schema=", options[i], 6) == 0)
		opt_schema = options[i];
	    else
		G_warning(_("Option '%s' ignored for 'PostGIS' format"),
			  options[i]);
	}
	
	if (opt_schema) {
	    opt_len = strlen(opt_schema);
	    pg_schema = G_malloc(opt_len - 6);
	    for (i = 7; i < opt_len; i++) {
		pg_schema[i - 7] = opt_schema[i];
	    }
	    pg_schema[opt_len - 7] = '\0';
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
    
    if (!use_ogr && pg_schema)
	G_set_key_value("schema", pg_schema, key_val);
    
    /* save file - OGR or PG */
    fp = G_fopen_new("", filename);
    if (!fp)
	G_fatal_error(_("Unable to create %s file"), filename);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing %s file"), filename);

    fclose(fp);
}
