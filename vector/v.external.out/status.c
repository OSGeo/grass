#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static int print_status_file(const char *, int);
static void print_key_value(const char *, const char *, int);

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
    if (!value)
	return;
    
    if (shell)
	fprintf(stdout, "%s=%s\n", key, value);
    else 
	fprintf(stdout, "%s: %s\n", key, value);
}

int print_status_file(const char *file, int shell)
{
    FILE *fp;
    const char *p;

    struct Key_Value *key_val;
    
    fp = G_fopen_old("", file, G_mapset());
    if (!fp)
	return FALSE;
    
    key_val = G_fread_key_value(fp);
    fclose(fp);

    if (strcmp(file, "OGR") == 0) {
	/* dsn (required) */
	p = G_find_key_value("dsn", key_val);
	if (!p)
	    G_fatal_error(_("OGR datasource (dsn) not defined"));
	print_key_value("dsn", p, shell);
    
	/* format (required) */
	p = G_find_key_value("format", key_val);
	if (!p)
	    G_fatal_error(_("OGR format not defined"));
	print_key_value("format", p, shell);
	
	/* options */
	p = G_find_key_value("options", key_val);
	print_key_value("options", p, shell);
    }
    else { /* PG */
	/* conninfo (required) */
	p = G_find_key_value("conninfo", key_val);
	if (!p)
	    G_fatal_error(_("PG connection info (conninfo) not defined"));
	print_key_value("conninfo", p, shell);

	/* schema */
	p = G_find_key_value("schema", key_val);
	print_key_value("schema", p, shell);

	/* fid */
	p = G_find_key_value("fid", key_val);
	print_key_value("fid", p, shell);

	/* geometry_name */
	p = G_find_key_value("geometry_name", key_val);
	print_key_value("geometry_name", p, shell);

	/* spatial_index */
	p = G_find_key_value("spatial_index", key_val);
	print_key_value("spatial_index", p, shell);

	/* primary_key */
	p = G_find_key_value("primary_key", key_val);
	print_key_value("primary_key", p, shell);
    }

    G_free_key_value(key_val);

    return TRUE;
}
