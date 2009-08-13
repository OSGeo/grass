/* Function: write_PS_header
 **
 ** Author: Paul W. Carlson     3/92
 */

#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "ps_info.h"

static long bb_offset;
extern int eps_output;
extern int rotate_plot;

int write_PS_header(void)
{
    struct Categories cats;
    int cats_ok;

    if (PS.do_raster)
	cats_ok = Rast_read_cats(PS.cell_name, PS.cell_mapset, &cats) >= 0;

    /* write PostScript header */
    /*fprintf(PS.fp, "%%!PS-Adobe-2.0 EPSF-1.2\n"); */
    if (eps_output)
	fprintf(PS.fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");
    else
	fprintf(PS.fp, "%%!PS-Adobe-3.0\n");

    bb_offset = G_ftell(PS.fp);
    fprintf(PS.fp, "                                       ");
    fprintf(PS.fp, "                                       \n");
    fprintf(PS.fp, "%%%%Title: ");
    if (PS.do_raster && cats_ok)
	fprintf(PS.fp, "%s\n", cats.title);
    else
	fprintf(PS.fp, "Mapset = %s\n", PS.cell_mapset);
    fprintf(PS.fp, "%%%%EndComments\n");

    return 0;
}

int write_bounding_box(void)
{
    int llx, lly, urx, ury;

    if (!rotate_plot) {
	/*
	   llx = (int) 72.0 * PS.left_marg;
	   lly = (int) 72.0 * PS.bot_marg;
	   urx = (int) 72.0 * (PS.page_width - PS.right_marg);
	   ury = (int) 72.0 * (PS.page_height - PS.top_marg);
	 */
	/* BBox should be as above but then such box is displayed in gv and I think that people
	 * expect full page. Also id rectangle is drawn around printable area half of such
	 * line would be clipped out */
	llx = 0;
	lly = 0;
	urx = (int)72.0 *PS.page_width;
	ury = (int)72.0 *PS.page_height;
    }
    else {
	llx = 0;
	lly = 0;
	urx = (int)72.0 *PS.page_height;
	ury = (int)72.0 *PS.page_width;
    }

    G_fseek(PS.fp, bb_offset, SEEK_SET);
    fprintf(PS.fp, "%%%%BoundingBox: %d %d %d %d", llx, lly, urx, ury);

    return 0;
}
