
/****************************************************************************
 *
 * MODULE:       i.smap
 * AUTHOR(S):    Michael Shapiro (USACERL) (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      segment multispectral images using a spectral class model 
 *               known as a Gaussian mixture distribution
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "bouman.h"


int main(int argc, char *argv[])
{
    struct parms parms;		/* command line parms */
    struct files files;		/* file descriptors, io, buffers */
    struct SigSet S;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("supervised"));
    G_add_keyword(_("segmentation"));
    G_add_keyword(_("SMAP"));
    module->description =
	_("Performs contextual image classification "
	  "using sequential maximum a posteriori (SMAP) estimation.");

    parse(argc, argv, &parms);
    openfiles(&parms, &files);
    read_signatures(&parms, &S);
    create_output_labels(&S, &files);

    segment(&S, &parms, &files);

    closefiles(&parms, &files);

    G_done_msg(" ");
    
    exit(EXIT_SUCCESS);
}
