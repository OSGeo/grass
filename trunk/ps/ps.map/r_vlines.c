/* Function: vectfile
 **
 ** This PostScript version is just slightly modified p.map code.
 **
 ** Modified by: Paul W. Carlson                March 1992
 ** Modified by: Janne Soimasuo August 1994 line_cat added
 ** Modified by: Radim Blazek Jan 2000 acolor, label added
 */
#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include "vector.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "color       color" "rgbcolumn   column",
    "width       #",
    "cwidth      #",
    "hcolor      color",
    "hwidth      #",
    "offset      #",
    "coffset     #",
    "masked      [y|n]",
    "style       solid|dashed|dotted|dashdotted|[0|1]...",
    "linecap     butt|round|extended_butt"
    "line_cat    #",
    "acolor      r g b",
    "label       label",
    "lpos        0|1-20",
    "ref         left|right",
    "pat         EPS pattern file",
    "scale       #",
    "pwidth      #",
    ""
};

int read_vlines(char *name, char *mapset)
{
    char fullname[GNAME_MAX];
    char buf[1024];
    char *key, *data, *dp;
    double width;
    int itmp, vec;
    int r, g, b;
    int ret;
    struct Map_info Map;

    vector_alloc();		/* allocate space */

    sprintf(fullname, "%s in %s", name, mapset);

    Vect_set_open_level(2);
    if (2 > Vect_open_old(&Map, name, mapset)) {
	error(fullname, "", "can't open vector map");
	gobble_input();
	return 0;
    }
    Vect_close(&Map);

    vec = vector.count;

    vector.layer[vec].type = VLINES;
    vector.layer[vec].name = G_store(name);
    vector.layer[vec].mapset = G_store(mapset);
    vector.layer[vec].ltype = GV_LINE;
    vector.layer[vec].masked = 0;

    vector.layer[vec].field = 1;
    vector.layer[vec].cats = NULL;
    vector.layer[vec].where = NULL;

    vector.layer[vec].width = 1.;
    vector.layer[vec].cwidth = 0.;
    vector.layer[vec].offset = 0.;
    vector.layer[vec].coffset = 0.;
    set_color(&(vector.layer[vec].color), 0, 0, 0);
    vector.layer[vec].rgbcol = NULL;
    vector.layer[vec].linestyle = NULL;
    vector.layer[vec].linecap = LINECAP_BUTT;
    vector.layer[vec].ref = LINE_REF_CENTER;
    vector.layer[vec].hwidth = 0.;
    unset_color(&(vector.layer[vec].hcolor));
    vector.layer[vec].label = NULL;
    vector.layer[vec].lpos = -1;
    vector.layer[vec].pwidth = 1.;


    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("masked")) {
	    vector.layer[vec].masked = yesno(key, data);
	    if (vector.layer[vec].masked)
		PS.mask_needed = 1;
	    continue;
	}

	if (KEY("type")) {
	    G_strip(data);
	    vector.layer[vec].ltype = 0;

	    if (strstr(data, "line"))
		vector.layer[vec].ltype |= GV_LINE;

	    if (strstr(data, "boundary"))
		vector.layer[vec].ltype |= GV_BOUNDARY;

	    continue;
	}

	if (KEY("layer")) {
	    G_strip(data);
	    vector.layer[vec].field = atoi(data);
	    continue;
	}

	if (KEY("cats")) {
	    G_strip(data);
	    vector.layer[vec].cats = G_store(data);
	    continue;
	}

	if (KEY("where")) {
	    G_strip(data);
	    vector.layer[vec].where = G_store(data);
	    continue;
	}

	if (KEY("style")) {
	    G_strip(data);
	    if (strcmp(data, "solid") == 0) {
		vector.layer[vec].linestyle = NULL;
		continue;
	    }
	    else if (strcmp(data, "dashed") == 0) {
		vector.layer[vec].linestyle = G_store("000000111");
		continue;
	    }
	    else if (strcmp(data, "dotted") == 0) {
		vector.layer[vec].linestyle = G_store("100000");
		continue;
	    }
	    else if (strcmp(data, "dashdotted") == 0) {
		vector.layer[vec].linestyle = G_store("000000111011111");
		continue;
	    }
	    for (dp = data; *dp; dp++)
		if (*dp < '0' || *dp > '9')
		    break;
	    if (*dp != 0 || dp == data) {
		error(key, data, "illegal line style (vlines)");
		continue;
	    }
	    vector.layer[vec].linestyle = G_store(data);
	    continue;
	}

	if (KEY("linecap")) {
	    G_strip(data);
	    if (strcmp(data, "butt") == 0) {
		vector.layer[vec].linecap = LINECAP_BUTT;
		continue;
	    }
	    else if (strcmp(data, "round") == 0) {
		vector.layer[vec].linecap = LINECAP_ROUND;
		continue;
	    }
	    else if (strcmp(data, "extended_butt") == 0) {
		vector.layer[vec].linecap = LINECAP_EXTBUTT;
		continue;
	    }
	    else
		error(key, data, "illegal line cap (vlines)");		
	    continue;
	}

	if (KEY("width")) {
	    width = -1.;
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width (vlines)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].width = width;
	    continue;
	}

	if (KEY("cwidth")) {
	    width = -1.;
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal cwidth (vlines)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].cwidth = width;
	    continue;
	}

	if (KEY("offset")) {
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1) {
		width = 0.;
		error(key, data, "illegal offset (vlines)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].offset = width;
	    continue;
	}

	if (KEY("coffset")) {
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1) {
		width = 0.;
		error(key, data, "illegal coffset (vlines)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].coffset = width;
	    continue;
	}

	if (KEY("hwidth")) {
	    width = -1.;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1 || width < 0.) {
		width = 0.;
		error(key, data, "illegal hwidth (vlines)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].hwidth = width;
	    continue;
	}

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&(vector.layer[vec].color), r, g, b);
	    else if (ret == 2)
		unset_color(&(vector.layer[vec].color));
	    else
		error(key, data, "illegal color request (vlines)");

	    continue;
	}

	if (KEY("rgbcolumn")) {
	    G_strip(data);
	    vector.layer[vec].rgbcol = G_store(data);
	    continue;
	}

	if (KEY("hcolor")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&(vector.layer[vec].hcolor), r, g, b);
	    else if (ret == 2)
		unset_color(&(vector.layer[vec].hcolor));
	    else
		error(key, data, "illegal hcolor request (vlines)");

	    continue;
	}

	if (KEY("label")) {	/* map legend label */
	    G_strip(data);
	    vector.layer[vec].label = G_store(data);
	    continue;
	}

	if (KEY("lpos")) {
	    if (sscanf(data, "%d", &itmp) < 1 || itmp < 0) {
		itmp = -1;
		error(key, data, "illegal lpos (vlines)");
		continue;
	    }
	    vector.layer[vec].lpos = itmp;
	    continue;
	}

	if (KEY("ref")) {
	    G_strip(data);
	    if (strcmp(data, "left") == 0) {
		vector.layer[vec].ref = LINE_REF_LEFT;
		continue;
	    }
	    if (strcmp(data, "right") == 0) {
		vector.layer[vec].ref = LINE_REF_RIGHT;
		continue;
	    }
	    error(key, data, "illegal ref request (vlines)");
	    continue;
	}

	if (KEY("scale")) {
	    G_strip(data);
	    vector.layer[vec].scale = atof(data);
	    continue;
	}
	error(key, "", "illegal request (vlines)");
    }

    vector.count++;
    return 1;
}
