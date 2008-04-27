/* Function: ctablfile
**
** Author: Paul W. Carlson	April 1992
*/

#include <stdlib.h>
#include <string.h>
#include "colortable.h"
#include "ps_info.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] =
{
    "where      x y",
    "width      table_width",
    "height     fptable_height",
    "raster	raster_name",
    "range	min max",
    "cols       columns",
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    "nodata     Y|n",
    "tickbar    y|N"
    ""
};

int 
read_colortable (void)
{	
    char buf[1024];
    char *key, *data;
    char name[100], mapset[50];
    int color, fontsize, cols, nodata, tickbar;
    double w, h, x, y;
    int range_override;
    double min,max,tmpD;


    fontsize = 0;
    color = BLACK;
    cols = 1;
    h = w = x = y = 0.0;
    ct.nodata = 1;
    ct.tickbar = 0;
    range_override = FALSE;

    while (input(2, buf, help))
    {
	if (!key_data(buf, &key, &data)) continue;

        if (KEY("where"))
 	{
	    if (sscanf(data, "%lf %lf", &x, &y) != 2)
	    {
		x = y = 0.0;
		error(key, data, "illegal where request");
	    }
	    else continue;
	}

        if (KEY("width"))
 	{
	    if (sscanf(data, "%lf", &w) != 1 || w <= 0 )
	    {
		error(key, data, "illegal width request");
	    }
	    else continue;
	}
	
        if (KEY("height"))
 	{
	    if (sscanf(data, "%lf", &h) != 1 || h <= 0 )
	    {
		error(key, data, "illegal height request");
	    }
	    else continue;
	}

	if (KEY("raster"))
	{
		if (scan_gis("cell", "raster", key, data, name, mapset, 0)) {
			ct.name = G_store(name);
			ct.mapset = G_store(mapset);
			continue;
		}
	}

	if (KEY("range"))
	{
	    if (sscanf(data, "%lf %lf", &min, &max) != 2)
	    {
		range_override = FALSE;
		error(key, data, "illegal range request");
	    }
	    else
	    {
		range_override = TRUE;
		if(min > max) { /* flip if needed */
		    tmpD = min;
		    min = max;
		    max = tmpD;
		}
		continue;
	    }
	}

        if (KEY("cols"))
 	{
	    if (sscanf(data, "%d", &cols) != 1)
	    {
		cols= 1;
		error(key, data, "illegal columns request");
	    }
	    else continue;
	}

	if (KEY("fontsize"))
	{
	    fontsize = atoi(data);
	    if (fontsize < 4 || fontsize > 50) fontsize = 0;
	    continue;
	}

	if (KEY("color"))
	{
	    color = get_color_number(data);
	    if (color < 0)
	    {
		color = BLACK;
		error(key, data, "illegal color request");
	    }
	    continue;
	}

	if (KEY("font"))
	{
	    get_font(data);
	    ct.font = G_store(data);
	    continue;
	}
	if (KEY("nodata"))
	{
	    nodata = yesno(key, data);
	    ct.nodata = nodata;
	    continue;
        }
	if (KEY("tickbar"))
	{
	    tickbar = yesno(key, data);
	    ct.tickbar = tickbar;
	    continue;
        }

	error(key, data, "illegal colortabe sub-request");
    }
    ct.x = x;
    ct.y = y;
    if (fontsize) ct.fontsize = fontsize;

    /* Check for Raster */
    if (!ct.name) {
	    if ( !PS.cell_name ) {
		    error(key, data, "No raster selected for colortable !");
	    } else {
		    ct.name = PS.cell_name;
		    ct.mapset = PS.cell_mapset;
	    }
    }

    ct.min = min;
    ct.max = max;
    ct.range_override = range_override;
    ct.width = w;
    ct.height = h;
    ct.color = color;
    ct.cols = cols;

    return 0;
}
