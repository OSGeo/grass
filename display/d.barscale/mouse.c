#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "options.h"

int mouse_query(int toptext)
{
    int t, b, l, r;
    char *panel = G_tempfile();
    int x_pos, y_pos, button;

    fprintf(stderr, "Left:  choose location\n" "Middle: cancel\n");

    R_get_location_with_pointer(&x_pos, &y_pos, &button);

    if (button == 2)
	return 0;

    D_get_screen_window(&t, &b, &l, &r);

    for (;;) {
	east = (x_pos * 100.0) / (r - l);
	north = (y_pos * 100.0) / (b - t);

	draw_scale(panel, toptext);

	fprintf(stderr,
		"\n"
		"Left: choose location\n"
		"Middle: cancel\n" "Right: confirm location\n");

	R_get_location_with_pointer(&x_pos, &y_pos, &button);

	switch (button) {
	case 1:
	    R_panel_restore(panel);
	    break;
	case 2:
	    R_panel_restore(panel);
	    return 0;
	case 3:
	    R_panel_delete(panel);
	    return 1;
	}
    }
}
