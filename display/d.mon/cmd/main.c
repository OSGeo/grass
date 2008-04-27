/****************************************************************************
 *
 * MODULE:       d.mon
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      control graphic monitors
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/* Changed for truecolor 24bit support by 
 * Roberto Flor/ITC-Irst, Trento, Italy
 * August 1999
 *
 * added new parameter "nlev" to specify number of colors per color channel
 * example; nlev=8 means 8bit for each R, G, B equal to 24bit truecolor
*/
 
int run(char *, char *);

int main(int argc, char *argv[])
{
    int error;
    int oops;
    char *mon_name;

	struct GModule *module;
    struct Option *start, *stop, *select, *unlock;
    struct Flag *list, *status, *print, *release, *no_auto_select;

    G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("display");
    module->description =
	   _("To establish and control use of a graphics display monitor.");

    start = G_define_option();
    start->key="start";
    start->type=TYPE_STRING;
    start->required=NO;
    start->description= _("Name of graphics monitor to start");

    stop = G_define_option();
    stop->key="stop";
    stop->type=TYPE_STRING;
    stop->required=NO;
    stop->description= _("Name of graphics monitor to stop");

    select = G_define_option();
    select->key="select";
    select->type=TYPE_STRING;
    select->required=NO;
    select->description= _("Name of graphics monitor to select");

    unlock = G_define_option();
    unlock->key="unlock";
    unlock->type=TYPE_STRING;
    unlock->required=NO;
    unlock->description= _("Name of graphics monitor to unlock");

    list = G_define_flag();
    list->key='l';
    list->description= _("List all monitors");

    status = G_define_flag();
    status->key='L';
    status->description= _("List all monitors (with current status)");

    print = G_define_flag();
    print->key='p';
    print->description= _("Print name of currently selected monitor");

    release = G_define_flag();
    release->key='r';
    release->description= _("Release currently selected monitor");

    no_auto_select = G_define_flag();
    no_auto_select->key='s';
    no_auto_select->description=
	_("Do not automatically select when starting");

    if (G_parser(argc,argv))
	exit(EXIT_FAILURE);

    if (unlock->answer)
	run("release -f",unlock->answer);

    if (!select->answer && !no_auto_select->answer)
	select->answer = start->answer;

    G__read_env();
    mon_name = G__getenv("MONITOR"); /* remember old monitor name */

    error = 0;
    if (status->answer)
	error += run("status","");
    else if (list->answer)
	error += run("list","");
    if (release->answer)
	error += run("release","");
    if (stop->answer)
	error += run("stop",stop->answer);
    if (start->answer)
    {
	error += run("start",start->answer);
        if(error) /* needed procedure failed */
	{
            if(mon_name != NULL)
	    {
		/* restore the previous environ. */
		G__setenv("MONITOR", mon_name); 
		/* write the name to the .gisrc file */
		G__write_env();
	    }
	}
    }
    if (select->answer)
    {
	oops = run("select", select->answer); /* couldn't select */
	if (oops && start->answer && strcmp (start->answer, select->answer) == 0) /* try once more */
	{
	    G_message(_("Problem selecting %s. Will try once more"), select->answer);
	    oops = run("select", select->answer); /* couldn't select */
	}
        if(oops) /* needed procedure failed */
          {
            if(mon_name != NULL)
              {
		 /* restore the previous environ. */
                 G__setenv("MONITOR", mon_name); 
                 /* write the name to the .gisrc file */
                 G__write_env();
	       }
           }
	error += oops;
    }
    if (print->answer)
	error += run("which","");
    exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int run (char *pgm, char *name)
{
    char command[1024];

    sprintf (command, "%s/etc/mon.%s %s", G_gisbase(), pgm, name);
    return system(command);
}



