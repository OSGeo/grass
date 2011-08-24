#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

void print_status(int shell)
{
    FILE *fp;
    struct Key_Value *key_val;
    const char *p;

    if (!G_find_file2("", "OGR", G_mapset())) {
	if (shell)
	    fprintf(stdout, "format=%s\n", "native");
	else
	    fprintf(stdout, _("format: native\n"));
	return;
    }

    fp = G_fopen_old("", "OGR", G_mapset());
    if (!fp)
	G_fatal_error(_("Unable to open OGR file"));
    key_val = G_fread_key_value(fp);
    fclose(fp);

    p = G_find_key_value("dsn", key_val);
    if (shell)
	fprintf(stdout, "dsn=%s\n",
		p ? p : "ogr");
    else 
	fprintf(stdout, _("dsn: %s\n"),
		p ? p : _("not set (default 'ogr')"));

    p = G_find_key_value("format", key_val);
    if (shell)
	fprintf(stdout, "format=%s\n",
		p ? p : "ESRI_Shapefile");
    else
	fprintf(stdout, _("format: %s\n"),
		p ? p : _("not set (default ESRI_Shapefile)"));
    
    p = G_find_key_value("options", key_val);
    if (shell)
	fprintf(stdout, "options=%s\n", p ? p : "");
    else
	fprintf(stdout, _("options: %s\n"), p ? p : _("<none>"));
    
    G_free_key_value(key_val);
}
