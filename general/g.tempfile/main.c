/****************************************************************************
 *
 * MODULE:       g.tempfile
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:
 * SPDX-FileCopyrightText: 1999-2006, 2011 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *pid;
    struct Flag *dry_run;
    char *tempfile;
    int p;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("support"));
    G_add_keyword(_("scripts"));
    module->description =
        _("Creates a temporary file and prints it's file name.");

    pid = G_define_option();
    pid->key = "pid";
    pid->type = TYPE_INTEGER;
    pid->required = YES;
    pid->description = _("Process id to use when naming the tempfile");

    dry_run = G_define_flag();
    dry_run->key = 'd';
    dry_run->description =
        _("Dry run - don't create a file, just prints it's file name");

    G_disable_interactive();
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (sscanf(pid->answer, "%d", &p) != 1) {
        G_usage();
        exit(EXIT_FAILURE);
    }
    tempfile = G_tempfile_pid(p);

    /* create tempfile so next run of this program will create a unique name */
    if (!dry_run->answer)
        close(creat(tempfile, 0666));
    fprintf(stdout, "%s\n", tempfile);

    exit(EXIT_SUCCESS);
}
