#include <stdio.h>
#include <string.h>
#include <grass/raster.h>
#include <grass/gis.h>
#include <grass/monitors.h>
#include "open.h"

int main(int argc, char *argv[])
{
    struct MON_CAP *cap;
    char *status;
    char *fmt = "%-15s %-30s %s\n";
    char *mon_name;

    mon_name = G__getenv("MONITOR");

    fprintf(stdout, fmt, "name", "description", "status");
    fprintf(stdout, fmt, "----", "-----------", "------");
    while ((cap = R_parse_monitorcap(MON_NEXT, "")) != NULL) {
	G__setenv("MONITOR", cap->name);
	R__open_quiet();
	switch (R_open_driver()) {
	case OK:
	    status = mon_name && (strcmp(cap->name, mon_name) == 0)
		? "running (selected)" : "running";
	    R_close_driver();
	    R_release_driver();
	    break;
	case NO_RUN:
	case NO_MON:
	    status = "not running";
	    break;
	case LOCKED:
	    status = "in use";
	    break;
	default:
	    status = "??";
	    break;
	}			/* switch */
	fprintf(stdout, fmt, cap->name, cap->comment, status);
    }

    return 0;
}
