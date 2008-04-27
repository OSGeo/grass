/****************************************************************************
 *
 * MODULE:       r.mfilter
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Roberto Flor <flor itc.it>, Markus Neteler <neteler itc.it>
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
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "filter.h"
#include "glob.h"

int main (int argc, char *argv[])
{
    FILTER *filter;
    int nfilters;
    int repeat;
    char *in_name, *in_mapset;
    char *filt_name;
    char *out_name;
    char title[1024];
    char temp[300];
    int i;
    struct GModule *module;
    /* please, remove before GRASS 7 released */
    struct Flag *flag1 ;
    struct Flag *flag2 ;
    struct Option *opt1 ;
    struct Option *opt2 ;
    struct Option *opt3 ;
    struct Option *opt4 ;
    struct Option *opt5 ;

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("raster, map algebra");
    module->description = _("Raster map matrix filter.");

    /* Define the different options */

    opt1 = G_define_standard_option(G_OPT_R_INPUT);

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt3 = G_define_standard_option(G_OPT_F_INPUT);
    opt3->key        = "filter";
    opt3->required   = YES;
    opt3->description= _("Name of filter file") ;

    opt4 = G_define_option() ;
    opt4->key        = "repeat";
    opt4->type       = TYPE_INTEGER;
    opt4->multiple   = NO;
    opt4->required   = NO;
    opt4->answer     = "1";
    opt4->description= _("Number of times to repeat the filter") ;

    opt5 = G_define_option() ;
    opt5->key        = "title";
    opt5->type       = TYPE_STRING;
    opt5->required   = NO;
    opt5->description= _("Output raster map title") ;

    /* Define the different flags */

    /* please, remove before GRASS 7 released */
    flag1 = G_define_flag() ;
    flag1->key         = 'q' ;
    flag1->description = _("Quiet") ;

    /* this isn't implemented at all 
    flag3 = G_define_flag() ;
    flag3->key         = 'p' ;
    flag3->description = _("Preserved edge") ;
    */

    flag2 = G_define_flag() ;
    flag2->key         = 'z' ;
    flag2->description = _("Apply filter only to zero data values") ;

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(flag1->answer) {
        putenv("GRASS_VERBOSE=0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead"));
    }

    /*
    preserve_edges = flag3->answer;
    */
    zero_only = flag2->answer;

    sscanf (opt4->answer, "%d", &repeat);
    out_name = opt2->answer;
    filt_name = opt3->answer;

    in_name = opt1->answer;

    in_mapset = G_find_cell2 (in_name,"");
    if (in_mapset == NULL)
        G_fatal_error (_("Raster map <%s> not found"), in_name);

    nrows = G_window_rows();
    ncols = G_window_cols();
    buflen = ncols * sizeof (CELL);

    /* get the filter */
    filter = get_filter (filt_name, &nfilters, temp);

    /* make sure filter matrix won't extend outside the raster map */
    for (i=0; i < nfilters; i++)
    {
        if (filter[i].size > ncols || filter[i].size > nrows)
	    G_fatal_error (_("Raster map too small for the size of the filter"));
    }


    /* make a title for result */
    if (opt5->answer)
	strcpy (title, opt5->answer);
    else
    {
        if (*temp == 0)
            strcpy (temp, "unknown filter");
        sprintf (title, "%s filtered using %s", in_name, temp);
    }

    perform_filter (in_name, in_mapset, out_name, filter, nfilters, repeat);

    G_put_cell_title (out_name, title);

    exit(EXIT_SUCCESS);
}
