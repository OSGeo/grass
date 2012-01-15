#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


void rdwr_gridatb(void)
{
    char buf[1024];
    FILE *fp;
    int fd, i, j, retval;
    float idx;

    fp = fopen(file, "r");

    buf[0] = 0;
    fscanf(fp, "%[^\n]", buf);
    if (!buf[0])
	getc(fp);

    fscanf(fp, "%d %d %lf\n", &cellhd.cols, &cellhd.rows, &cellhd.ns_res);
    cellhd.ew_res = cellhd.ns_res;
    cellhd.south = 0;
    cellhd.north = cellhd.south + cellhd.ns_res * cellhd.rows;
    cellhd.west = 0;
    cellhd.east = cellhd.west + cellhd.ew_res * cellhd.cols;
    cellhd.format = -1;
    cellhd.compressed = 1;

    if (retval = adjcellhd(&cellhd)) {
	fclose(fp);
	switch (retval) {
	case 1:
	    G_fatal_error(_("Setting window header failed"));
	    break;
	case 2:
	    G_fatal_error(_("Rows changed"));
	    break;
	case 3:
	    G_fatal_error(_("Cols changed"));
	    break;
	}
    }

    fd = Rast_open_new(oname, FCELL_TYPE);

    cell = (FCELL *) G_malloc(sizeof(FCELL) * cellhd.cols);

    for (i = 0; i < cellhd.rows; i++) {
	G_percent(i, cellhd.rows, 2);

	for (j = 0; j < cellhd.cols; j++) {
	    idx = 9999.0;
	    fscanf(fp, "%f", &idx);
	    if (idx >= 9999.0) {
		Rast_set_f_null_value(&(cell[j]), 1);
	    }
	    else {
		cell[j] = idx;
	    }
	}
	Rast_put_f_row(fd, cell);
    }
    G_percent(i, cellhd.rows, 2);
    if(fp)
	fclose(fp);
    Rast_close(fd);

    Rast_put_cell_title(oname, buf);
    Rast_put_cellhd(oname, &cellhd);

    return;
}
