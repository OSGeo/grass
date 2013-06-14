#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include "local.h"


struct Cell_head region, page;

static union
{
    CELL **cell;
    DCELL **dcell;
} raster;

static int max_rows;
static int at_row;
static CELL cat;
static DCELL dcat;
static int cur_x, cur_y;
static int format;
static int dense;
static CELL *cell;
static DCELL *dcell;
static char **null_flags;
static char isnull;

/* function prototypes */
static int configure_plot(void);
static int cell_dot(int, int);
static int dcell_dot(int, int);
static int cont(int, int);
static int move(int, int);
static int (*dot) (int, int);


int begin_rasterization(int nrows, int f, int do_dense)
{
    int i, size;
    int pages;

    dense = (do_dense != 0);

    /* otherwise get complaints about window changes */
    G_suppress_warnings(1);

    format = f;

    max_rows = nrows;
    if (max_rows <= 0)
	max_rows = 512;

    G_get_set_window(&region);
    G_get_set_window(&page);

    pages = (region.rows + max_rows - 1) / max_rows;

    if (max_rows > region.rows)
	max_rows = region.rows;

    size = max_rows * region.cols;
    switch (format) {
    case USE_CELL:
	raster.cell =
	    (CELL **) G_calloc(max_rows * sizeof(char), sizeof(CELL *));
	raster.cell[0] = (CELL *) G_calloc(size * sizeof(char), sizeof(CELL));
	for (i = 1; i < max_rows; i++)
	    raster.cell[i] = raster.cell[i - 1] + region.cols;
	dot = cell_dot;
	break;

    case USE_DCELL:
	raster.dcell =
	    (DCELL **) G_calloc(max_rows * sizeof(char), sizeof(DCELL *));
	raster.dcell[0] =
	    (DCELL *) G_calloc(size * sizeof(char), sizeof(DCELL));
	for (i = 1; i < max_rows; i++)
	    raster.dcell[i] = raster.dcell[i - 1] + region.cols;
	dot = dcell_dot;
	break;
    }

    null_flags = (char **)G_calloc(max_rows * sizeof(char), sizeof(char *));
    null_flags[0] = (char *)G_calloc(size * sizeof(char), sizeof(char));
    for (i = 1; i < max_rows; i++)
	null_flags[i] = null_flags[i - 1] + region.cols;

    at_row = 0;
    configure_plot();

    return pages;
}


static int configure_plot(void)
{
    int i, j;
    int nrows;
    int ncols;

    nrows = region.rows - at_row;
    if (nrows <= 0)
	return 1;

    if (nrows > max_rows)
	nrows = max_rows;

    ncols = region.cols;

    /* zero the raster */
    switch (format) {
    case USE_CELL:
	for (i = 0; i < nrows; i++)
	    for (j = 0; j < ncols; j++)
		raster.cell[i][j] = 0;
	break;
    case USE_DCELL:
	for (i = 0; i < nrows; i++)
	    for (j = 0; j < ncols; j++)
		raster.dcell[i][j] = 0;
	break;
    }

    for (i = 0; i < nrows; i++)
	for (j = 0; j < ncols; j++)
	    null_flags[i][j] = 1;

    /* change the region */
    page.north = region.north - at_row * region.ns_res;
    page.south = page.north - nrows * region.ns_res;
    G_set_window(&page);

    /* configure the plot routines */
    if (dense)
	setup_plot(0, page.rows, 0, page.cols, dot);
    else
	G_setup_plot(-0.5, page.rows - 0.5, -0.5, page.cols - 0.5, move, cont);

    return 0;
}


int output_raster(int fd)
{
    int i;

    for (i = 0; i < page.rows; i++, at_row++) {
	G_percent(i, page.rows, 2);
	switch (format) {
	case USE_CELL:
	    cell = raster.cell[i];

	    /* insert the NULL values */
	    Rast_insert_c_null_values(cell, null_flags[i], page.cols);
	    Rast_put_c_row(fd, cell);
	    break;
	case USE_DCELL:
	    dcell = raster.dcell[i];

	    /* insert the NULL values */
	    Rast_insert_d_null_values(dcell, null_flags[i], page.cols);
	    Rast_put_d_row(fd, dcell);
	    break;
	}
    }
    G_percent(1, 1, 1);
    
    return configure_plot();
}

int set_cat(CELL x)
{
    cat = x;
    if ((isnull = ISNULL(&cat)))
	cat = 0;

    return 0;
}

int set_dcat(DCELL x)
{
    dcat = x;
    if ((isnull = ISDNULL(&dcat)))
	dcat = 0;

    return 0;
}

static int move(int x, int y)
{
    cur_x = x;
    cur_y = y;

    return 0;
}

static int cont(int x, int y)
{
    if (cur_x < 0 && x < 0) {
	move(x, y);
	return 0;
    }
    if (cur_y < 0 && y < 0) {
	move(x, y);
	return 0;
    }
    if (cur_x >= page.cols && x >= page.cols) {
	move(x, y);
	return 0;
    }
    if (cur_y >= page.rows && y >= page.rows) {
	move(x, y);
	return 0;
    }

    G_bresenham_line(cur_x, cur_y, x, y, dot);
    move(x, y);

    return 0;
}

static int cell_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows) {
	raster.cell[y][x] = cat;
	null_flags[y][x] = isnull;
    }

    return 0;
}

static int dcell_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows) {
	raster.dcell[y][x] = dcat;
	null_flags[y][x] = isnull;
    }

    return 0;
}
