/* Function: getgrid
 **
 ** Author: Paul W. Carlson     May 1992
 */

#include <stdlib.h>
#include <string.h>
#include "local_proto.h"

#define KEY(x) (strcmp(x,key)==0)

static char *help[] = {
    "font       fontname",
    "fontsize   fontsize",
    "cross	cross_size",
    "color      color",
    "numbers    # [color]",
    ""
};

static char *help2[] = {
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    "numbers    # [color]",
    ""
};

int getgrid(void)
{
    int spacing;
    int color = 0, fontsize = PS_FONT_DEFAULT_SIZE;
    char temp[30];
    char buf[1024];
    char ch, *key, *data;

    PS.grid_font = G_store("Helvetica");
    PS.grid_fontsize = 0;
    PS.grid_color = BLACK;
    PS.grid_numbers = 0;
    PS.grid_cross = 0.;
    PS.grid_width = 0.25;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    color = get_color_number(data);
	    if (color < 0)
		error(key, data, "illegal color request");
	    else
		PS.grid_color = color;
	    continue;
	}

	if (KEY("numbers")) {
	    spacing = -1;

	    if (strlen(data) == 0) {
		spacing = 1;
		color = BLACK;
	    }

	    switch (sscanf(data, "%d %[^\n]", &spacing, temp)) {
	    case 1:
		color = BLACK;
		break;
	    case 2:
		color = get_color_number(temp);
		if (color < 0)
		    spacing = -1;
		break;
	    }
	    if (spacing < 0)
		error(key, data, "illegal numbers request");
	    else {
		PS.grid_numbers = spacing;
		PS.grid_numbers_color = color;
	    }
	    continue;
	}

	if (KEY("cross")) {
	    PS.grid_cross = atof(data);
	    continue;
	}

	if (KEY("fontsize")) {
	    fontsize = atoi(data);
	    if (fontsize < PS_FONT_MIN_SIZE || fontsize > PS_FONT_MAX_SIZE)
		fontsize = PS_FONT_DEFAULT_SIZE;
	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    PS.grid_font = G_store(data);
	    continue;
	}
	if (KEY("width")) {
	    PS.grid_width = -1.;
	    ch = ' ';
	    if ((sscanf(data, "%lf%c", &PS.grid_width, &ch) < 1) ||
		(PS.grid_width < 0.)) {
		PS.grid_width = 1.;
		error(key, data, "illegal grid width request");
	    }
	    if (ch == 'i')
		PS.grid_width = PS.grid_width / 72.0;
	    continue;
	}
	error(key, data, "illegal request (getgrid)");
    }

    PS.grid_fontsize = fontsize;

    return 0;
}

/*************************************************
 * same as getgrid except used for geographic grid
*************************************************/
int getgeogrid(void)
{
    int spacing;
    int color = 0, fontsize = PS_FONT_DEFAULT_SIZE;
    char temp[30];
    char buf[1024];
    char ch, *key, *data;

    PS.geogrid_font = G_store("Helvetica");
    PS.geogrid_fontsize = 0;
    PS.geogrid_color = BLACK;
    PS.geogrid_numbers = 0;
    PS.geogrid_width = 0.25;

    while (input(2, buf, help2)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    color = get_color_number(data);
	    if (color < 0)
		error(key, data, "illegal color request");
	    else
		PS.geogrid_color = color;
	    continue;
	}

	if (KEY("numbers")) {
	    spacing = -1;

	    if (strlen(data) == 0) {
		spacing = 1;
		color = BLACK;
	    }

	    switch (sscanf(data, "%d %[^\n]", &spacing, temp)) {
	    case 1:
		color = BLACK;
		break;
	    case 2:
		color = get_color_number(temp);
		if (color < 0)
		    spacing = -1;
		break;
	    }

	    if (spacing < 0)
		error(key, data, "illegal numbers request");
	    else {
		PS.geogrid_numbers = spacing;
		PS.geogrid_numbers_color = color;
	    }

	    continue;
	}

	if (KEY("fontsize")) {
	    fontsize = atoi(data);
	    if (fontsize < PS_FONT_MIN_SIZE || fontsize > PS_FONT_MAX_SIZE) {
		fontsize = PS_FONT_DEFAULT_SIZE;
	    }
	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    PS.geogrid_font = G_store(data);
	    continue;
	}
	if (KEY("width")) {
	    PS.geogrid_width = -1.;
	    ch = ' ';
	    if ((sscanf(data, "%lf%c", &PS.geogrid_width, &ch) < 1) ||
		(PS.geogrid_width < 0.)) {
		PS.geogrid_width = 1.;
		error(key, data, "illegal grid width request");
	    }
	    if (ch == 'i')
		PS.geogrid_width = PS.geogrid_width / 72.0;
	    continue;
	}
	error(key, data, "illegal request (getgrid)");
    }

    PS.geogrid_fontsize = fontsize;

    return 0;
}
