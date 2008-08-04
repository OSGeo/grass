#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>


int get_legend_box(int *x0, int *x1, int *y0, int *y1)
{
    int screen_x, screen_y;
    int button;
    int cur_screen_x, cur_screen_y;

    D_get_screen_window(&cur_screen_y, &screen_y, &cur_screen_x, &screen_x);

    fprintf(stderr, "\n\n");
    fprintf(stderr, "Buttons:\n");
    fprintf(stderr, "Left:   Establish a corner\n");
    fprintf(stderr, "Middle: Cancel\n");
    fprintf(stderr, "Right:  Accept box for legend\n\n");

    do {
	R_get_location_with_box(cur_screen_x, cur_screen_y, &screen_x,
				&screen_y, &button);
	button &= 0xf;


	switch (button) {
	case 1:
	    cur_screen_x = screen_x;
	    cur_screen_y = screen_y;
	    break;
	case 2:
	    return (0);
	case 3:
	    break;
	}

    } while (button != 3);

    *x0 = cur_screen_x;
    *x1 = screen_x;
    *y0 = cur_screen_y;
    *y1 = screen_y;

    fprintf(stderr, "\n");
    return (1);

}
