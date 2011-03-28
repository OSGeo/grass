/* Function: infofile
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "map_info.h"
#include "clr.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "where      x y",
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    "background color|none",
    "border     color|none",
    ""
};

int read_info(void)
{
    char buf[1024];
    char *key, *data;
    int fontsize;
    double x, y;
    int r, g, b, ret;
    PSCOLOR color, bgcolor, border;

    fontsize = 0;
    set_color(&color, 0, 0, 0);
    set_color(&bgcolor, 255, 255, 255);
    unset_color(&border);
    x = y = 0.0;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("where")) {
	    if (sscanf(data, "%lf %lf", &x, &y) != 2) {
		x = y = 0.0;
		error(key, data, _("illegal where request"));
	    }
	    else
		continue;
	}

	if (KEY("fontsize")) {
	    fontsize = atoi(data);
	    if (fontsize < 4 || fontsize > 50)
		fontsize = 0;
	    continue;
	}

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

	if (KEY("background")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&bgcolor, r, g, b);
	    else if (ret == 2)  /* i.e. "none" */
		unset_color(&bgcolor);
	    else
		error(key, data, _("illegal bgcolor request"));

	    continue;
	}

	if (KEY("border")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&border, r, g, b);
	    else if (ret == 2)  /* i.e. "none" */
		unset_color(&border);
	    else
		error(key, data, _("illegal border color request"));

	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    m_info.font = G_store(data);
	    continue;
	}
	error(key, data, _("illegal mapinfo sub-request"));
    }
    m_info.x = x;
    m_info.y = y;
    m_info.color = color;
    m_info.bgcolor = bgcolor;
    m_info.border = border;
    if (fontsize)
	m_info.fontsize = fontsize;

    return 0;
}
