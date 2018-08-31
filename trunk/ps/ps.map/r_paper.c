/* Function: infofile
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include "map_info.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "width      #",
    "height     #",
    "left       #",
    "right      #",
    "top        #",
    "bottom     #",
    ""
};

int read_paper(void)
{
    char buf[1024];
    char *key, *data;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("width")) {
	    PS.page_width = atof(data);
	    if (PS.page_width <= 0) {
		error(key, data, "illegal paper width request");
	    }
	    continue;
	}
	if (KEY("height")) {
	    PS.page_height = atof(data);
	    if (PS.page_height <= 0) {
		error(key, data, "illegal paper height request");
	    }
	    continue;
	}
	if (KEY("left")) {
	    PS.left_marg = atof(data);
	    if (PS.left_marg < 0) {
		error(key, data, "illegal paper left margin request");
	    }
	    continue;
	}
	if (KEY("right")) {
	    PS.right_marg = atof(data);
	    if (PS.right_marg < 0) {
		error(key, data, "illegal paper right margin request");
	    }
	    continue;
	}
	if (KEY("top")) {
	    PS.top_marg = atof(data);
	    if (PS.top_marg < 0) {
		error(key, data, "illegal paper top margin request");
	    }
	    continue;
	}
	if (KEY("bottom")) {
	    PS.bot_marg = atof(data);
	    if (PS.bot_marg < 0) {
		error(key, data, "illegal paper bottom margin request");
	    }
	    continue;
	}

	error(key, data, "illegal paper sub-request");
    }

    return 0;
}
