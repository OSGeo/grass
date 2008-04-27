/****************************************************************************
 *
 * MODULE:       g.list
 *               
 * AUTHOR(S):    Michael Shapiro,
 *               U.S.Army Construction Engineering Research Laboratory
 *               
 * PURPOSE:      Lists available GRASS data base files of the
 *               user-specified data type to standard output
 *
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define MAIN
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/spawn.h>
#include "local_proto.h"
#include "list.h"

struct Option *element;

int 
main (int argc, char *argv[])
{
	int i, n, len;
	struct GModule *module;
	struct Option *mapset;
	struct Flag *full;

	init (argv[0]);

	module = G_define_module();
	module->keywords = _("general, map management");
	module->description =
		_("Lists available GRASS data base files "
		  "of the user-specified data type to standard output.");

	element = G_define_option();
	element->key =      "type";
	element->key_desc = "datatype";
	element->type     = TYPE_STRING;
	element->required = YES;
	element->multiple = YES;
	element->description = "Data type";
	for (len=0,n=0 ; n < nlist; n++)
	    len += strlen (list[n].alias)+1;
	element->options = G_malloc(len);

	for (n=0; n < nlist; n++)
	{
	    if (n)
	    {
		G_strcat (element->options, ",");
		G_strcat (element->options, list[n].alias);
	    }
	    else
		G_strcpy (element->options, list[n].alias);
	}

	mapset = G_define_option();
	mapset->key = "mapset";
	mapset->type = TYPE_STRING;
	mapset->required = NO;
	mapset->multiple = NO;
	mapset->description = _("Mapset to list (default: current search path)");
#define MAPSET mapset->answer

	full = G_define_flag();
	full->key = 'f';
	full->description = _("Verbose listing (also list map titles)");
#define FULL full->answer

	if (G_parser(argc, argv))
	{
		exit(EXIT_FAILURE);
	}

	if (MAPSET == NULL)
		MAPSET = "";

	if (G_strcasecmp (MAPSET,".") == 0)
		MAPSET = G_mapset();

	i = 0;
	while (element -> answers[i])
	{ 
		n = parse(element -> answers[i]);
		
		if (FULL)
		{
			char lister[300];
			sprintf (lister, "%s/etc/lister/%s", G_gisbase(), list[n].element[0]);
			G_debug(3,"lister CMD: %s",lister);
			if (access (lister, 1) == 0) /* execute permission? */
				G_spawn (lister, lister, MAPSET, NULL);
			else
				do_list (n, MAPSET);
		}
		else
		{
			do_list (n, MAPSET);
		}

		i++;
	}

	exit(EXIT_SUCCESS);
}

int 
parse (const char *data_type)
{
	int n;
	
	for (n = 0 ; n < nlist; n++)
	{
		if (G_strcasecmp (list[n].alias, data_type) == 0)
			break;
	}

	return n;
}
