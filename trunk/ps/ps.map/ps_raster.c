/* Functions: PS_raster_plot, ps_get_map_row
 **
 ** Author: Paul W. Carlson     3/92
 ** 
 ** ps_get_map_row is substituted by Rast_get_c_row_nomask
 ** writing mask file is done separately by function ps_write_mask_row
 ** which used code previously in ps_get_map_row. This is done because
 ** sometimes the raster map is not drawn, but we still need a mask
 ** so temporary mask file is created by new function PS_make_mask()
 ** which calls ps_write_mask_row()
 ** These changes are made by Olga Waupotitsch 4/94
 */
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

static FILE *ps_mask_fp;
extern char *ps_mask_file;

int PS_make_mask(void)
{
    int row;
    CELL *maskbuf;
    int maskfd, r, g, b;

    maskfd = Rast_maskfd();
    if (maskfd < 0)
	/* there is no mask */
    {
	if (PS.mask_needed)
	    PS.mask_needed = 0;
	return 0;
    }
    if (maskfd >= 0)
	maskbuf = Rast_allocate_c_buf();

    /* if masked, open a file to hold the PostScript mask data */
    if (maskfd >= 0 && PS.mask_needed) {
	if ((ps_mask_fp = fopen(ps_mask_file, "w")) == NULL)
	    G_fatal_error(_("Can't create temporary PostScript mask file."));

	/* get no data rgb values for mask */
	Rast_get_null_value_color(&r, &g, &b, &PS.colors);
	PS.r0 = (double)r / 255.0;
	PS.g0 = (double)g / 255.0;
	PS.b0 = (double)b / 255.0;

	for (row = 0; row < PS.w.rows; row++) {
	    Rast_get_c_row_nomask(maskfd, maskbuf, row);
	    ps_write_mask_row(maskbuf);
	}
	fclose(ps_mask_fp);
	G_free(maskbuf);

    }
    return 0;
}

int PS_raster_plot(void)
{
    int i, n, r, g, b, rr, gg, bb, row, col, doing_color;
    RASTER_MAP_TYPE map_type, grp_map_type[3];
    void *cellbuf = NULL, *cbuf[3], *ptr;

    if (!PS.do_raster && !grp.do_group)
	return 1;

    /* are we doing color? */
    doing_color = (PS.grey == 0 && PS.level == 2);

    /* save graphics state */
    fprintf(PS.fp, "gsave\n");

    /* make variables for cells_wide and cells_high */
    fprintf(PS.fp, "/cw %d def /ch %d def\n", PS.cells_wide, PS.cells_high);

    /* set lower left corner of map */
    fprintf(PS.fp, "%.2f %.2f TR\n", PS.map_left, PS.map_bot);

    /* mapping of image to map_pix_wide x map_pix_high unit rectangle */
    fprintf(PS.fp, "%d %d scale\n",
	    (int)(PS.map_pix_wide + 0.5), (int)(PS.map_pix_high + 0.5));


    /* make strings to hold image RGB values */
    if (doing_color)
	fprintf(PS.fp, "/imgstrg cw 3 mul string def\n");
    else
	fprintf(PS.fp, "/imgstrg cw string def\n");
    fprintf(PS.fp, "cw ch 8\n");
    fprintf(PS.fp, "[cw 0 0 ch neg 0 ch]\n");
    fprintf(PS.fp, "{currentfile imgstrg readhexstring pop}\n");
    if (doing_color)
	fprintf(PS.fp, "false 3 colorimage\n");
    else
	fprintf(PS.fp, "image\n");

    /* let user know what's happenning */
    if (PS.do_raster)
	G_message(_("Reading raster map <%s>..."),
		  G_fully_qualified_name(PS.cell_name, PS.cell_mapset));
    else
	G_message(_("Reading raster maps in group <%s>..."),
		  grp.group_name);

    /* build the image RGB string */
    if (PS.do_raster) {
	map_type = Rast_get_map_type(PS.cell_fd);
	cellbuf = Rast_allocate_buf(map_type);
	n = 0;
	for (row = 0; row < PS.w.rows; row++) {
	    Rast_get_row(PS.cell_fd, cellbuf, row, map_type);
	    if ((row % PS.row_delta) == 0) {
		ptr = cellbuf;
		for (col = 0; col < PS.w.cols; col += PS.col_delta) {
		    Rast_get_color(ptr, &r, &g, &b, &PS.colors, map_type);

		    /* if color raster */
		    if (doing_color) {
			fprintf(PS.fp, "%02X%02X%02X", r, g, b);
			if (++n == 13) {
			    n = 0;
			    fprintf(PS.fp, "\n");
			}
		    }

		    /* if grey raster */
		    else {
			fprintf(PS.fp, "%02X",
				(int)(.3 * (double)r + .59 * (double)g +
				      .11 * (double)b));
			if (++n == 39) {
			    n = 0;
			    fprintf(PS.fp, "\n");
			}
		    }
		    ptr =
			G_incr_void_ptr(ptr,
					Rast_cell_size(map_type) *
					PS.col_delta);
		}
	    }
	}
    }
    else {
	void *cptr[3];

	for (i = 0; i < 3; i++) {
	    grp_map_type[i] = Rast_get_map_type(grp.fd[i]);
	    cbuf[i] = Rast_allocate_buf(grp_map_type[i]);
	}
	n = 0;
	for (row = 0; row < PS.w.rows; row++) {
	    for (i = 0; i < 3; i++) {
		Rast_get_row(grp.fd[i], cbuf[i], row, grp_map_type[i]);
		cptr[i] = cbuf[i];
	    }

	    if ((row % PS.row_delta) == 0) {
		for (col = 0; col < PS.w.cols; col += PS.col_delta) {
		    for (i = 0; i < 3; i++) {
			Rast_get_color(cptr[i], &rr, &gg, &bb,
					   &(grp.colors[i]), grp_map_type[i]);
			if (i == 0)
			    r = rr;
			if (i == 1)
			    g = gg;
			if (i == 2)
			    b = bb;
			cptr[i] = G_incr_void_ptr(cptr[i],
						  Rast_cell_size(grp_map_type
								[0]) *
						  PS.col_delta);
		    }

		    /* if color raster */
		    if (doing_color) {
			fprintf(PS.fp, "%02X%02X%02X", r, g, b);
			if (++n == 13) {
			    n = 0;
			    fprintf(PS.fp, "\n");
			}
		    }
		}
	    }
	}
    }
    fprintf(PS.fp, "\n");

    /* we're done with the cell stuff */
    if (PS.do_raster) {
	if (!PS.do_colortable)
	    Rast_free_colors(&PS.colors);
	Rast_close(PS.cell_fd);
	G_free(cellbuf);
    }
    else {
	for (i = 0; i < 3; i++) {
	    Rast_free_colors(&(grp.colors[i]));
	    Rast_close(grp.fd[i]);
	    G_free(cbuf[i]);
	}
	I_free_group_ref(&grp.ref);
    }
    /* restore graphics state */
    fprintf(PS.fp, "grestore\n");

    return 0;
}


int ps_write_mask_row(register CELL * mask)
{
    register int n;
    int i, j, byte;
    static int bit[] = { 128, 64, 32, 16, 8, 4, 2, 1 };

    i = 0;
    j = 0;
    byte = 0;
    n = PS.w.cols;
    while (n-- > 0) {
	if (*mask++ == 0)
	    byte |= bit[i];
	i++;
	if (i == 8) {
	    i = 0;
	    j++;
	    fprintf(ps_mask_fp, "%02X", byte);
	    if (j == 39) {
		fprintf(ps_mask_fp, "\n");
		j = 0;
	    }
	    byte = 0;
	}
    }
    if (i) {
	while (i < 8) {
	    if (*(mask - 1) == 0)
		byte |= bit[i];
	    i++;
	}
	fprintf(ps_mask_fp, "%02X", byte);
    }
    fprintf(ps_mask_fp, "\n");

    return 0;
}
