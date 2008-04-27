/****************************************************************************
 *
 * MODULE:       r.rescale
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>

int main (int argc, char *argv[])
{
	char buf[512];
	FILE *fd;
	long old_min, old_max;
	long new_min, new_max;
	long new_delta, old_delta;
	long value, first, prev;
	long cat;
	float divisor ;
	char *old_name;
	char *new_name;
	char *mapset;
	struct GModule *module;
	struct
	    {
		struct Option *input, *from, *output, *to, *title;
	} parm;
        /* please, remove before GRASS 7 released */
	struct
	    {
		struct Flag *quiet;
	} flag;

	G_gisinit (argv[0]);

	module = G_define_module();
    module->keywords = _("raster");
    module->description =
		_("Rescales the range of category values "
		"in a raster map layer.");
				        
	/* Define the different options */

	parm.input = G_define_option() ;
	parm.input->key        = "input";
	parm.input->type       = TYPE_STRING;
	parm.input->required   = YES;
	parm.input->gisprompt  = "old,cell,raster" ;
	parm.input->description= _("The name of the raster map to be rescaled") ;

	parm.from = G_define_option() ;
	parm.from->key        = "from";
	parm.from->key_desc   = "min,max";
	parm.from->type       = TYPE_INTEGER;
	parm.from->required   = NO;
	parm.from->description= _("The input data range to be rescaled (default: full range of input map)");

	parm.output = G_define_option() ;
	parm.output->key        = "output";
	parm.output->type       = TYPE_STRING;
	parm.output->required   = YES;
	parm.output->gisprompt  = "new,cell,raster" ;
	parm.output->description= _("The resulting raster map name");

	parm.to = G_define_option() ;
	parm.to->key        = "to";
	parm.to->key_desc   = "min,max";
	parm.to->type       = TYPE_INTEGER;
	parm.to->required   = YES;
	parm.to->description= _("The output data range");

	parm.title = G_define_option() ;
	parm.title->key        = "title";
	parm.title->key_desc   = "\"phrase\"";
	parm.title->type       = TYPE_STRING;
	parm.title->required   = NO;
	parm.title->description= _("Title for new raster map") ;

        /* please, remove before GRASS 7 released */
	flag.quiet = G_define_flag();
	flag.quiet->key = 'q';
	flag.quiet->description = _("Quietly");

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(flag.quiet->answer) {
        putenv("GRASS_VERBOSE=0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }

	old_name = parm.input->answer;
	new_name = parm.output->answer;

	mapset = G_find_cell (old_name,"");
	if (mapset == NULL)
	{
		sprintf (buf, "%s - not found\n", old_name);
		G_fatal_error (buf);
	}
	if (G_legal_filename (new_name) < 0)
	{
		sprintf (buf, "%s - illegal map name\n", new_name);
		G_fatal_error (buf);
	}

	if (parm.from->answer)
	{
		sscanf (parm.from->answers[0], "%ld", &old_min);
		sscanf (parm.from->answers[1], "%ld", &old_max);

	}
	else
		get_range (old_name, mapset, &old_min, &old_max);
	if (old_min > old_max)
	{
		value = old_min; /* swap */
		old_min = old_max;
		old_max = value;
	}

	sscanf (parm.to->answers[0], "%ld", &new_min);
	sscanf (parm.to->answers[1], "%ld", &new_max);
	if (new_min > new_max)
	{
		value = new_min; /* swap */
		new_min = new_max;
		new_max = value;
	}
	
        G_message (_("Rescale %s[%ld,%ld] to %s[%ld,%ld]"),
		    old_name, old_min, old_max, new_name, new_min, new_max);

	sprintf (buf, "r.reclass input=\"%s\" output=\"%s\" title=\"", old_name, new_name);
	if (parm.title->answer)
		strcat (buf, parm.title->answer);
	else
	{
		strcat (buf, "rescale of ");
		strcat (buf, old_name);
	}
	strcat (buf, "\"");

	fd = popen (buf, "w");
	old_delta = old_max - old_min ;
	new_delta = new_max - new_min ;
	divisor = (float)new_delta/(float)old_delta ;

	prev = new_min;
	first = old_min;
	for (cat = old_min; cat <= old_max; cat++)
	{
		value = (int)(divisor * (cat - old_min) + new_min + .5) ;
		if (value != prev)
		{
			fprintf (fd, "%ld thru %ld = %ld %ld", first, cat-1, prev, first);
			if (cat-1 != first)
				fprintf (fd, " thru %ld", cat-1);
			fprintf (fd, "\n");
			prev = value;
			first = cat;
		}
	}
	fprintf (fd, "%ld thru %ld = %ld %ld", first, cat-1, prev, first);
	if (cat-1 != first)
		fprintf (fd, " thru %ld", cat-1);
	fprintf (fd, "\n");

	pclose (fd);
	exit(EXIT_SUCCESS);
}
