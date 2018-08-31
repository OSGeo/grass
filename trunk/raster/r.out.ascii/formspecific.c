#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>

/* write the GRASS ASCII heading */
int writeGRASSheader(FILE * fp)
{
    struct Cell_head region;
    char buf[128];

    G_get_window(&region);
    G_format_northing(region.north, buf, region.proj);
    fprintf(fp, "north: %s\n", buf);
    G_format_northing(region.south, buf, region.proj);
    fprintf(fp, "south: %s\n", buf);
    G_format_easting(region.east, buf, region.proj);
    fprintf(fp, "east: %s\n", buf);
    G_format_easting(region.west, buf, region.proj);
    fprintf(fp, "west: %s\n", buf);

    fprintf(fp, "rows: %d\n", region.rows);
    fprintf(fp, "cols: %d\n", region.cols);

    return 0;
}

/* write GRASS ASCII GRID */
int write_GRASS(int fd,
		FILE * fp,
		int nrows, int ncols, int out_type, int dp, char *null_str)
{
    int row, col;
    void *ptr, *raster;
    char cell_buf[300];

    raster = Rast_allocate_buf(out_type);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	Rast_get_row(fd, raster, row, out_type);

	for (col = 0, ptr = raster; col < ncols; col++,
	     ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
	    if (!Rast_is_null_value(ptr, out_type)) {
		if (out_type == CELL_TYPE)
		    fprintf(fp, "%d", *((CELL *) ptr));

		else if (out_type == FCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((FCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
		else if (out_type == DCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((DCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
	    }
	    else
		fprintf(fp, "%s", null_str);
	    fprintf(fp, " ");
	}
	fprintf(fp, "\n");
    }

    return (0);
}

int writeMFheader(FILE * fp, int dp, int width, int out_type)
{
    if (out_type == CELL_TYPE)
	fprintf(fp, "INTERNAL  1  (FREE)  -1\n");
    else
	fprintf(fp, "INTERNAL  1.  (%de%d.%d)  -1\n", width, dp + 6, dp - 1);

    return 0;
}

/* write MODFLOW ASCII ARRAY */
int write_MODFLOW(int fd,
		  FILE * fp,
		  int nrows, int ncols, int out_type, int dp, int width)
{
    int row, col, colcnt;
    void *ptr, *raster;

    raster = Rast_allocate_buf(out_type);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	Rast_get_row(fd, raster, row, out_type);

	colcnt = 0;
	for (col = 0, ptr = raster; col < ncols; col++,
	     ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
	    if (out_type == CELL_TYPE) {
		if (Rast_is_null_value(ptr, out_type))
		    *((CELL *) ptr) = 0;
		fprintf(fp, " %d", *((CELL *) ptr));
	    }
	    else if (out_type == FCELL_TYPE) {
		if (Rast_is_null_value(ptr, out_type))
		    *((FCELL *) ptr) = 0;
		fprintf(fp, "%*.*e", dp + 6, dp - 1, *((FCELL *) ptr));
	    }
	    else if (out_type == DCELL_TYPE) {
		if (Rast_is_null_value(ptr, out_type))
		    *((DCELL *) ptr) = 0;
		fprintf(fp, "%*.*e", dp + 6, dp - 1, *((DCELL *) ptr));
	    }

	    colcnt += 1;
	    if (colcnt >= width) {
		colcnt = 0;
		fprintf(fp, "\n");
	    }
	}
	if (colcnt > 0)
	    fprintf(fp, "\n");
    }

    return (0);
}

/* write the Surfer grid heading */
int writeGSheader(FILE * fp, const char *name)
{
    struct Cell_head region;
    char fromc[128], toc[128];
    struct FPRange range;
    DCELL Z_MIN, Z_MAX;

    if (Rast_read_fp_range(name, "", &range) < 0)
	return 1;

    fprintf(fp, "DSAA \n");

    G_get_window(&region);
    fprintf(fp, "%d %d\n", region.cols, region.rows);

    /* Projection set to -1 to force floating point output */
    G_format_easting(region.west + region.ew_res / 2., fromc,
		     G_projection() == PROJECTION_LL ? -1 : 0);
    G_format_easting(region.east - region.ew_res / 2., toc,
		     G_projection() == PROJECTION_LL ? -1 : 0);
    fprintf(fp, "%s %s\n", fromc, toc);

    G_format_northing(region.south + region.ns_res / 2., fromc,
		      G_projection() == PROJECTION_LL ? -1 : 0);
    G_format_northing(region.north - region.ns_res / 2., toc,
		      G_projection() == PROJECTION_LL ? -1 : 0);
    fprintf(fp, "%s %s\n", fromc, toc);

    Rast_get_fp_range_min_max(&range, &Z_MIN, &Z_MAX);
    fprintf(fp, "%f %f\n", (double)Z_MIN, (double)Z_MAX);

    return 0;

}

/* write Surfer Golden Software grid file */
int write_GSGRID(int fd,
		 FILE * fp,
		 int nrows,
		 int ncols, int out_type, int dp, char *null_str, int width)
{
    int row, col, colcnt;
    void *ptr, *raster;
    char cell_buf[300];

    raster = Rast_allocate_buf(out_type);

    for (row = nrows - 1; row >= 0; row--) {
	G_percent((row - nrows) * (-1), nrows, 2);

	Rast_get_row(fd, raster, row, out_type);

	colcnt = 0;
	for (col = 0, ptr = raster; col < ncols; col++,
	     ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
	    colcnt += 1;
	    if (!Rast_is_null_value(ptr, out_type)) {
		if (out_type == CELL_TYPE)
		    fprintf(fp, "%d", *((CELL *) ptr));
		else if (out_type == FCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((FCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
		else if (out_type == DCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((DCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
	    }
	    else
		fprintf(fp, "%s", null_str);

	    if (colcnt >= width) {
		fprintf(fp, "\n");
		colcnt = 0;
	    }
	    else
		fprintf(fp, " ");
	}
	if (colcnt != 0)
	    fprintf(fp, "\n");
	fprintf(fp, "\n");
    }

    return (0);
}
