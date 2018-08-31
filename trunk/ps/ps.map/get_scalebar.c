/* Function: read_scalebar
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "where      x y",
    "length	length",
    "units	auto|meters|kilometers|feet|miles|nautmiles",
    "height	height",
    "segment	no_segemnts",
    "numbers	no_labels",
    "font   	fontname",
    "fontsize   fontsize",
    "color   	fontcolor",
    "bgcolor   	backgroundcolor",
    "background [Y|n]",
    "width      #",
    ""
};

int read_scalebar(void)
{
    char buf[1024];
    char *key, *data;
    char ch;

    /* struct defined in decorate.h */
    sb.segment = 4;		/* four segments */
    sb.numbers = 1;		/* label each segment */
    sb.font = G_store("Helvetica");
    sb.fontsize = 8;
    sb.color = BLACK;		/* TODO: multi-color */
    sb.width = 1.;
    sb.length = -1.;
    sb.height = 0.1;		/* default height in inches */
    sb.x = PS.page_width / 2.;
    sb.y = 2.;
    sb.bgcolor = 1;		/* TODO: multi-color */
    sb.units = SB_UNITS_AUTO;   /* default to automatic based on value in PROJ_UNITS */


    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("where")) {
	    if (sscanf(data, "%lf %lf", &sb.x, &sb.y) != 2) {
		error(key, data, "illegal where request");
	    }
	    else
		continue;
	}

	if (KEY("height")) {
	    if (sscanf(data, "%lf", &sb.height) != 1 || sb.height <= 0.) {
		error(key, data, "illegal height request");
	    }
	    else
		continue;
	}

	if (KEY("length")) {
	    if (sscanf(data, "%lf", &sb.length) != 1 || sb.length <= 0.) {
		error(key, data, "illegal length request");
	    }
	    else
		continue;
	}

	if (KEY("units")) {
	    G_strip(data);
	    if (strcmp(data, "auto") == 0) {
		sb.units = SB_UNITS_AUTO;
		continue;
	    }
	    else if (G_projection() == PROJECTION_XY) {
		error(key, data,
		  "Earth units not available in simple XY location");
	    }
	    else if (strcmp(data, "meters") == 0) {
		sb.units = SB_UNITS_METERS;
		continue;
	    }
	    else if (strcmp(data, "kilometers") == 0 || strcmp(data, "km") == 0) {
		sb.units = SB_UNITS_KM;
		continue;
	    }
	    else if (strcmp(data, "feet") == 0) {
		sb.units = SB_UNITS_FEET;
		continue;
	    }
	    else if (strcmp(data, "miles") == 0) {
		sb.units = SB_UNITS_MILES;
		continue;
	    }
	    else if (strcmp(data, "nautmiles") == 0 || strcmp(data, "nm") == 0) {
		sb.units = SB_UNITS_NMILES;
		continue;
	    }
	    else
		error(key, data, "illegal units request");
	}

	if (KEY("segment")) {
	    if (sscanf(data, "%d", &sb.segment) != 1 || sb.segment <= 0) {
		error(key, data, "illegal segment request");
	    }
	    else
		continue;
	}

	if (KEY("numbers")) {
	    if (sscanf(data, "%d", &sb.numbers) != 1 || sb.numbers <= 0) {
		error(key, data, "illegal numbers request");
	    }
	    else
		continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    G_free(sb.font);
	    sb.font = G_store(data);
	    continue;
	}

	if (KEY("fontsize")) {
	    if (sscanf(data, "%d", &sb.fontsize) != 1 || sb.fontsize <= 0) {
		error(key, data, "illegal fontsize request");
	    }
	    else
		continue;
	}

	if (KEY("color"))
	{
	    sb.color = get_color_number(data);
	    if (sb.color < 0)
	    {
		sb.color = BLACK;
		error(key, data, "illegal color request");
	    }
	    continue;
	}

	if (KEY("background")) {
	    sb.bgcolor = yesno(key, data);
	    continue;
	}

	if (KEY("width")) {
	    sb.width = -1.;
	    ch = ' ';
	    if ((sscanf(data, "%lf%c", &sb.width, &ch) < 1) ||
		(sb.width < 0.)) {
		sb.width = 1.;
		error(key, data, "illegal grid width request");
	    }
	    if (ch == 'i')
		sb.width = sb.width * 72.0;
	    continue;
	}

	error(key, data, "illegal request (scalebar)");

    }

    return 0;

}
