/****************************************************************************
 *
 * MODULE:       i.gensig
 * AUTHOR(S):    Michael Shapiro (USACERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      generate image signatures based on training map
 * SPDX-FileCopyrightText: 1999-2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/

#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "signature.h"
#include "parms.h"
#include "files.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct parms parms; /* command line parms */
    struct files files; /* file descriptors, io, buffers */
    struct Signature S;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("supervised classification"));
    G_add_keyword(_("Maximum Likelihood Classification"));
    G_add_keyword("MLC");
    G_add_keyword(_("signatures"));
    module->description =
        _("Generates statistics for i.maxlik from raster map.");

    parse(argc, argv, &parms);
    openfiles(&parms, &files, &S);
    read_training_labels(&parms, &files);

    get_training_classes(&files, &S);
    compute_means(&files, &S);
    compute_covariances(&files, &S);
    check_signatures(&S);
    write_sigfile(&parms, &S);
    I_free_signatures(&S);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

