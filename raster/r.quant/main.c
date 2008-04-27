/****************************************************************************
 *
 * MODULE:       r.quant
 * AUTHOR(S):    Michael Shapiro, Olga Waupotitsch, CERL (original contributors)
 *               Markus Neteler <neteler itc.it>, Roberto Flor <flor itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define MAIN
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "global.h"
#include <grass/glocale.h>

int 
main (int argc, char *argv[])
{
    char buf[1024];
	struct GModule *module;
    struct Option *input, *basemap, *fprange, *range;
    struct Flag *trunc, *rnd;
    int truncate;
    int round;
    int i;
    CELL new_min, new_max;
    DCELL new_dmin, new_dmax;
    char *basename, *basemapset;

    G_gisinit (argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Produces the quantization file for a floating-point map.");

    basemap = G_define_option();
    basemap->key = "basemap";
    basemap->required = NO;
    basemap->type = TYPE_STRING;
    basemap->answer = "NONE";
    basemap->gisprompt  = "old,cell,raster" ;
    basemap->description = _("Base map to take quant rules from");

    input = G_define_option();
    input->key = "input";
    input->required = YES;
    input->multiple = YES ;
    input->type = TYPE_STRING;
    input->gisprompt  = "old,cell,raster" ;
    input->description =  _("Raster map(s) to be quantized");

    fprange = G_define_option();
    fprange->key = "fprange";
    fprange->key_desc = "dmin,dmax";
    fprange->description = _("Floating point range: dmin,dmax");
    fprange->type = TYPE_STRING;
    fprange->answer = "";
    fprange->required = YES;

    range = G_define_option();
    range->key = "range";
    range->key_desc = "min,max";
    range->description = _("Integer range: min,max");
    range->type = TYPE_STRING;
    range->answer = "1,255";
    range->required = YES;

    trunc = G_define_flag();
    trunc->key = 't';
    trunc->description	= _("Truncate floating point data");

    rnd = G_define_flag();
    rnd->key = 'r';
    rnd->description	= _("Round floating point data");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    truncate = trunc->answer;
    round = rnd->answer;
    G_quant_init(&quant_struct);

    /* read and check inputs */
    for (noi = 0; input->answers[noi]; noi++)
    {
       name[noi] = G_store(input->answers[noi]);
       mapset[noi] = G_find_cell2 (name[noi], "");
       if (mapset[noi] == NULL)
       {
 	 sprintf (buf, "%s - not found", name[noi]);
	 G_fatal_error (buf);
       }

       if(G_raster_map_type(name[noi], mapset[noi]) == CELL_TYPE)
       {
	 sprintf (buf, "%s is integer map, it can't be quantized", name[noi]);
	 G_fatal_error (buf);
       }
    }

    basename = basemap->answer;

    /* now figure out what new quant rules to write */
    if(truncate)
    {
	G_message (_("Truncating..."));
	G_quant_truncate (&quant_struct);
    }

    else if(round)
    {
	G_message (_("Rounding..."));
	G_quant_round (&quant_struct);
    }

    else if(strncmp(basename, "NONE",4) != 0) 
    /* set the quant to that of basemap */
    {
        basemapset = G_find_cell2 (basename, "");
        if (basemapset == NULL)
        {
 	    sprintf (buf, "%s - not found", basename);
	    G_fatal_error (buf);
        }

        if(G_raster_map_type(basename, basemapset) == CELL_TYPE)
        {
	    sprintf (buf, "%s is integer map, it can't be used as basemap", basename);
	    G_fatal_error (buf);
        }

	if(G_read_quant(basename, basemapset, &quant_struct)<=0)
        {
	    sprintf (buf, "Can't read quant rules for basemap %s! Exiting.", basename);
	    G_fatal_error (buf);
        }
     }

     else if((sscanf(fprange->answer, "%lf,%lf", &new_dmin, &new_dmax)==2)
          && (sscanf(range->answer, "%d,%d", &new_min, &new_max)==2))
     {
       G_message (_("Setting quant rules for input map(s) to (%f %f) -> (%d,%d)"),
	       new_dmin, new_dmax, new_min, new_max);
       G_quant_add_rule(&quant_struct, new_dmin,new_dmax, new_min,new_max);
     }

     else /* ask user for quant rules */
     {

        if (!read_rules())
        {
	    if (isatty(0))
	        G_message (_("No rules specified. Quant table(s) not changed."));
	    else
	        G_fatal_error ("No rules specified");
        }

    } /* use rules */


    for(i=0; i < noi; i++)
    {
       if( G_write_quant(name[i], mapset[i], &quant_struct) < 0)
   	   G_message(_("Quant table not changed for %s"), name[i]);
       else
	   G_message(_("New quant table created for %s"), name[i]);
    }

    exit(EXIT_FAILURE);
}
