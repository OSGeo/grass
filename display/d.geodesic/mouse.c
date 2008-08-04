#include <stdio.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "local_proto.h"

void mouse(int line_color, int text_color)
{
    int x1, y1, x2, y2;
    double lat1, lon1, lat2, lon2;
    int top, bottom, left, right;
    int button;

    right = (int)D_get_d_east();
    left = (int)D_get_d_west();
    top = (int)D_get_d_north();
    bottom = (int)D_get_d_south();

    x2 = x1 = (right + left) / 2;
    y2 = y1 = (top + bottom) / 2;

    fprintf(stderr, "\n");
    fprintf(stderr, "Left:   begin\n");
    fprintf(stderr, "Middle: draw\n");
    fprintf(stderr, "Right:  quit\n");

    while (1) {
	R_standard_color(line_color);
	R_get_location_with_line(x1, y1, &x2, &y2, &button);
	if (button == 3)
	    return;
	if (x2 < left || x2 > right)
	    continue;
	if (y2 < top || y2 > bottom)
	    continue;

	if (button == 2) {
	    lon1 = D_d_to_u_col((double)x1);
	    lat1 = D_d_to_u_row((double)y1);
	    lon2 = D_d_to_u_col((double)x2);
	    lat2 = D_d_to_u_row((double)y2);
	    plot(lon1, lat1, lon2, lat2, line_color, text_color);
	}

	x1 = x2;
	y1 = y2;
    }
}
