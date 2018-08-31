/* Function: ps_map
 **
 ** This function writes the PostScript output file.
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include <stdio.h>
#include <unistd.h>
#include "vector.h"
#include "border.h"
#include "colortable.h"
#include "local_proto.h"

extern int do_mapinfo;
extern int do_vlegend;
extern int eps_output;
extern int rotate_plot;
extern int ps_copies;

int ps_map(void)
{
    long current_offset;
    const char *date;
    int urx, ury;

    /* get date */
    date = G_date();

    /* write the PostScript header */
    write_PS_header();

    /* create the PostScript procs */
    make_procs();

    /* set number of copies */
    if (ps_copies > 1)
	fprintf(PS.fp, "/#copies %d def\n", ps_copies);

    /* Set page size */
    if (!eps_output) {
	if (!rotate_plot) {
	    urx = (int)72.0 *PS.page_width;
	    ury = (int)72.0 *PS.page_height;
	}
	else {
	    urx = (int)72.0 *PS.page_height;
	    ury = (int)72.0 *PS.page_width;
	}
	fprintf(PS.fp, "<< /PageSize [  %d %d ] >> setpagedevice\n", urx,
		ury);
    }

    /* rotate map? */
    if (rotate_plot) {
	fprintf(PS.fp, "%.2f 0.0 TR\n", 72.0 * PS.page_height);
	fprintf(PS.fp, "90 rotate\n");
    }

    /* do the map header */
    if (PS.do_header)
	do_map_header(date);

    /* size the map */
    map_setup();

    /* do the raster stuff, if any */
    if (PS.do_raster || grp.do_group)
	PS_raster_plot();

    /* do the outline, if requested */
    if (PS.do_outline)
	ps_outline();

    /* do the masked vector plots, if any */
    if (vector.count) {
	do_vectors(0);
	do_vpoints(0);
    }

    /* do the masked points/lines, if any */
    do_plt(0);

    /* do masking, if required */
    PS_make_mask();
    if (PS.mask_needed)
	do_masking();

    /* do the unmasked vector plots, if any */
    if (vector.count)
	do_vectors(1);

    /* do the grid, if any */
    if (PS.grid_cross)
	do_grid_cross();
    else
	do_grid();

    /* do geo-grid, if any */
    do_geogrid();

    /* do the grid numbers, if any */
    if (PS.grid_numbers > 0)
	do_grid_numbers();
    if (PS.geogrid_numbers > 0)
	do_geogrid_numbers();

    /* do the labels from paint/labels, if any */
    do_labels(0);

    /* restore the unclipped graphics state */
    fprintf(PS.fp, "grestore ");

    /* do the unmasked vector points, if any */
    if (vector.count)
	do_vpoints(1);

    /* do the unmasked points, lines and eps if any */
    do_plt(1);

    /* do the labels specified in script file */
    do_labels(1);

    /* show the map info */
    if (do_mapinfo)
	map_info();

    /* show the vector legend */
    if (do_vlegend && vector.count)
	PS_vlegend();

    /* Make scalebar */
    if (PS.do_scalebar)
	do_scalebar();

    /* put border around map */
    if (PS.do_border && brd.r >= 0.) {	/* if color wasn't "none" */
	fprintf(PS.fp, "%.3f %.3f %.3f C\n", brd.r, brd.g, brd.b);
	fprintf(PS.fp, "%.8f W\n", brd.width);
	box_draw(PS.map_top - 0.5, PS.map_bot + 0.5,
		 PS.map_left + 0.5, PS.map_right - 0.5);
    }

    /* do the colortable, if requested */
    if (PS.do_colortable) {
	if (ct.discrete == TRUE)
	    PS_colortable();
	else
	    PS_fcolortable();
    }

    /* do comments, if any */
    if (PS.commentfile != NULL)
	do_comment();

    /* do any PostScript include files */
    if (PS.num_psfiles)
	do_psfiles();

    /* write the bounding box */
    current_offset = G_ftell(PS.fp);
    write_bounding_box();
    G_fseek(PS.fp, current_offset, SEEK_SET);

    fprintf(PS.fp, "showpage\n");
    fprintf(PS.fp, "%%%%Trailer\n");
    fprintf(PS.fp, "%%%%EOF\n");
    fclose(PS.fp);

    return 0;
}
