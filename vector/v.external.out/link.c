#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int parse_option_pg(const char *, char **, char **);

void make_link(const char *dsn_opt,
	       const char *format,
	       char *option_str, char **options)
{
    int use_ogr;
    char *filename, *dsn;
    FILE *fp;
    
    struct Key_Value *key_val;
    
    key_val = G_create_key_value();

    /* check for weird options */
    if (G_strncasecmp(dsn_opt, "PG:", 3) == 0 &&
        strcmp(format, "PostgreSQL") != 0)
        G_warning(_("Data source starts with \"PG:\" prefix, expecting \"PostgreSQL\" "
                    "format (\"%s\" given)"), format);
    
    /* use OGR ? */
    use_ogr = is_ogr(format);
    if (use_ogr) {
        filename = "OGR";
        G_remove("", "PG");
    }
    else {
        filename = "PG";
        G_remove("", "OGR");
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
    if (options && *options && !use_ogr) {
        int i;
        char *key, *value;
        
        i = 0;
        while (options[i]) {
            if (parse_option_pg(options[i++], &key, &value) != 0)
                continue;
            G_set_key_value(key, value, key_val);
        }
    }

    /* datasource section */
    if (dsn) {
	if (use_ogr)
	    G_set_key_value("dsn", dsn, key_val);
	else
	    G_set_key_value("conninfo", dsn, key_val);
    }
    
    if (use_ogr) { /* OGR */
        if (format)
            G_set_key_value("format", format, key_val);
        if (option_str)
            G_set_key_value("options", option_str, key_val);
    }
    else {        /* PG */
        G_set_key_value("format", "PostgreSQL", key_val);
    }
    /* save file - OGR or PG */
    fp = G_fopen_new("", filename);
    if (!fp)
	G_fatal_error(_("Unable to create settings file"));

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing settings file"));

    fclose(fp);

    if (use_ogr)
        G_verbose_message(_("Switched to OGR format (%s)"),
                          G_find_key_value("format", key_val));
    else
        G_verbose_message(_("Switched to PostGIS format"));

    G_free_key_value(key_val);
}

int parse_option_pg(const char *option, char **key, char **value)
{
    char **tokens;
    
    tokens = G_tokenize(option, "=");
    if (G_number_of_tokens(tokens) != 2) {
        G_warning(_("Unable to parse option '%s'"), option);
        return 1;
    }
    
    *key = G_store(tokens[0]);
    G_str_to_lower(*key);
    
    *value = G_store(tokens[1]);
    G_str_to_lower(*value);

    G_free_tokens(tokens);

    return 0;
}

