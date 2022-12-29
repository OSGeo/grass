/* Function: hdrfile
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "header.h"
#include "clr.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "file       header file",
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    ""
};

int read_header(void)
{
    char buf[1024];
    char *key, *data;
    int fontsize;
    PSCOLOR color;
    int ret, r, g, b;

    fontsize = 0;
    set_color(&color, 0, 0, 0);

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("none")) {
	    PS.do_header = 0;
	    continue;
	}

	if (KEY("file")) {
	    G_strip(data);
	    hdr.fp = fopen(data, "r");
	    if (hdr.fp != NULL)
		hdr.file = G_store(data);
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

	if (KEY("font")) {
	    get_font(data);
	    hdr.font = G_store(data);
	    continue;
	}
	error(key, data, _("illegal header sub-request"));
    }

    hdr.color = color;
    if (fontsize)
	hdr.fontsize = fontsize;

    return 0;
}
