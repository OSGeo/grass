/* Function: hdrfile
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include "header.h"
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
    int color, fontsize;

    fontsize = 0;
    color = BLACK;
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
	    color = get_color_number(data);
	    if (color < 0) {
		color = BLACK;
		error(key, data, "illegal color request");
	    }
	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    hdr.font = G_store(data);
	    continue;
	}
	error(key, data, "illegal header sub-request");
    }
    hdr.color = color;
    if (fontsize)
	hdr.fontsize = fontsize;

    return 0;
}
