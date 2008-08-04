/* release.mon - turn loose of a monitor */
/* if no name is specified, release current monitor; otherwise, release */
/* monitor specified */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/monitors.h>
#include "open.h"
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int override;
    char *monitor;
    char *option;
    char *me;

    override = 0;
    me = argv[0];
    while (argc > 1 && argv[1][0] == '-') {
	if (argv[1][1] == 0)
	    usage(me);
	option = &argv[1][1];

	while (*option) {
	    switch (*option) {
	    case 'f':
		override = 1;
		break;
		/* Please remove before GRASS 7 is released */
	    case 'v':
		G_set_verbose(G_verbose_max());
		G_warning(_("The '-v' flag is superseded and will be removed "
			    "in future. Please use '--verbose' instead."));
		;
		break;
	    default:
		G_warning(_("%s: -%c unrecognized option"), me, *option);
		usage(me);
	    }
	    option++;
	}
	argv++;
	argc--;
    }
    if (argc > 2)
	usage(me);

    G_gisinit(argv[0]);
    if (argc > 1)
	G__setenv("MONITOR", argv[1]);
    monitor = G__getenv("MONITOR");
    if (!monitor)
	exit(1);

    if (override)
	R_release_driver();
    else {
	R__open_quiet();	/* call R_open_driver to see if we */
	switch (R_open_driver()) {	/*  own the monitor */
	case OK:		/* if not locked or locked by us */
	    R_close_driver();
	case NO_RUN:		/*   or not even in memory, */
	    R_release_driver();	/*   we may release */
	    G_message(_("Monitor <%s> released"), monitor);
	    break;
	case LOCKED:		/* if locked by another, fail */
	    G_message(_("Monitor <%s> in use by another user"), monitor);
	    break;
	case NO_MON:		/* if no such monitor, fail */
	    G_warning(_("No such monitor as <%s>"), monitor);
	    break;
	default:		/* couldn't access lock file? */
	    G_warning(_("Failed testing lock mechanism"));
	    break;
	}
    }
    G_unsetenv("MONITOR");

    return 0;
}

int usage(char *me)
{
    G_fatal_error(_("Usage:  %s [-fv] [name]"), me);
    exit(EXIT_FAILURE);
}
