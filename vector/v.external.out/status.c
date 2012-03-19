#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static int print_status_file(const char *, int shell);

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
	p = G_find_key_value("dsn", key_val);
	if (!p)
	    G_fatal_error(_("OGR datasource (dsn) not defined"));
	if (shell)
	    fprintf(stdout, "dsn=%s\n", p);
	else 
	    fprintf(stdout, "dsn: %s\n", p);
    
	p = G_find_key_value("format", key_val);
	if (!p)
	    G_fatal_error(_("OGR format not defined"));
	if (shell)
	    fprintf(stdout, "format=%s\n", p);
	else
	    fprintf(stdout, "format: %s\n", p);
	
	p = G_find_key_value("options", key_val);
	if (shell)
	    fprintf(stdout, "options=%s\n", p ? p : "");
	else
	    fprintf(stdout, _("options: %s\n"), p ? p : _("<none>"));
    }
    else { /* PG */
	p = G_find_key_value("conninfo", key_val);
	if (!p)
	    G_fatal_error(_("PG connection info (conninfo) not defined"));
	if (shell)
	    fprintf(stdout, "conninfo=%s\n", p);
	else 
	    fprintf(stdout, "conninfo: %s\n", p);
	p = G_find_key_value("schema", key_val);
	if (p) {
	    if (shell)
		fprintf(stdout, "schema=%s\n", p);
	    else 
		fprintf(stdout, "schema: %s\n", p);
	}
    }

    G_free_key_value(key_val);

    return TRUE;
}
