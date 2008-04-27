/****************************************************************************
 *
 * MODULE:       d.ask
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      ask user to select a database file from mapset search path
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

int main(int argc,char *argv[])
{
    struct GModule *module;
    struct Option *element, *prompt;
    char *tempfile;
    char command[1024];
    FILE *fd;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
		"Prompts the user to select a GRASS data base file from among "
		"files displayed in a menu on the graphics monitor.";

    element = G_define_option();
    element->key = "element";
    element->key_desc = "name,description";
    element->type = TYPE_STRING;
    element->required = YES;
    element->description = "Database element , one word description";

    prompt = G_define_option();
    prompt->key = "prompt";
    prompt->key_desc = "\"message\"";
    prompt->type = TYPE_STRING;
    prompt->description = "Short user prompt message";

    G_disable_interactive();
    if (G_parser(argc,argv))
	exit(1);

/* make sure we can do graphics */
    if (R_open_driver() != 0)
	    G_fatal_error ("No graphics device selected");
    R_close_driver();

    tempfile = G_tempfile();
    unlink (tempfile);
    sprintf (command, "%s/etc/i.find %s %s %s %s",
	G_gisbase(), G_location(), G_mapset(), element->answers[0], tempfile);
    system(command);

    if (access(tempfile,0)==0)
    {
	if (prompt->answer)
	{
	    sprintf (command, "%s/etc/i.ask %s '%s'",
		G_gisbase(), tempfile, prompt->answer);
	}
	else
	{
	    sprintf (command, "%s/etc/i.ask %s",
		G_gisbase(), tempfile);
	}
	exit(system(command));
    }
    else
    {
	fd = popen ("d.menu tcolor=red > /dev/null", "w");
	if (fd)
	{
	    fprintf (fd, "** no %s files found **\n", element->answers[1]);
	    fprintf (fd, "Click here to CONTINUE\n");
	    pclose (fd);
	}
	exit(0);
    }
}



