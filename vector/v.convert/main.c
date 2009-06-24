
/*****************************************************************
 *
 * MODULE:       v.convert
 * 
 * AUTHOR(S):    Radim Blazek - Radim.Blazek@dhv.cz
 *               
 * PURPOSE:      Convert GRASS vector maps versions:
 *               from 3 or 4 to 5.0
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "conv.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct Option *opt_in, *opt_out, *opt_end;
    int endian;
    char *output;
    struct GModule *module;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("conversion"));
    module->description = _("Imports older versions of GRASS vector maps.");

    /* input vector map */
    opt_in = G_define_standard_option(G_OPT_V_INPUT);
    opt_in->gisprompt = "old,dig,vector";

    /* output vector map */
    opt_out = G_define_standard_option(G_OPT_V_OUTPUT);
    opt_out->required = NO;

    /* endian of input vector map */
    opt_end = G_define_option();
    opt_end->key = "endian";
    opt_end->type = TYPE_STRING;
    opt_end->required = NO;
    opt_end->multiple = NO;
    opt_end->options = "big,little";
    opt_end->description = _("Endian of input vector map");
    opt_end->answer = "big";

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Numbers in portable format files are saved as big endians */
    if (opt_end->answer[0] == 'l')
	endian = ENDIAN_LITTLE;
    else
	endian = ENDIAN_BIG;

    if (opt_out->answer)
	output = G_store(opt_out->answer);
    else
	output = G_store(opt_in->answer);

    old2new(opt_in->answer, output, endian);

    exit(EXIT_SUCCESS);
}
