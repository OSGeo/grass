/* Function: vectfile
 **
 ** This PostScript version is just slightly modified p.map code.
 **
 ** Modified by: Paul W. Carlson                March 1992
 ** Modified by: Janne Soimasuo August 1994 line_cat added
 ** Modified by: Radim Blazek Jan 2000 acolor, label added
 ** Modified by: Hamish Bowman Sept 2005, sizecol, scale added
 */
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "vector.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "masked     [y|n]",
    "color      color",
    "fcolor     color",
    "rgbcolumn  column",
    "icon       iconfile",
    "eps        epsfile",
    "size       #",
    "sizecolumn column",
    "layer      #",
    "scale      factor",
    "rotate     #",
    "rotatecolumn  column",
    "font       fontname",
    "label      label",
    "lpos       #",
    ""
};

int read_vpoints(char *name, char *mapset)
{
    char fullname[100];
    char buf[1024];
    char *key, *data;
    double width, size, scale, rotate;
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

    vector.layer[vec].type = VPOINTS;
    vector.layer[vec].name = G_store(name);
    vector.layer[vec].mapset = G_store(mapset);
    vector.layer[vec].ltype = GV_POINT;
    vector.layer[vec].masked = 0;

    vector.layer[vec].field = 1;
    vector.layer[vec].cats = NULL;
    vector.layer[vec].where = NULL;

    vector.layer[vec].width = 1.;
    set_color(&(vector.layer[vec].color), 0, 0, 0);
    set_color(&(vector.layer[vec].fcolor), 255, 0, 0);
    vector.layer[vec].rgbcol = NULL;

    vector.layer[vec].label = NULL;
    vector.layer[vec].lpos = -1;
    vector.layer[vec].symbol = G_store("basic/diamond");

    vector.layer[vec].size = 6.0;
    vector.layer[vec].sizecol = NULL;
    vector.layer[vec].scale = 1.0;

    vector.layer[vec].rotate = 0.0;
    vector.layer[vec].rotcol = NULL;
    vector.layer[vec].epstype = 0;


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

	    if (strstr(data, "point"))
		vector.layer[vec].ltype |= GV_POINT;

	    if (strstr(data, "centroid"))
		vector.layer[vec].ltype |= GV_CENTROID;

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

	if (KEY("width")) {
	    width = -1.;
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width (vpoints)");
		continue;
	    }
	    if (mapset[0] == 'i')
		width = width / 72.;
	    vector.layer[vec].width = width;
	    continue;
	}

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&(vector.layer[vec].color), r, g, b);
	    else if (ret == 2)
		unset_color(&(vector.layer[vec].color));
	    else
		error(key, data, "illegal color request");

	    continue;
	}

	if (KEY("fcolor")) {	/* fill color */
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&(vector.layer[vec].fcolor), r, g, b);
	    else if (ret == 2)
		unset_color(&(vector.layer[vec].fcolor));
	    else
		error(key, data, "illegal color request (vpoints)");

	    continue;
	}

	if (KEY("rgbcolumn")) {
	    G_strip(data);
	    vector.layer[vec].rgbcol = G_store(data);
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
		error(key, data, "illegal lpos");
		continue;
	    }
	    vector.layer[vec].lpos = itmp;
	    continue;
	}

	if (KEY("symbol")) {
	    /* TODO: test here if isymbol exists */
	    vector.layer[vec].symbol = G_store(data);
	    continue;
	}

	if (KEY("eps")) {
	    char *cc;

	    G_chop(data);
	    vector.layer[vec].epspre = G_store(data);

	    /* epstype: 0 - no eps, 1 - common eps, 2 - eps for each category */
	    vector.layer[vec].epstype = 1;

	    /* find dynamic filename by cat number character */
	    /* pre is filename before the $, suf is filename after the $ */
	    cc = (char *)strchr(vector.layer[vec].epspre, '$');
	    if (cc != NULL) {
		*cc = '\0';
		vector.layer[vec].epssuf = G_store(cc + sizeof(char));
		vector.layer[vec].epstype = 2;

		G_debug(2, "epstype=%d, pre=[%s], suf=[%s]",
			vector.layer[vec].epstype, vector.layer[vec].epspre,
			vector.layer[vec].epssuf);
	    }
	    else {
		G_debug(2, "epstype=%d, eps file=[%s]",
			vector.layer[vec].epstype, vector.layer[vec].epspre);
	    }
	    continue;
	}

	if (KEY("size")) {
	    if (sscanf(data, "%lf", &size) != 1 || size <= 0.0) {
		size = 1.0;
		error(key, data, "illegal size request (vpoints)");
	    }
	    vector.layer[vec].size = size;
	    continue;
	}

	if (KEY("sizecol") || KEY("sizecolumn")) {
	    G_strip(data);
	    vector.layer[vec].sizecol = G_store(data);
	    continue;
	}

	if (KEY("scale")) {
	    if (sscanf(data, "%lf", &scale) != 1 || scale <= 0.0) {
		scale = 1.0;
		error(key, data, "illegal scale request (vpoints)");
	    }
	    vector.layer[vec].scale = scale;
	    continue;
	}

	if (KEY("rotate")) {
	    if (sscanf(data, "%lf", &rotate) != 1) {
		rotate = 0.0;
		error(key, data, "illegal rotation request (vpoints)");
	    }
	    vector.layer[vec].rotate = rotate;
	    continue;
	}

	if (KEY("rotatecolumn")) {
	    G_strip(data);
	    vector.layer[vec].rotcol = G_store(data);
	    continue;
	}

	error(key, "", "illegal request (vpoints)");
    }

    vector.count++;
    return 1;
}
