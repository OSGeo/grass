#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include "local_proto.h"
#include <grass/glocale.h>

int zoomwindow(struct Cell_head *window, int quiet, double magnify)
{
    int screen_x, screen_y, button;
    int end = 0;
    int printmenu = 1;

    while (!end) {
	if (printmenu) {
	    fprintf(stderr, _("\n\nButtons:\n"));
	    fprintf(stderr, _("Left:   Zoom menu\n"));
	    fprintf(stderr, _("Middle: Pan\n"));
	    fprintf(stderr, _("Right:  Quit menu\n"));
	    printmenu = 0;
	}

	R_get_location_with_pointer(&screen_x, &screen_y, &button);

	if (button == 1) {
	    /* enter zoom menu */
	    make_window_box(window, magnify, 1, 0);
	    printmenu = 1;
	}
	else if (button == 2) {
	    /* pan */
	    pan_window(window, screen_x, screen_y);
	}
	else if (button == 3) {
	    end = 1;
	}
    }

#ifdef QUIET
    if (!quiet) {
	fprintf(stderr, _("This region now saved as current region.\n\n"));
	fprintf(stderr,
		_("Note: run 'd.erase' for the new region to affect the graphics.\n"));
    }
#endif
    return (0);
}
