#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int print_status_file(const char *, int);
static void print_key_value(const char *, const char *, int);
static void check_required_options(struct Key_Value *, int);

void print_status(int shell)
{
    /* try to print OGR first then PG if needed */
    if (print_status_file("OGR", shell))
	return;
    
    if (print_status_file("PG", shell))
	return;

    if (shell)
	fprintf(stdout, "format=%s\n", "native");
    else
	fprintf(stdout, _("format: native\n"));
}

void print_key_value(const char *key, const char *value, int shell)
{
    if (!value && !shell)
	return;
    
    if (shell)
	fprintf(stdout, "%s=%s\n", key, value ? value : "");
    else 
	fprintf(stdout, "%s: %s\n", key, value);
}

int print_status_file(const char *file, int shell)
{
    int i;
    FILE *fp;

    struct Key_Value *key_val;
    
    fp = G_fopen_old("", file, G_mapset());
    if (!fp)
	return FALSE;
    
    key_val = G_fread_key_value(fp);
    fclose(fp);

    /* check required options */
    check_required_options(key_val, strcmp(file, "OGR") == 0);
     
    /* print all options */
    for (i = 0; i < key_val->nitems; i++)
        print_key_value(key_val->key[i], key_val->value[i], shell);
    
    G_free_key_value(key_val);

    return TRUE;
}

int save_status_file(const struct Option *file)
{
    int use_ogr;
    FILE *fp_input, *fp_output;

    struct Key_Value *key_val;
    
    /* read settings file */
    use_ogr = FALSE;
    fp_input = G_fopen_old("", "PG", G_mapset());
    if (!fp_input) {
        use_ogr = TRUE;
        fp_input = G_fopen_old("", "OGR", G_mapset());
    }
    if (!fp_input)
        G_fatal_error(_("No settings defined"));
    
    /* read settings from file */
    key_val = G_fread_key_value(fp_input);
    fclose(fp_input);
    
    /* check required options */
    check_required_options(key_val, use_ogr);

    /* open output file */
    fp_output = G_open_option_file(file);

    /* write settings to output file */
    G_fwrite_key_value(fp_output, key_val);
    
    G_close_option_file(fp_output);
    
    G_free_key_value(key_val);

    return TRUE;
}

void check_required_options(struct Key_Value *key_val, int use_ogr)
{
    const char *p;
    
    /* format (required) */
    p = G_find_key_value("format", key_val);
    if (!p)
        G_fatal_error(_("Format not defined"));
    
    /* check required options */
    if (use_ogr) { /* OGR */
	/* dsn (required) */
	p = G_find_key_value("dsn", key_val);
	if (!p)
	    G_fatal_error(_("OGR datasource (dsn) not defined"));
    }
    else {         /* PG */
        char dsn_name[GNAME_MAX];
        
	/* conninfo (required) */
	p = G_find_key_value("conninfo", key_val);
	if (!p)
	    G_fatal_error(_("PG connection info (conninfo) not defined"));

        /* add dsn for compatibility */
        sprintf(dsn_name, "PG:%s", p);
        G_set_key_value("dsn", dsn_name, key_val);
    }
}

int read_status_file(const struct Option *file)
{
    int use_ogr;
    const char *format;
    FILE *fp_input, *fp_output;

    struct Key_Value *key_val;
    
    /* read settings file */
    fp_input = G_open_option_file(file);

    /* read settings from file */
    key_val = G_fread_key_value(fp_input);
    G_close_option_file(fp_input);

    format = G_find_key_value("format", key_val);
    if (!format)
        G_fatal_error(_("Format not defined"));
    use_ogr = is_ogr(format);
    
    /* check required options */
    check_required_options(key_val, use_ogr);

    /* write settings to output file */
    fp_output = G_fopen_new("", use_ogr ? "OGR" : "PG");
    if (!fp_output)
        G_fatal_error(_("Unable to create settings file"));
    if (G_fwrite_key_value(fp_output, key_val) < 0)
        G_fatal_error(_("Error writing settings file"));
    fclose(fp_output);
    
    G_free_key_value(key_val);

    return TRUE;
}
