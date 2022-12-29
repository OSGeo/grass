
/****************************************************************************
 *
 * MODULE:       g.rename
 * AUTHOR(S):    CERL (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Cedric Shock <cedricgrass shockfamily.net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Martin Landa <landa.martin gmail.com>,
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      Rename map names
 * COPYRIGHT:    (C) 1994-2007, 2011-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/manage.h>

void update_reclass_maps(const char *, const char *);
void update_base_map(const char *, const char *, const char *);

int main(int argc, char *argv[])
{
    int n;
    struct GModule *module;
    struct Option **parm;
    int nlist;
    const char *mapset;
    int result = EXIT_SUCCESS;

    G_gisinit(argv[0]);

    M_read_list(FALSE, &nlist);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("rename"));
    module->description =
	_("Renames data base element files in the user's current mapset.");
    module->overwrite = 1;

    parm = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	parm[n] = M_define_option(n, _("renamed"), NO);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    mapset = G_mapset();

    for (n = 0; n < nlist; n++) {
	int i;
	char *old, *new;

	if (parm[n]->answers == NULL)
	    continue;
	i = 0;
	while (parm[n]->answers[i]) {
	    int renamed;

	    old = parm[n]->answers[i++];
	    new = parm[n]->answers[i++];
	    if (!M_find(n, old, mapset)) {
		G_warning(_("%s <%s> not found"), M_get_list(n)->maindesc, old);
		continue;
	    }
	    if (M_find(n, new, mapset) && !(module->overwrite)) {
		G_warning(_("<%s> already exists in mapset <%s>"), new,
			  M_find(n, new, ""));
		continue;
	    }
	    if (G_legal_filename(new) < 0) {
		G_warning(_("<%s> is an illegal file name"), new);
		continue;
	    }
	    if (G_strcasecmp(old, new) == 0) {
		/* avoid problems on case-insensitive file systems (FAT, NTFS, ...) */
		G_warning(_("%s=%s,%s: files could be the same, no rename possible"),
			  parm[n]->key, old, new);
		continue;
	    }

	    if ((renamed = M_do_rename(n, old, new)) == 1) {
		result = EXIT_FAILURE;
	    }

	    if (!renamed && strcmp(parm[n]->key, "raster") == 0) {
		update_reclass_maps(new, mapset);
		update_base_map(old, new, mapset);
	    }
	}
    }
    exit(result);
}

void update_reclass_maps(const char *name, const char *mapset)
{
    int nrmaps;
    char **rmaps;

    if (Rast_is_reclassed_to(name, mapset, &nrmaps, &rmaps) <= 0)
	return;

    G_message(_("Updating reclass maps"));

    for (; *rmaps; rmaps++) {
	char buf1[256], buf2[256], buf3[256], *str;
	FILE *fp;
	int ptr, l;

	G_message(" %s", *rmaps);
	sprintf(buf3, "%s", *rmaps);
	if ((str = strchr(buf3, '@'))) {
	    *str = 0;
	    sprintf(buf2, "%s", str + 1);
	}
	else {
	    sprintf(buf2, "%s", mapset);
	}
	G_file_name(buf1, "cellhd", buf3, buf2);

	fp = fopen(buf1, "r");
	if (fp == NULL)
	    continue;

	fgets(buf2, 255, fp);
	fgets(buf2, 255, fp);
	fgets(buf2, 255, fp);

	ptr = G_ftell(fp);
	G_fseek(fp, 0L, SEEK_END);
	l = G_ftell(fp) - ptr;

	str = (char *)G_malloc(l);
	G_fseek(fp, ptr, SEEK_SET);
	fread(str, l, 1, fp);
	fclose(fp);

	fp = fopen(buf1, "w");
	fprintf(fp, "reclass\n");
	fprintf(fp, "name: %s\n", name);
	fprintf(fp, "mapset: %s\n", mapset);
	fwrite(str, l, 1, fp);
	G_free(str);
	fclose(fp);
    }
}

void update_base_map(const char *old, const char *new, const char *mapset)
{
    int i, nrmaps, found;
    char bname[GNAME_MAX], bmapset[GMAPSET_MAX], rpath[GPATH_MAX];
    char *xold, *xnew, **rmaps;
    FILE *fp;

    if (Rast_is_reclass(new, mapset, bname, bmapset) <= 0)
	return;

    if (Rast_is_reclassed_to(bname, bmapset, &nrmaps, &rmaps) <= 0)
	nrmaps = 0;

    found = 0;
    xold = G_fully_qualified_name(old, mapset);
    for (i = 0; i < nrmaps; i++) {
	if (strcmp(xold, rmaps[i]) == 0) {
	    found = 1;
	    break;
	}
    }

    if (!found) {
	G_fatal_error(_("Unable to find reclass information for <%s> in "
			"base map <%s@%s>"), xold, bname, bmapset);
    }

    G_message(_("Updating base map <%s@%s>"), bname, bmapset);

    G_file_name_misc(rpath, "cell_misc", "reclassed_to", bname, bmapset);

    fp = fopen(rpath, "w");
    if (fp == NULL) {
	G_fatal_error(_("Unable to update dependency file in <%s@%s>"),
		  bname, bmapset);
    }

    xnew = G_fully_qualified_name(new, mapset);
    for (; *rmaps; rmaps++) {
	if (strcmp(xold, *rmaps) == 0) {
	    fprintf(fp, "%s\n", xnew);
	}
	else {
	    fprintf(fp, "%s\n", *rmaps);
	}
    }

    G_free(xold);
    G_free(xnew);
    fclose(fp);
}
