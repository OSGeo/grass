
/****************************************************************************
 *
 * MODULE:       nviz
 * AUTHOR(S):    Bill Brown, Terry Baker, Mark Astley and David Gerdes
 *               (CERL and UIUC, original contributors)
 *               Bob Covill, Tekmap Consulting
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Philip Warner <pjw rhyme.com.au>
 * PURPOSE:      main app for nviz visualization and animation tool
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************//*
 * This is basically tkAppInit.c from the tk4.0 distribution except
 * that we define Tcl_AppInit in tkAppInit.c.
 */

#include <stdlib.h>
#include <string.h>
#include <tk.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "interface.h"

extern int NVIZ_AppInit(Tcl_Interp *);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tk_Main never returns here, so this procedure never
 *      returns either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

struct options opts;
int script_mode;

int main(int argc, char **argv)
{
    struct GModule *module;
    char *argv2[4];
    char *source;

    G_gisinit(argv[0]);

    if (argc >= 2 && strcmp(argv[1], "-f") == 0) {
	script_mode = 1;
	Tcl_FindExecutable(argv[0]);
	Tk_Main(argc, argv, NVIZ_AppInit);
	exit(EXIT_SUCCESS);
    }

    module = G_define_module();
    G_add_keyword(_("visualization"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("display"));
    module->description =
	_("nviz - Visualization and animation tool for GRASS data.");

    opts.elev = G_define_standard_option(G_OPT_R_ELEV);
    opts.elev->required = NO;
    opts.elev->multiple = YES;
    opts.elev->description = _("Name of raster map(s) for Elevation");
    opts.elev->guisection = _("Raster");

    opts.colr = G_define_option();
    opts.colr->key = "color";
    opts.colr->type = TYPE_STRING;
    opts.colr->required = NO;
    opts.colr->multiple = YES;
    opts.colr->gisprompt = "old,cell,raster";
    opts.colr->description = _("Name of raster map(s) for Color");
    opts.colr->guisection = _("Raster");

    opts.vct = G_define_option();
    opts.vct->key = "vector";
    opts.vct->type = TYPE_STRING;
    opts.vct->required = NO;
    opts.vct->multiple = YES;
    opts.vct->gisprompt = "old,vector,vector";
    opts.vct->description = _("Name of vector lines/areas overlay map(s)");
    opts.vct->guisection = _("Vector");

    opts.pnt = G_define_option();
    opts.pnt->key = "points";
    opts.pnt->type = TYPE_STRING;
    opts.pnt->required = NO;
    opts.pnt->multiple = YES;
    opts.pnt->gisprompt = "old,vector,vector";
    opts.pnt->description = _("Name of vector points overlay file(s)");
    opts.pnt->guisection = _("Vector");

    opts.vol = G_define_option();
    opts.vol->key = "volume";
    opts.vol->type = TYPE_STRING;
    opts.vol->required = NO;
    opts.vol->multiple = YES;
    opts.vol->gisprompt = "old,grid3,3d-raster";
    opts.vol->description = _("Name of existing 3d raster map");
    opts.vol->guisection = _("Raster");

    opts.no_args = G_define_flag();
    opts.no_args->key = 'q';
    opts.no_args->description = _("Quickstart - Do not load any data");

    opts.script_kill = G_define_flag();
    opts.script_kill->key = 'k';
    opts.script_kill->description =
	_("Exit after completing script launched from the command line");

    opts.demo = G_define_flag();
    opts.demo->key = 'x';
    opts.demo->description =
	_("Start in Demo mode (skip the \"please wait\" message)");

    opts.panel_path = G_define_option();
    opts.panel_path->key = "path";
    opts.panel_path->type = TYPE_STRING;
    opts.panel_path->required = NO;
    opts.panel_path->description = _("Set alternative panel path");

    opts.script = G_define_option();
    opts.script->key = "script";
    opts.script->type = TYPE_STRING;
    opts.script->required = NO;
    opts.script->description = _("Execute script file at startup");

    opts.state = G_define_option();
    opts.state->key = "state";
    opts.state->type = TYPE_STRING;
    opts.state->required = NO;
    opts.state->description = _("Load previously saved state file");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Tcl_FindExecutable(argv[0]);

    G_asprintf(&source, "%s/etc/nviz2.2/scripts/nviz2.2_script", G_gisbase());

    argv2[0] = argv[0];
    argv2[1] = "-f";
    argv2[2] = source;
    argv2[3] = NULL;

    Tk_Main(4, argv2, NVIZ_AppInit);

    exit(EXIT_SUCCESS);
}

