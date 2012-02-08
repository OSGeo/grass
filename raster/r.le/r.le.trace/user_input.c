/*
 ************************************************************
 * MODULE: r.le.trace/user_input.c                          *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To obtain information from the user about what  *
 *          information is desired                          *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/config.h>
#include <grass/glocale.h>
#include "r.le.trace.h"

extern struct CHOICE *choice;

void user_input(int argc, char **argv)
{
    struct Flag *bound;
    struct Flag *trace;
    struct Option *name;
    struct Option *out;

    bound = G_define_flag();
    bound->key = 'p';
    bound->description = _("Include sampling area boundary as perimeter");

    trace = G_define_flag();
    trace->key = 't';
    trace->description = _("Use 4 neighbor tracing instead of 8 neighbor");

    name = G_define_standard_option(G_OPT_R_MAP);
    name->description = _("Raster map to be analyzed");

    out = G_define_standard_option(G_OPT_F_OUTPUT);
    out->description = _("Name of output file to store patch data");
    out->required = NO;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    strcpy(choice->fn, name->answer);

    if (out->answer)
	strcpy(choice->out, out->answer);
    else
	strcpy(choice->out, "");

    /* if the 4 neighbor tracing flag -t
       is specified, then set the 
       choice->trace flag to 1 */

    choice->trace = 1;
    if (trace->answer)
	choice->trace = 0;

    /* if the -p flag is specified, then
       set the choice->perim2 flag to 0 */

    if (bound->answer)
	choice->perim2 = 0;
    else
	choice->perim2 = 1;

}
