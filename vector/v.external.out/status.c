#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

void print_status(void)
{
    FILE *fp;
    struct Key_Value *key_val;
    const char *p;

    if (!G_find_file2("", "OGR", G_mapset())) {
	fprintf(stdout, _("Not using OGR\n"));
	return;
    }

    fp = G_fopen_old("", "OGR", G_mapset());
    if (!fp)
	G_fatal_error(_("Unable to open OGR file"));
    key_val = G_fread_key_value(fp);
    fclose(fp);

    p = G_find_key_value("directory", key_val);
    fprintf(stdout, _("directory: %s\n"),
	    p ? p : _("not set (default 'ogr')"));

    p = G_find_key_value("extension", key_val);
    fprintf(stdout, _("extension: %s\n"), p ? p : _("<none>"));

    p = G_find_key_value("format", key_val);
    fprintf(stdout, _("format: %s\n"),
	    p ? p : _("not set (default ESRI_Shapefile)"));

    p = G_find_key_value("options", key_val);
    fprintf(stdout, _("options: %s\n"), p ? p : _("<none>"));

    G_free_key_value(key_val);
}
