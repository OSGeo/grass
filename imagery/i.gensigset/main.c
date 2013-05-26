
/****************************************************************************
 *
 * MODULE:       i.gensigset
 * AUTHOR(S):    Charles Bouman, Purdue University and
 *               Michael Shapiro, USACERL (original contributors)
 *               Markus Neteler <neteler itc.it> 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      non-interactive method for generating image signature files
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "files.h"
#include "parms.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct parms parms;		/* command line parms */
    struct files files;		/* file descriptors, io, buffers */
    struct SigSet S;
    int i;
    int junk;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("supervised"));
    G_add_keyword(_("SMAP"));
    G_add_keyword(_("signatures"));
    module->description =
	_("Generates statistics for i.smap from raster map.");

    parse(argc, argv, &parms);
    openfiles(&parms, &files);
    read_training_labels(&parms, &files);

    get_training_classes(&parms, &files, &S);
    read_data(&files, &S);

    for (i = 0; i < S.nclasses; i++) {
	G_message(_("Clustering class %d (%d pixels)..."),
		  i + 1, S.ClassSig[i].ClassData.npixels);
	subcluster(&S, i, &junk, parms.maxsubclasses);
	G_message(_("Number of subclasses is %d"),
		  S.ClassSig[i].nsubclasses);
    }
    write_sigfile(&parms, &S);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
