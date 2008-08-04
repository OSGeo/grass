#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vask.h>
#include "local_proto.h"

/*
 * ask_format()
 *
 * RETURN: 0 success : -1 failure
 */
int ask_format(char *name, struct Cell_head *cellhd, off_t filesize)
{
    RASTER_MAP_TYPE maptype;
    char title[80];
    char buf[80];
    char no_zeros[80];

    G_zero(no_zeros, (int)sizeof(no_zeros));
    maptype = G_raster_map_type(name, G_mapset());

    G_snprintf(title, sizeof(title), _("Please enter the following "
				       "information for [%s]:"), name);

    V_clear();
    V_line(0, title);
    V_line(2, _("        Number of rows"));
    V_line(3, _("        Number of cols"));
    V_line(4, (maptype == CELL_TYPE ?
	       _("        Number of bytes per cell") :
	       _("        Floating point map")));

    if (cellhd->compressed)
	V_const(&cellhd->rows, 'i', 2, 1, 5);
    else
	V_ques(&cellhd->rows, 'i', 2, 1, 5);

    V_ques(&cellhd->cols, 'i', 3, 1, 5);

    if (maptype == CELL_TYPE)
	V_ques(&cellhd->format, 'i', 4, 1, 5);

    /* If not compressed and filesize mismatch */
    if (maptype == CELL_TYPE && cellhd->compressed == 0 &&
	(off_t) cellhd->rows * cellhd->cols * cellhd->format != filesize) {

	if (sizeof(off_t) > sizeof(long) && filesize > ULONG_MAX)
	    filesize = 0;
	G_snprintf(buf, sizeof(buf), _("rows * cols * bytes per cell "
				       "must be same as file size (%lu)"),
		   (unsigned long)filesize);
	V_line(6, buf);
	V_line(7, _("If you need help figuring them out, just hit ESC"));
    }
    V_line(10, no_zeros);

    while (1) {
	V_intrpt_ok();
	if (!V_call())
	    return -1;

	G_zero(no_zeros, (int)sizeof(no_zeros));
	if (maptype == CELL_TYPE && cellhd->rows > 0 &&
	    cellhd->cols > 0 && cellhd->format > 0)
	    break;

	if (maptype != CELL_TYPE && cellhd->rows > 0 && cellhd->cols > 0)
	    break;

	if (!cellhd->compressed) {
	    if (cellhd->rows >= 0 && cellhd->cols >= 0 && cellhd->format >= 0)
		break;

	    strcpy(no_zeros, _("** Negative values not allowed!"));
	}
	else
	    strcpy(no_zeros, _("** Positive values only please!"));
    }

    return EXIT_SUCCESS;
}
