
/****************************************************************************
 *
 * MODULE:       cmd
 * AUTHOR(S):    CERL (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      lets users remove GRASS database files
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/list.h>

static int check_reclass(const char *name, const char *mapset, int force)
{
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];
    char **rmaps;
    int nrmaps;

    if (Rast_is_reclassed_to(name, mapset, &nrmaps, &rmaps) > 0) {
	for (; *rmaps; rmaps++) {
	    /* force remove */
	    if (force)
		G_warning(_("[%s@%s] is a base map for [%s]. Remove forced."),
			  name, mapset, *rmaps);
	    else
		G_warning(_("[%s@%s] is a base map. Remove reclassed map first: %s"),
			  name, mapset, *rmaps);
	}

	if (!force)
	    return 1;
    }

    if (Rast_is_reclass(name, mapset, rname, rmapset) > 0 &&
	Rast_is_reclassed_to(rname, rmapset, &nrmaps, &rmaps) > 0) {
	char path[GPATH_MAX];
	char *p = strchr(rname, '@');
	char *qname = G_fully_qualified_name(name, mapset);

	if (p)
	    *p = '\0';

	G_file_name_misc(path, "cell_misc", "reclassed_to", rname, rmapset);

	if (nrmaps == 1 && !G_strcasecmp(rmaps[0], qname)) {

	    if (remove(path) < 0)
		G_warning(_("Removing information about reclassed map from [%s@%s] failed"),
			  rname, rmapset);
	}
	else {
	    FILE *fp = fopen(path, "w");

	    if (fp) {
		for (; *rmaps; rmaps++)
		    if (G_strcasecmp(*rmaps, qname))
			fprintf(fp, "%s\n", *rmaps);
		fclose(fp);
	    }
	    else
		G_warning(_("Removing information about reclassed map from [%s@%s] failed"),
			  rname, rmapset);

	}
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int i, n;
    struct GModule *module;
    struct Option **parm, *p;
    struct Flag *force_flag;
    const char *name, *mapset;
    const char *location_path;
    int result = EXIT_SUCCESS;
    int force = 0;

    G_gisinit(argv[0]);

    read_list(0);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    module->description =
	_("Removes data base element files from "
	  "the user's current mapset.");

    force_flag = G_define_flag();
    force_flag->key = 'f';
    force_flag->description = _("Force remove");

    parm = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	char *str;

	p = parm[n] = G_define_option();
	p->key = list[n].alias;
	p->type = TYPE_STRING;
	p->required = NO;
	p->multiple = YES;
	G_asprintf(&str, "old,%s,%s", list[n].mainelem, list[n].maindesc);
	p->gisprompt = str;
	G_asprintf(&str, _("%s file(s) to be removed"), list[n].alias);
	p->description = str;
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    location_path = G_location_path();
    mapset = G_mapset();

    if (force_flag->answer)
	force = 1;

    for (n = 0; n < nlist; n++) {
	if (parm[n]->answers)
	    for (i = 0; (name = parm[n]->answers[i]); i++) {
		if (G_strcasecmp(list[n].alias, "rast") == 0 &&
		    check_reclass(name, mapset, force))
		    continue;

		if (do_remove(n, name) == 1) {
		    result = EXIT_FAILURE;
		}
	    }
    }
    exit(result);
}
