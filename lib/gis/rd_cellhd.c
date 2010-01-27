/* read cell header, or window.
   returns NULL if ok, error message otherwise
   note:  the error message can be freed using G_free ().
 */
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static int scan_item(const char *, char *, char *);
static int scan_int(const char *, int *);
static double scan_double(const char *, double *);

#define F_PROJ   1
#define F_ZONE   2
#define F_NORTH  3
#define F_SOUTH  4
#define F_EAST   5
#define F_WEST   6
#define F_EWRES  7
#define F_NSRES  8
#define F_FORMAT 9
#define F_COMP   10
#define F_COLS   11
#define F_ROWS   12

#define F_EWRES3 13
#define F_NSRES3 14
#define F_COLS3  15
#define F_ROWS3  16
#define F_TOP    17
#define F_BOTTOM 18
#define F_TBRES  19
#define F_DEPTHS 20

#define SET(x) flags|=(1<<x)
#define TEST(x) (flags&(1<<x))

void G__read_Cell_head(FILE * fd, struct Cell_head *cellhd, int is_cellhd)
{
    int count;
    char **array;
    char buf[1024];

    G_debug(2, "G__read_Cell_head");

    /* Count lines */
    count = 0;
    G_fseek(fd, 0L, 0);
    while (G_getl(buf, sizeof(buf), fd))
	count++;

    array = (char **)G_calloc(count + 1, sizeof(char **));

    count = 0;
    G_fseek(fd, 0L, 0);
    while (G_getl(buf, sizeof(buf), fd)) {
	array[count] = G_store(buf);
	count++;
    }

    G__read_Cell_head_array(array, cellhd, is_cellhd);

    count = 0;
    while (array[count]) {
	G_free(array[count]);
	count++;
    }
    G_free(array);
}

/* Read window from NULL terminated array of strings */
void G__read_Cell_head_array(char **array,
			     struct Cell_head *cellhd, int is_cellhd)
{
    char *buf;
    char label[200];
    char value[200];
    int i, line;
    int flags;

    G_debug(2, "G__read_Cell_head_array");

    flags = 0;

    /* initialize the cell header */
    cellhd->format = 0;
    cellhd->rows = 0;
    cellhd->rows3 = 0;
    cellhd->cols = 0;
    cellhd->cols3 = 0;
    cellhd->depths = 1;
    cellhd->proj = -1;
    cellhd->zone = -1;
    cellhd->compressed = -1;
    cellhd->ew_res = 0.0;
    cellhd->ew_res3 = 1.0;
    cellhd->ns_res = 0.0;
    cellhd->ns_res3 = 1.0;
    cellhd->tb_res = 1.0;
    cellhd->north = 0.0;
    cellhd->south = 0.0;
    cellhd->east = 0.0;
    cellhd->west = 0.0;
    cellhd->top = 1.0;
    cellhd->bottom = 0.0;

    /* determine projection, zone first */

    i = 0;
    for (line = 1; (buf = array[i++]); line++) {
	if (TEST(F_PROJ) && TEST(F_ZONE))
	    break;

	switch (scan_item(buf, label, value)) {
	case -1:
	    G_fatal_error(_("Syntax error"));
	case 0:
	    continue;
	case 1:
	    break;
	}
	if (strncmp(label, "proj", 4) == 0) {
	    if (TEST(F_PROJ))
		G_fatal_error(_("duplicate projection field"));

	    if (!scan_int(value, &cellhd->proj))
		G_fatal_error(_("Syntax error"));

	    SET(F_PROJ);
	    continue;
	}
	if (strncmp(label, "zone", 4) == 0) {
	    if (TEST(F_ZONE))
		G_fatal_error(_("duplicate zone field"));

	    if (!scan_int(value, &cellhd->zone))
		G_fatal_error(_("Syntax error"));

	    SET(F_ZONE);
	    continue;
	}
    }
    if (!TEST(F_PROJ))
	G_fatal_error(_("projection field missing"));
    if (!TEST(F_ZONE))
	G_fatal_error(_("zone field missing"));

    /* read the other info */
    i = 0;
    for (line = 1; (buf = array[i++]); line++) {
	G_debug(3, "region item: %s", buf);
	switch (scan_item(buf, label, value)) {
	case -1:
	    G_fatal_error(_("Syntax error"));
	case 0:
	    continue;
	case 1:
	    break;
	}

	if (strncmp(label, "proj", 4) == 0)
	    continue;
	if (strncmp(label, "zone", 4) == 0)
	    continue;

	if (strncmp(label, "nort", 4) == 0) {
	    if (TEST(F_NORTH))
		G_fatal_error(_("duplicate north field"));
	    if (!G_scan_northing(value, &cellhd->north, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    SET(F_NORTH);
	    continue;
	}
	if (strncmp(label, "sout", 4) == 0) {
	    if (TEST(F_SOUTH))
		G_fatal_error(_("duplicate south field"));
	    if (!G_scan_northing(value, &cellhd->south, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    SET(F_SOUTH);
	    continue;
	}
	if (strncmp(label, "east", 4) == 0) {
	    if (TEST(F_EAST))
		G_fatal_error(_("duplicate east field"));
	    if (!G_scan_easting(value, &cellhd->east, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    SET(F_EAST);
	    continue;
	}
	if (strncmp(label, "west", 4) == 0) {
	    if (TEST(F_WEST))
		G_fatal_error(_("duplicate west field"));
	    if (!G_scan_easting(value, &cellhd->west, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    SET(F_WEST);
	    continue;
	}
	if (strncmp(label, "top", 3) == 0) {
	    if (TEST(F_TOP))
		G_fatal_error(_("duplicate top field"));
	    if (!scan_double(value, &cellhd->top))
		G_fatal_error(_("Syntax error"));
	    SET(F_TOP);
	    continue;
	}
	if (strncmp(label, "bottom", 6) == 0) {
	    if (TEST(F_BOTTOM))
		G_fatal_error(_("duplicate bottom field"));
	    if (!scan_double(value, &cellhd->bottom))
		G_fatal_error(_("Syntax error"));
	    SET(F_BOTTOM);
	    continue;
	}
	if (strncmp(label, "e-w ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_EWRES))
		G_fatal_error(_("duplicate e-w resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ew_res, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->ew_res <= 0.0)
		G_fatal_error(_("Syntax error"));
	    SET(F_EWRES);
	    continue;
	}
	if (strncmp(label, "e-w resol3", 10) == 0) {
	    if (TEST(F_EWRES3))
		G_fatal_error(_("duplicate 3D e-w resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ew_res3, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->ew_res3 <= 0.0)
		G_fatal_error(_("Syntax error"));
	    SET(F_EWRES3);
	    continue;
	}
	if (strncmp(label, "n-s ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_NSRES))
		G_fatal_error(_("duplicate n-s resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ns_res, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->ns_res <= 0.0)
		G_fatal_error(_("Syntax error"));
	    SET(F_NSRES);
	    continue;
	}
	if (strncmp(label, "n-s resol3", 10) == 0) {
	    if (TEST(F_NSRES3))
		G_fatal_error(_("duplicate 3D n-s resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ns_res3, cellhd->proj))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->ns_res3 <= 0.0)
		G_fatal_error(_("Syntax error"));
	    SET(F_NSRES3);
	    continue;
	}
	if (strncmp(label, "t-b ", 4) == 0) {
	    if (TEST(F_TBRES))
		G_fatal_error(_("duplicate t-b resolution field"));
	    if (!scan_double(value, &cellhd->tb_res))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->tb_res <= 0.0)
		G_fatal_error(_("Syntax error"));
	    SET(F_TBRES);
	    continue;
	}
	if (strncmp(label, "rows", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_ROWS))
		G_fatal_error(_("duplicate rows field"));
	    if (!scan_int(value, &cellhd->rows))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->rows <= 0)
		G_fatal_error(_("Syntax error"));
	    SET(F_ROWS);
	    continue;
	}
	if (strncmp(label, "rows3", 5) == 0) {
	    if (TEST(F_ROWS3))
		G_fatal_error(_("duplicate 3D rows field"));
	    if (!scan_int(value, &cellhd->rows3))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->rows3 <= 0)
		G_fatal_error(_("Syntax error"));
	    SET(F_ROWS3);
	    continue;
	}
	if (strncmp(label, "cols", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_COLS))
		G_fatal_error(_("duplicate cols field"));
	    if (!scan_int(value, &cellhd->cols))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->cols <= 0)
		G_fatal_error(_("Syntax error"));
	    SET(F_COLS);
	    continue;
	}
	if (strncmp(label, "cols3", 5) == 0) {
	    if (TEST(F_COLS3))
		G_fatal_error(_("duplicate 3D cols field"));
	    if (!scan_int(value, &cellhd->cols3))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->cols3 <= 0)
		G_fatal_error(_("Syntax error"));
	    SET(F_COLS3);
	    continue;
	}
	if (strncmp(label, "depths", 6) == 0) {
	    if (TEST(F_DEPTHS))
		G_fatal_error(_("duplicate depths field"));
	    if (!scan_int(value, &cellhd->depths))
		G_fatal_error(_("Syntax error"));
	    if (cellhd->depths <= 0)
		G_fatal_error(_("Syntax error"));
	    SET(F_DEPTHS);
	    continue;
	}
	if (strncmp(label, "form", 4) == 0) {
	    if (TEST(F_FORMAT))
		G_fatal_error(_("duplicate format field"));
	    if (!scan_int(value, &cellhd->format))
		G_fatal_error(_("Syntax error"));
	    SET(F_FORMAT);
	    continue;
	}
	if (strncmp(label, "comp", 4) == 0) {
	    if (TEST(F_COMP))
		G_fatal_error(_("duplicate compressed field"));
	    if (!scan_int(value, &cellhd->compressed))
		G_fatal_error(_("Syntax error"));
	    SET(F_COMP);
	    continue;
	}
	G_fatal_error(_("Syntax error"));
    }

    /* check some of the fields */
    if (!TEST(F_NORTH))
	G_fatal_error(_("north field missing"));
    if (!TEST(F_SOUTH))
	G_fatal_error(_("south field missing"));
    if (!TEST(F_WEST))
	G_fatal_error(_("west field missing"));
    if (!TEST(F_EAST))
	G_fatal_error(_("east field missing"));
    if (!TEST(F_EWRES) && !TEST(F_COLS))
	G_fatal_error(_("cols field missing"));
    if (!TEST(F_NSRES) && !TEST(F_ROWS))
	G_fatal_error(_("rows field missing"));
    /* This next stmt is commented out to allow wr_cellhd.c to write
     * headers that will be readable by GRASS 3.1
     if ((TEST(F_ROWS) && TEST(F_NSRES))
     ||  (TEST(F_COLS) && TEST(F_EWRES)))
     ERROR ("row/col and resolution information can not both appear ",0);
     */

    /* 3D defined? */
    if (TEST(F_EWRES3) || TEST(F_NSRES3) || TEST(F_COLS3) || TEST(F_ROWS3)) {
	if (!TEST(F_EWRES3))
	    G_fatal_error(_("ewres3 field missing"));
	if (!TEST(F_NSRES3))
	    G_fatal_error(_("nsres3 field missing"));
	if (!TEST(F_COLS3))
	    G_fatal_error(_("cols3 field missing"));
	if (!TEST(F_ROWS3))
	    G_fatal_error(_("rows3 field missing"));
    }
    else {			/* use 2D */
	cellhd->ew_res3 = cellhd->ew_res;
	cellhd->ns_res3 = cellhd->ns_res;
	cellhd->cols3 = cellhd->cols;
	cellhd->rows3 = cellhd->rows;
    }

    /* Adjust and complete the cell header  */
    G_adjust_Cell_head(cellhd, TEST(F_ROWS), TEST(F_COLS));
}

static int scan_item(const char *buf, char *label, char *value)
{
    /* skip blank lines */
    if (sscanf(buf, "%1s", label) != 1)
	return 0;

    /* skip comment lines */
    if (*label == '#')
	return 0;

    /* must be label: value */
    if (sscanf(buf, "%[^:]:%[^\n]", label, value) != 2)
	return -1;

    G_strip(label);
    G_strip(value);
    return 1;
}

static int scan_int(const char *buf, int *n)
{
    char dummy[3];

    *dummy = 0;
    return (sscanf(buf, "%d%1s", n, dummy) == 1 && *dummy == 0);
}

static double scan_double(const char *buf, double *n)
{
    char dummy[3];

    *dummy = 0;
    return (sscanf(buf, "%lf%1s", n, dummy) == 1 && *dummy == 0);
}

