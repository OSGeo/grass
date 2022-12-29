/* Function: getgrid
 **
 ** Author: Paul W. Carlson     May 1992
 */

#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "clr.h"
#include "local_proto.h"

#define KEY(x) (strcmp(x,key)==0)

static char *help[] = {
    "font       fontname",
    "fontsize   fontsize",
    "cross	cross_size",
    "color      color",
    "numbers    # [color]",
    "width      #",
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
    int fontsize = PS_FONT_DEFAULT_SIZE;
    int ret, r, g, b;
    char temp[30];
    char buf[1024];
    char ch, *key, *data;
    PSCOLOR color, text_color;

    PS.grid_font = G_store("Helvetica");
    PS.grid_fontsize = 0;
    PS.grid_numbers = 0;
    PS.grid_cross = 0.;
    PS.grid_width = 0.25;
    set_color(&color, 0, 0, 0);
    set_color(&text_color, 0, 0, 0);

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b); 
	    if (ret == 1)
		set_color(&color, r, g, b);
	    else if (ret == 2)  /* i.e. "none" */
		/* unset_color(&color); */
		error(key, data, _("Unsupported color request"));
	    else
		error(key, data, _("illegal color request"));

	    continue;
	}

	if (KEY("numbers")) {
	    spacing = -1;

	    if (strlen(data) == 0) {
		spacing = 1;
		set_color(&text_color, 0, 0, 0);
	    }

	    switch (sscanf(data, "%d %[^\n]", &spacing, temp)) {
	    case 1:
		set_color(&text_color, 0, 0, 0);
		break;
	    case 2:
		ret = G_str_to_color(temp, &r, &g, &b);
		if (ret == 1)
		    set_color(&text_color, r, g, b);
		else if (ret == 2)  /* i.e. "none" */
		    error(key, data, _("Unsupported color request"));
		else
		    error(key, data, _("illegal color request"));

		if (ret < 1)
		    spacing = -1;

		break;
	    }
	    if (spacing < 0)
		error(key, data, _("illegal numbers request"));
	    else {
		PS.grid_numbers = spacing;
		PS.grid_numbers_color = text_color;
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
		error(key, data, _("illegal grid width request"));
	    }
	    if (ch == 'i')
		PS.grid_width = PS.grid_width * 72.0;
	    continue;
	}
	error(key, data, _("illegal request (grid)"));
    }

    PS.grid_fontsize = fontsize;
    PS.grid_color = color;

    return 0;
}

/*************************************************
 * same as getgrid except used for geographic grid
*************************************************/
int getgeogrid(void)
{
    int spacing;
    int fontsize = PS_FONT_DEFAULT_SIZE;
    int ret, r, g, b;
    char temp[30];
    char buf[1024];
    char ch, *key, *data;
    PSCOLOR color, text_color;

    PS.geogrid_font = G_store("Helvetica");
    PS.geogrid_fontsize = 0;
    PS.geogrid_numbers = 0;
    PS.geogrid_width = 0.25;
    set_color(&color, 0, 0, 0);
    set_color(&text_color, 0, 0, 0);

    while (input(2, buf, help2)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b); 
	    if (ret == 1)
		set_color(&color, r, g, b);
	    else if (ret == 2)  /* i.e. "none" */
		/* unset_color(&color); */
		error(key, data, _("Unsupported color request"));
	    else
		error(key, data, _("illegal color request"));

	    continue;
	}

	if (KEY("numbers")) {
	    spacing = -1;

	    if (strlen(data) == 0) {
		spacing = 1;
		set_color(&text_color, 0, 0, 0);
	    }

	    switch (sscanf(data, "%d %[^\n]", &spacing, temp)) {
	    case 1:
		set_color(&text_color, 0, 0, 0);
		break;
	    case 2:
		ret = G_str_to_color(temp, &r, &g, &b);
		if (ret == 1)
		    set_color(&text_color, r, g, b);
		else if (ret == 2)  /* i.e. "none" */
		    error(key, data, _("Unsupported color request"));
		else
		    error(key, data, _("illegal color request"));

		if (ret < 1)
		    spacing = -1;

		break;
	    }

	    if (spacing < 0)
		error(key, data, _("illegal numbers request"));
	    else {
		PS.geogrid_numbers = spacing;
		PS.geogrid_numbers_color = text_color;
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
		error(key, data, _("illegal grid width request"));
	    }
	    if (ch == 'i')
		PS.geogrid_width = PS.geogrid_width * 72.0;
	    continue;
	}
	error(key, data, _("illegal request (geogrid)"));
    }

    PS.geogrid_fontsize = fontsize;
    PS.geogrid_color = color;

    return 0;
}
