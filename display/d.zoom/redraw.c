#include <stdlib.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"

int redraw(void)
{
    int i;
    char command[128];

    R_close_driver();

    sprintf(command, "d.erase");
    system(command);

    if (cmd) {
	system(cmd);
    }
    else {
	/* Redraw raster map */
	if (rast) {
	    for (i = 0; i < nrasts; i++) {
		sprintf(command, "d.rast -o map=%s", rast[i]);
		system(command);
	    }
	}

	/* Redraw vector map */
	if (vect) {
	    for (i = 0; i < nvects; i++) {
		sprintf(command, "d.vect map=%s", vect[i]);
		system(command);
	    }
	}
    }

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));
    D_setup(0);

    return 0;
}
