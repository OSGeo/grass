#include <grass/gis.h>
#include "colors.h"

static int toggle_number;

int table_toggle(char *name, char *mapset, struct Colors *colors)
{
    CELL min, max;
    char *msg = '\0';
    char info[100];

    Rast_get_c_color_range(&min, &max, colors);
    Rast_free_colors(colors);
    sprintf(info, "Color range: %d to %d\n", min, max);

    toggle_number++;
    toggle_number &= 6;
    switch (toggle_number) {
    case 0:
	msg = "Original colors";
	Rast_read_colors(name, mapset, colors);
	break;
    case 1:
	msg = "Ramp colors";
	Rast_make_ramp_colors(colors, min, max);
	break;
    case 2:
	msg = "Grey scale colors";
	Rast_make_grey_scale_colors(colors, min, max);
	break;
    case 3:
	msg = "Random colors";
	Rast_make_random_colors(colors, min, max);
	break;
    case 4:
	msg = "Wave colors";
	Rast_make_wave_colors(colors, min, max);
	break;
    case 5:
	msg = "Aspect colors";
	Rast_make_aspect_colors(colors, min, max);
	break;
    }
    Write_message(2, msg);
    Write_message(3, info);

    return 0;
}
