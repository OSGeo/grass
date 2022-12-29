/* Function: ctablfile
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "colortable.h"
#include "clr.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "where      x y",
    "width      table_width",
    "height     fptable_height",
    "lwidth     line_width",
    "raster	raster_name",
    "range	min max",
    "cols       columns",
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    "nodata     Y|n",
    "tickbar    y|N"
    "discrete   y|n"
    ""
};

int read_colortable(void)
{
    char buf[1024];
    char *key, *data;
    char name[GNAME_MAX], mapset[GMAPSET_MAX];
    int fontsize, cols, nodata, tickbar, discrete;
    double w, h, x, y, lw;
    int range_override;
    double min, max, tmpD;
    int r, g, b, ret;
    PSCOLOR color;


    fontsize = 10;
    set_color(&color, 0, 0, 0);
    cols = 1;
    h = w = x = y = 0.0;
    lw = 1;
    ct.font = G_store("Helvetica");
    ct.nodata = TRUE;
    ct.tickbar = FALSE;
    ct.discrete = -1;	    /* default: TRUE for CELL map, FALSE for FP maps */
    range_override = FALSE;

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

	if (KEY("width")) {
	    if (sscanf(data, "%lf", &w) != 1 || w <= 0) {
		error(key, data, _("illegal width request"));
	    }
	    else
		continue;
	}

	if (KEY("height")) {
	    if (sscanf(data, "%lf", &h) != 1 || h <= 0) {
		error(key, data, _("illegal height request"));
	    }
	    else
		continue;
	}

	if (KEY("lwidth")) {
	    if (sscanf(data, "%lf", &lw) != 1 || lw < 0) {
		error(key, data, _("illegal width request"));
	    }
	    else
		continue;
	}

	if (KEY("raster")) {
	    if (scan_gis("cell", "raster", key, data, name, mapset, 0)) {
		ct.name = G_store(name);
		ct.mapset = G_store(mapset);
		continue;
	    }
	}

	if (KEY("range")) {
	    if (sscanf(data, "%lf %lf", &min, &max) != 2) {
		range_override = FALSE;
		error(key, data, _("illegal range request"));
	    }
	    else {
		range_override = TRUE;
		if (min > max) {	/* flip if needed */
		    tmpD = min;
		    min = max;
		    max = tmpD;
		}
		continue;
	    }
	}

	if (KEY("cols")) {
	    if (sscanf(data, "%d", &cols) != 1) {
		cols = 1;
		error(key, data, _("illegal columns request"));
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
	    else if (ret == 2)	/* i.e. "none" */
		/* unset_color(&color); */
		error(key, data, _("Unsupported color request (colortable)"));
	    else
		error(key, data, _("illegal color request (colortable)"));

	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    ct.font = G_store(data);
	    continue;
	}
	if (KEY("nodata")) {
	    nodata = yesno(key, data);
	    ct.nodata = nodata;
	    continue;
	}
	if (KEY("tickbar")) {
	    tickbar = yesno(key, data);
	    ct.tickbar = tickbar;
	    continue;
	}

	if (KEY("discrete")) {
	    discrete = yesno(key, data);
	    ct.discrete = discrete;
	    continue;
	}

	error(key, data, _("illegal colortable sub-request"));
    }

    ct.x = x;
    ct.y = y;
    if (fontsize)
	ct.fontsize = fontsize;

    /* Check for Raster */
    if (!ct.name) {
	if (!PS.cell_name) {
	    error(key, data, _("No raster selected for colortable!"));
	}
	else {
	    ct.name = PS.cell_name;
	    ct.mapset = PS.cell_mapset;
	}
    }

    /* set default if legend type was not specified */
    if (ct.discrete == -1) {
	if (Rast_map_is_fp(ct.name, ct.mapset))
	    ct.discrete = FALSE;
	else
	    ct.discrete = TRUE;
    }

    ct.min = min;
    ct.max = max;
    ct.range_override = range_override;
    ct.width = w;
    ct.height = h;
    ct.lwidth = lw;
    ct.color = color;
    ct.cols = cols;

    return 0;
}
