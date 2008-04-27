/* stop_mon - stop a running monitor */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/raster.h>
#include <grass/gis.h>
#include <grass/monitors.h>
#include <grass/glocale.h>
#include "open.h"
#include "local_proto.h"

static char *me;

int 
main (int argc, char *argv[])
{
    int forced;
    char *option;

    me = argv[0];
    forced = 0;
    while(argc > 1 && argv[1][0] == '-')
    {
	if (argv[1][1] == 0) usage (me);
	option = &argv[1][1];

	while (*option)
	{
	    switch (*option)
	    {
	    case 'f': forced = 1; break;
	    default:  G_warning (_("%s: -%c unrecognized option"),
		    me, *option);
		      usage (me);
	    }
	    option++;
	}
	argv++;
	argc--;
    }
    if (argc != 2)
	usage (me);
    stop_mon(argv[1], forced);

    return 0;
}
int 
usage (char *me)
{
    G_fatal_error(_("Usage: %s [-f] monitor_name"), me);
    exit(EXIT_FAILURE);
}

int 
stop_mon (char *name, int forced)
{
    char *cur;
    int unset;

    unset = 0;
    cur = G__getenv("MONITOR");
    if (cur != NULL && strcmp (cur, name) == 0)
	unset = 1;
    G__setenv("MONITOR",name);
    if (forced)
	R_release_driver();
    R__open_quiet();			/* call open_driver in quiet mode */
    switch (R_open_driver())
    {
    case OK:
	    R_kill_driver();
	    /*R_close_driver();*/
	    G_message(_("Monitor '%s' terminated"),name);
	    break;
    case NO_RUN:
	    G_warning(_("Error - Monitor '%s' was not running"),name);
	    break;
    case NO_MON:
	    G_warning(_("No such monitor as <%s>"),name);
	    break;
    case LOCKED:
	    G_warning(_("Error - Monitor '%s' in use by another user"),name);
	    break;
    default:
	    G_warning(_("Error - Locking mechanism failed"));
	    break;
    }
    if (unset)
	G_unsetenv ("MONITOR");

    return 0;
}

