/****************************************************************************
 *
 * MODULE:       r.rescale.eq
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>
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

static FILE *fd;
static void reclass (CELL,CELL,CELL);

int main (int argc, char *argv[])
{
    char buf[512];
    CELL old_min, old_max;
    CELL new_min, new_max;
    long cat;
    struct Cell_stats statf;
    char *old_name;
    char *new_name;
    char *mapset;
    struct
        {
        struct Option *input, *from, *output, *to, *title;
    } parm;

    /* please, remove before GRASS 7 released */
    struct
        {
        struct Flag *quiet;
    } flag;
    struct GModule *module;

    G_gisinit (argv[0]);
    
    /* Set description */
    module              = G_define_module();
    module->keywords = _("raster");
    module->description =
    _("Rescales histogram equalized the range of category "
    "values in a raster map layer.");

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
    flag.quiet->description = _("Quiet");

    G_disable_interactive();
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

    get_stats (old_name, mapset, &statf);
    if (parm.from->answer)
    {
        sscanf (parm.from->answers[0], "%d", &old_min);
        sscanf (parm.from->answers[1], "%d", &old_max);
    }
    else
        get_range (&statf, &old_min, &old_max, 0);

    if (old_min > old_max)
    {
        cat = old_min; /* swap */
        old_min = old_max;
        old_max = cat;
    }

    sscanf (parm.to->answers[0], "%d", &new_min);
    sscanf (parm.to->answers[1], "%d", &new_max);
    if (new_min > new_max)
    {
        cat = new_min; /* swap */
        new_min = new_max;
        new_max = cat;
    }
    G_message (_("Rescale %s[%d,%d] to %s[%d,%d]"),
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
    G_cell_stats_histo_eq(&statf, (CELL)old_min, (CELL)old_max, (CELL)new_min, (CELL)new_max, 0, reclass);
    if (fd != stdout)
	pclose (fd);
    exit(EXIT_SUCCESS);
}

static void reclass (CELL cat1,CELL cat2,CELL value)
{
    fprintf (fd, "%ld thru %ld = %ld %ld",
	(long)cat1, (long)cat2, (long)value, (long)cat1);
    if (cat1 != cat2)
	fprintf (fd, " thru %ld", (long) cat2);
    fprintf (fd, "\n");
}



