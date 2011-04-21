
/****************************************************************************
 *
 * MODULE:       g.tempfile
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, Bernhard Reiter <bernhard intevation.de>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
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
    char *tempfile, *G__tempfile();
    int p;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    module->description =
	"Creates a temporary file and prints the file name.";

    pid = G_define_option();
    pid->key = "pid";
    pid->type = TYPE_INTEGER;
    pid->required = YES;
    pid->description = "Process id to use when naming the tempfile";

    G_disable_interactive();
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (sscanf(pid->answer, "%d", &p) != 1) {
	G_usage();
	exit(EXIT_FAILURE);
    }
    tempfile = G__tempfile(p);

    /* create tempfile so next run of this program will create a unique name */
    close(creat(tempfile, 0666));
    fprintf(stdout, "%s\n", tempfile);
    exit(EXIT_SUCCESS);
}
