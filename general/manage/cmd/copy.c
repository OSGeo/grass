
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
#define MAIN
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include "list.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int i, n;
    char *mapset;
    struct GModule *module;
    struct Option **parm, *p;
    char *from, *to;
    int result = EXIT_SUCCESS;

    init(argv[0]);

    module = G_define_module();
    module->keywords = _("general, map management");
    module->description =
	_("Copies available data files in the user's current mapset "
	  "search path and location to the appropriate element "
	  "directories under the user's current mapset.");

    parm = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	char *str;

	p = parm[n] = G_define_option();
	p->key = list[n].alias;
	p->key_desc = "from,to";
	p->type = TYPE_STRING;
	p->required = NO;
	p->multiple = NO;
	G_asprintf(&str, "old,%s,%s", list[n].mainelem, list[n].maindesc);
	p->gisprompt = str;
	G_asprintf(&str, _("%s file(s) to be copied"), list[n].alias);
	p->description = str;
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
	    mapset = find(n, from, "");
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
	    if (find(n, to, G_mapset()) && !(module->overwrite)) {
		G_warning(_("<%s> already exists"), to);
		continue;
	    }
	    if (G_legal_filename(to) < 0) {
		G_warning(_("<%s> is an illegal file name"), to);
		continue;
	    }
	    if (do_copy(n, from, mapset, to) == 1) {
		result = EXIT_FAILURE;
	    }
	    G_remove_misc("cell_misc", "reclassed_to", to);
	}
    }

    exit(result);
}
