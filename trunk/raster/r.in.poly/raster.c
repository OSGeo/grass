#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "format.h"
#include "local_proto.h"

struct Cell_head region, page;

static union
{
    char **c;
    unsigned char **u;
    short **s;
    CELL **cell;
    FCELL **fcell;
    DCELL **dcell;
} raster;
static int max_rows;
static int at_row;
static int cat_int;
static double cat_double;
static int cur_x, cur_y;
static int format;
static CELL *cell;

static int (*dot) ();
static int cell_dot(int, int);
static int fcell_dot(int, int);
static int dcell_dot(int, int);
static int uchar_dot(int, int);
static int char_dot(int, int);
static int short_dot(int, int);
static int move(int, int);
static int cont(int, int);

int begin_rasterization(int nrows, int f)
{
    int i, size;
    int pages;

    format = f;

    max_rows = nrows;
    if (max_rows <= 0)
	max_rows = 512;

    Rast_get_window(&region);
    Rast_get_window(&page);

    pages = (region.rows + max_rows - 1) / max_rows;

    if (max_rows > region.rows)
	max_rows = region.rows;

    size = max_rows * region.cols;
    switch (format) {
    case USE_CHAR:
	raster.c = (char **)G_calloc(max_rows, sizeof(char *));
	raster.c[0] = (char *)G_calloc(size, sizeof(char));
	for (i = 1; i < max_rows; i++)
	    raster.c[i] = raster.c[i - 1] + region.cols;
	dot = char_dot;
	break;

    case USE_UCHAR:
	raster.u =
	    (unsigned char **)G_calloc(max_rows, sizeof(unsigned char *));
	raster.u[0] = (unsigned char *)G_calloc(size, sizeof(unsigned char));
	for (i = 1; i < max_rows; i++)
	    raster.u[i] = raster.u[i - 1] + region.cols;
	dot = uchar_dot;
	break;

    case USE_SHORT:
	raster.s = (short **)G_calloc(max_rows, sizeof(short *));
	raster.s[0] = (short *)G_calloc(size, sizeof(short));
	for (i = 1; i < max_rows; i++)
	    raster.s[i] = raster.s[i - 1] + region.cols;
	dot = short_dot;
	break;

    case USE_CELL:
	raster.cell = (CELL **) G_calloc(max_rows, sizeof(CELL *));
	raster.cell[0] = (CELL *) G_calloc(size, sizeof(CELL));
	for (i = 1; i < max_rows; i++)
	    raster.cell[i] = raster.cell[i - 1] + region.cols;
	dot = cell_dot;
	break;

    case USE_FCELL:
	raster.fcell = (FCELL **) G_calloc(max_rows, sizeof(FCELL *));
	raster.fcell[0] = (FCELL *) G_calloc(size, sizeof(FCELL));
	for (i = 1; i < max_rows; i++)
	    raster.fcell[i] = raster.fcell[i - 1] + region.cols;
	dot = fcell_dot;
	break;
 
    case USE_DCELL:
	raster.dcell = (DCELL **) G_calloc(max_rows, sizeof(DCELL *));
	raster.dcell[0] = (DCELL *) G_calloc(size, sizeof(DCELL));
	for (i = 1; i < max_rows; i++)
	    raster.dcell[i] = raster.dcell[i - 1] + region.cols;
	dot = dcell_dot;
	break;
    }
    if (format != USE_CELL && format != USE_FCELL && format != USE_DCELL)
	cell = Rast_allocate_c_buf();

    at_row = 0;
    configure_plot();

    return pages;
}

#define DONE 1
#define ERROR -1
#define AGAIN 0

int configure_plot(void)
{
    int i, j;
    int nrows;
    int ncols;

    nrows = region.rows - at_row;
    if (nrows <= 0)
	return DONE;

    if (nrows > max_rows)
	nrows = max_rows;

    ncols = region.cols;

    /* zero the raster */
    switch (format) {
    case USE_CHAR:
	for (i = 0; i < nrows; i++)
	    for (j = 0; j < ncols; j++)
		raster.c[i][j] = 0;
	break;

    case USE_UCHAR:
	for (i = 0; i < nrows; i++)
	    for (j = 0; j < ncols; j++)
		raster.u[i][j] = 0;
	break;

    case USE_SHORT:
	for (i = 0; i < nrows; i++)
	    for (j = 0; j < ncols; j++)
		raster.s[i][j] = 0;
	break;

    case USE_CELL:
	for (i = 0; i < nrows; i++)
	    Rast_set_c_null_value(raster.cell[i], ncols);
	break;
    case USE_FCELL:
	for (i = 0; i < nrows; i++)
	    Rast_set_f_null_value(raster.fcell[i], ncols);
	break;
    case USE_DCELL:
	for (i = 0; i < nrows; i++)
	    Rast_set_d_null_value(raster.dcell[i], ncols);
	break;
    }

    /* change the region */
    page.north = region.north - at_row * region.ns_res;
    page.south = page.north - nrows * region.ns_res;
    /* Rast_set_[inpu|output]_window not working but G_set_window ??? */
    G_set_window(&page);

    /* configure the plot routines */
    G_setup_plot(-0.5, page.rows - 0.5, -0.5, page.cols - 0.5, move, cont);

    return AGAIN;
}

int  output_raster(int fd, int *null)
{
    int i, j;
    FCELL *fcell;
    DCELL *dcell;

    for (i = 0; i < page.rows; i++, at_row++) {
        switch (format) {
        
        case USE_CHAR:
            for (j = 0; j < page.cols; j++) {
                cell[j] = (CELL) raster.c[i][j];
                if (cell[j] == 0)
                    Rast_set_null_value(&cell[j], 1, CELL_TYPE);
            }
            break;
            
        case USE_UCHAR:
            for (j = 0; j < page.cols; j++) {
                cell[j] = (CELL) raster.u[i][j];
                if (cell[j] == 0)
                    Rast_set_null_value(&cell[j], 1, CELL_TYPE);
            }
            break;
            
        case USE_SHORT:
            for (j = 0; j < page.cols; j++) {
                cell[j] = (CELL) raster.s[i][j];
                if (cell[j] == 0)
                    Rast_set_null_value(&cell[j], 1, CELL_TYPE);
            }
            break;

        case USE_CELL:
            cell = raster.cell[i];
            if (!null)
                break;
            for (j = 0; j < page.cols; j++) {
                if (cell[j] == *null)
                    Rast_set_null_value(&cell[j], 1, CELL_TYPE);
            }
            break;

        case USE_FCELL:
            fcell = raster.fcell[i];
            if (!null)
                break;
            for (j = 0; j < page.cols; j++) {
                if (fabs(fcell[j] - (FCELL) *null) < 1e-6)
                    Rast_set_null_value(&fcell[j], 1, FCELL_TYPE);
            }
            break;
            
        case USE_DCELL:
            dcell = raster.dcell[i];
            if (!null)
                break;
            for (j = 0; j < page.cols; j++) {
                if (fabs(dcell[j] - (DCELL) *null) < 1e-6)
                    Rast_set_null_value(&dcell[j], 1, DCELL_TYPE);
            }
            break;
        }
        switch (format) {
        case USE_CHAR:
        case USE_UCHAR:
        case USE_SHORT:
        case USE_CELL:
            Rast_put_row(fd, cell, CELL_TYPE);
            break;
        case USE_FCELL:
            Rast_put_row(fd, fcell, FCELL_TYPE);
            break;
        case USE_DCELL:
            Rast_put_row(fd, dcell, DCELL_TYPE);
            break;
        }
        G_percent(i, page.rows, 2);
    }
    G_percent(i, page.rows, 2);
    return configure_plot();
}

int set_cat_int(int x)
{
    cat_int = x;

    return 0;
}

int set_cat_double(double x)
{
    cat_double = x;

    return 0;
}

int raster_dot(int x, int y)
{
    dot(x, y);

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
    if (cur_x < 0 && x < 0)
	goto set;
    if (cur_y < 0 && y < 0)
	goto set;
    if (cur_x >= page.cols && x >= page.cols)
	goto set;
    if (cur_y >= page.rows && y >= page.rows)
	goto set;

    G_bresenham_line(cur_x, cur_y, x, y, dot);

  set:
    move(x, y);

    return 0;
}

static int cell_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.cell[y][x] = (CELL) cat_int;

    return 0;
}

static int fcell_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.fcell[y][x] = (FCELL) cat_double;

    return 0;
}

static int dcell_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.dcell[y][x] = (DCELL) cat_double;

    return 0;
}

static int uchar_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.u[y][x] = cat_int;

    return 0;
}

static int char_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.c[y][x] = cat_int;

    return 0;
}

static int short_dot(int x, int y)
{
    if (x >= 0 && x < page.cols && y >= 0 && y < page.rows)
	raster.s[y][x] = cat_int;

    return 0;
}
