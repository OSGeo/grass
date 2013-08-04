
/****************************************************************************
 *
 * MODULE:       cmd
 * AUTHOR(S):    CERL (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      lets users copy database files 
 * COPYRIGHT:    (C) 2003-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/manage.h>

int main(int argc, char *argv[])
{
    int i, n, nlist;
    const char *mapset;
    struct GModule *module;
    struct Option **parm;
    char *from, *to;
    int result = EXIT_SUCCESS;

    G_gisinit(argv[0]);

    M_read_list(FALSE, &nlist);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    module->description =
	_("Copies available data files in the current mapset "
	  "search path to the user's current mapset.");
    module->overwrite = 1;

    parm = (struct Option **) G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
      parm[n] = M_define_option(n, _("copied"), NO);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    for (n = 0; n < nlist; n++) {
	if (parm[n]->answers == NULL)
	    continue;
	i = 0;
	while (parm[n]->answers[i]) {
	    from = parm[n]->answers[i++];
	    to = parm[n]->answers[i++];
	    mapset = M_find(n, from, "");
	    if (!mapset) {
		G_warning(_("<%s> not found"), from);
		continue;
	    }
	    if (G_strcasecmp(mapset, G_mapset()) == 0 &&
		G_strcasecmp(from, to) == 0) {
		G_warning(_("%s=%s,%s: files are the same, no copy required"),
			  parm[n]->key, from, to);
		continue;
	    }
	    if (M_find(n, to, G_mapset()) && !(module->overwrite)) {
		G_warning(_("<%s> already exists"), to);
		continue;
	    }
	    if (G_legal_filename(to) < 0) {
		G_warning(_("<%s> is an illegal file name"), to);
		continue;
	    }
	    if (M_do_copy(n, from, mapset, to) == 1) {
		result = EXIT_FAILURE;
	    }
	    G_remove_misc("cell_misc", "reclassed_to", to);
	}
    }

    exit(result);
}
