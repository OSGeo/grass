
/**************************************************************
 * Rast_put_cell_title (name, title)
 *   char *name        name of map file
 *   char *title       new title
 *
 *   changes the title for the cell file 'name' in  current mapset
 *
 *   returns  1 if ok, -1 if error
 *************************************************************/

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int Rast_put_cell_title(const char *name, const char *title)
{
    const char *mapset;
    FILE *in, *out;
    char *tempfile;
    int line;
    char buf[1024];

    mapset = G_mapset();
    in = out = 0;
    in = G_fopen_old("cats", name, mapset);
    if (!in) {
	G_warning(_("category information for [%s] in [%s]"
		    " missing or invalid"), name, mapset);
	return -1;
    }

    tempfile = G_tempfile();
    out = fopen(tempfile, "w");
    if (!out) {
	fclose(in);
	G_warning(_("G_put_title - can't create a temp file"));
	return -1;
    }

    for (line = 0; G_getl(buf, sizeof buf, in); line++) {
	if (line == 1) {
	    strcpy(buf, title);
	    G_strip(buf);
	}
	fprintf(out, "%s\n", buf);
    }
    fclose(in);
    fclose(out);

    /* must be #cats line, title line, and label for cat 0 */
    if (line < 3) {
	G_warning(_("category information for [%s] in [%s] invalid"),
		  name, mapset);
	return -1;
    }

    in = fopen(tempfile, "r");
    if (!in) {
	G_warning(_("G_put_title - can't reopen temp file"));
	return -1;
    }

    out = G_fopen_new("cats", name);
    if (!out) {
	fclose(in);
	G_warning(_("can't write category information for [%s] in [%s]"),
		  name, mapset);
	return -1;
    }

    while (fgets(buf, sizeof buf, in))
	fprintf(out, "%s", buf);

    fclose(in);
    fclose(out);
    remove(tempfile);

    return 1;
}
