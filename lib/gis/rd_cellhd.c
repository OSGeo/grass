/* read cell header, or window.
   returns NULL if ok, error message otherwise
   note:  the error message can be freed using G_free ().
 */

#include <grass/gis.h>
#include <grass/glocale.h>
#include <string.h>

#define ERROR(x,line) return error(x,line)
static int scan_item(const char *, char *, char *);
static int scan_int(const char *, int *);
static double scan_double(const char *, double *);
static char *error(const char *, int);

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

char *G__read_Cell_head_array(char **array,
			      struct Cell_head *cellhd, int is_cellhd);

char *G__read_Cell_head(FILE * fd, struct Cell_head *cellhd, int is_cellhd)
{
    int count;
    char *result, **array;
    char buf[1024];

    G_debug(2, "G__read_Cell_head");

    /* Count lines */
    count = 0;
    fseek(fd, 0L, 0);
    while (G_getl(buf, sizeof(buf), fd))
	count++;

    array = (char **)G_calloc(count + 1, sizeof(char **));

    count = 0;
    fseek(fd, 0L, 0);
    while (G_getl(buf, sizeof(buf), fd)) {
	array[count] = G_store(buf);
	count++;
    }

    result = G__read_Cell_head_array(array, cellhd, is_cellhd);

    count = 0;
    while (array[count]) {
	G_free(array[count]);
	count++;
    }
    G_free(array);

    return result;
}

/* Read window from NULL terminated array of strings */
char *G__read_Cell_head_array(char **array,
			      struct Cell_head *cellhd, int is_cellhd)
{
    char *buf;
    char label[200];
    char value[200];
    int i, line;
    int flags;
    char *G_adjust_Cell_head();
    char *err;

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
	    ERROR(buf, line);
	case 0:
	    continue;
	case 1:
	    break;
	}
	if (strncmp(label, "proj", 4) == 0) {
	    if (TEST(F_PROJ))
		ERROR(_("duplicate projection field"), line);

	    if (!scan_int(value, &cellhd->proj))
		ERROR(buf, line);

	    SET(F_PROJ);
	    continue;
	}
	if (strncmp(label, "zone", 4) == 0) {
	    if (TEST(F_ZONE))
		ERROR(_("duplicate zone field"), line);

	    if (!scan_int(value, &cellhd->zone))
		ERROR(buf, line);

	    SET(F_ZONE);
	    continue;
	}
    }
    if (!TEST(F_PROJ))
	ERROR(_("projection field missing"), 0);
    if (!TEST(F_ZONE))
	ERROR(_("zone field missing"), 0);

    /* read the other info */
    i = 0;
    for (line = 1; (buf = array[i++]); line++) {
	G_debug(3, "region item: %s", buf);
	switch (scan_item(buf, label, value)) {
	case -1:
	    ERROR(buf, line);
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
		ERROR(_("duplicate north field"), line);
	    if (!G_scan_northing(value, &cellhd->north, cellhd->proj))
		ERROR(buf, line);
	    SET(F_NORTH);
	    continue;
	}
	if (strncmp(label, "sout", 4) == 0) {
	    if (TEST(F_SOUTH))
		ERROR(_("duplicate south field"), line);
	    if (!G_scan_northing(value, &cellhd->south, cellhd->proj))
		ERROR(buf, line);
	    SET(F_SOUTH);
	    continue;
	}
	if (strncmp(label, "east", 4) == 0) {
	    if (TEST(F_EAST))
		ERROR(_("duplicate east field"), line);
	    if (!G_scan_easting(value, &cellhd->east, cellhd->proj))
		ERROR(buf, line);
	    SET(F_EAST);
	    continue;
	}
	if (strncmp(label, "west", 4) == 0) {
	    if (TEST(F_WEST))
		ERROR(_("duplicate west field"), line);
	    if (!G_scan_easting(value, &cellhd->west, cellhd->proj))
		ERROR(buf, line);
	    SET(F_WEST);
	    continue;
	}
	if (strncmp(label, "top", 3) == 0) {
	    if (TEST(F_TOP))
		ERROR(_("duplicate top field"), line);
	    if (!scan_double(value, &cellhd->top))
		ERROR(buf, line);
	    SET(F_TOP);
	    continue;
	}
	if (strncmp(label, "bottom", 6) == 0) {
	    if (TEST(F_BOTTOM))
		ERROR(_("duplicate bottom field"), line);
	    if (!scan_double(value, &cellhd->bottom))
		ERROR(buf, line);
	    SET(F_BOTTOM);
	    continue;
	}
	if (strncmp(label, "e-w ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_EWRES))
		ERROR(_("duplicate e-w resolution field"), line);
	    if (!G_scan_resolution(value, &cellhd->ew_res, cellhd->proj))
		ERROR(buf, line);
	    if (cellhd->ew_res <= 0.0)
		ERROR(buf, line);
	    SET(F_EWRES);
	    continue;
	}
	if (strncmp(label, "e-w resol3", 10) == 0) {
	    if (TEST(F_EWRES3))
		ERROR(_("duplicate 3D e-w resolution field"), line);
	    if (!G_scan_resolution(value, &cellhd->ew_res3, cellhd->proj))
		ERROR(buf, line);
	    if (cellhd->ew_res3 <= 0.0)
		ERROR(buf, line);
	    SET(F_EWRES3);
	    continue;
	}
	if (strncmp(label, "n-s ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_NSRES))
		ERROR(_("duplicate n-s resolution field"), line);
	    if (!G_scan_resolution(value, &cellhd->ns_res, cellhd->proj))
		ERROR(buf, line);
	    if (cellhd->ns_res <= 0.0)
		ERROR(buf, line);
	    SET(F_NSRES);
	    continue;
	}
	if (strncmp(label, "n-s resol3", 10) == 0) {
	    if (TEST(F_NSRES3))
		ERROR(_("duplicate 3D n-s resolution field"), line);
	    if (!G_scan_resolution(value, &cellhd->ns_res3, cellhd->proj))
		ERROR(buf, line);
	    if (cellhd->ns_res3 <= 0.0)
		ERROR(buf, line);
	    SET(F_NSRES3);
	    continue;
	}
	if (strncmp(label, "t-b ", 4) == 0) {
	    if (TEST(F_TBRES))
		ERROR(_("duplicate t-b resolution field"), line);
	    if (!scan_double(value, &cellhd->tb_res))
		ERROR(buf, line);
	    if (cellhd->tb_res <= 0.0)
		ERROR(buf, line);
	    SET(F_TBRES);
	    continue;
	}
	if (strncmp(label, "rows", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_ROWS))
		ERROR(_("duplicate rows field"), line);
	    if (!scan_int(value, &cellhd->rows))
		ERROR(buf, line);
	    if (cellhd->rows <= 0)
		ERROR(buf, line);
	    SET(F_ROWS);
	    continue;
	}
	if (strncmp(label, "rows3", 5) == 0) {
	    if (TEST(F_ROWS3))
		ERROR(_("duplicate 3D rows field"), line);
	    if (!scan_int(value, &cellhd->rows3))
		ERROR(buf, line);
	    if (cellhd->rows3 <= 0)
		ERROR(buf, line);
	    SET(F_ROWS3);
	    continue;
	}
	if (strncmp(label, "cols", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_COLS))
		ERROR(_("duplicate cols field"), line);
	    if (!scan_int(value, &cellhd->cols))
		ERROR(buf, line);
	    if (cellhd->cols <= 0)
		ERROR(buf, line);
	    SET(F_COLS);
	    continue;
	}
	if (strncmp(label, "cols3", 5) == 0) {
	    if (TEST(F_COLS3))
		ERROR(_("duplicate 3D cols field"), line);
	    if (!scan_int(value, &cellhd->cols3))
		ERROR(buf, line);
	    if (cellhd->cols3 <= 0)
		ERROR(buf, line);
	    SET(F_COLS3);
	    continue;
	}
	if (strncmp(label, "depths", 6) == 0) {
	    if (TEST(F_DEPTHS))
		ERROR(_("duplicate depths field"), line);
	    if (!scan_int(value, &cellhd->depths))
		ERROR(buf, line);
	    if (cellhd->depths <= 0)
		ERROR(buf, line);
	    SET(F_DEPTHS);
	    continue;
	}
	if (strncmp(label, "form", 4) == 0) {
	    if (TEST(F_FORMAT))
		ERROR(_("duplicate format field"), line);
	    if (!scan_int(value, &cellhd->format))
		ERROR(buf, line);
	    SET(F_FORMAT);
	    continue;
	}
	if (strncmp(label, "comp", 4) == 0) {
	    if (TEST(F_COMP))
		ERROR(_("duplicate compressed field"), line);
	    if (!scan_int(value, &cellhd->compressed))
		ERROR(buf, line);
	    SET(F_COMP);
	    continue;
	}
	ERROR(buf, line);
    }

    /* check some of the fields */
    if (!TEST(F_NORTH))
	ERROR(_("north field missing"), 0);
    if (!TEST(F_SOUTH))
	ERROR(_("south field missing"), 0);
    if (!TEST(F_WEST))
	ERROR(_("west field missing"), 0);
    if (!TEST(F_EAST))
	ERROR(_("east field missing"), 0);
    if (!TEST(F_EWRES) && !TEST(F_COLS))
	ERROR(_("cols field missing"), 0);
    if (!TEST(F_NSRES) && !TEST(F_ROWS))
	ERROR(_("rows field missing"), 0);
    /* This next stmt is commented out to allow wr_cellhd.c to write
     * headers that will be readable by GRASS 3.1
     if ((TEST(F_ROWS) && TEST(F_NSRES))
     ||  (TEST(F_COLS) && TEST(F_EWRES)))
     ERROR ("row/col and resolution information can not both appear ",0);
     */

    /* 3D defined? */
    if (TEST(F_EWRES3) || TEST(F_NSRES3) || TEST(F_COLS3) || TEST(F_ROWS3)) {
	if (!TEST(F_EWRES3))
	    ERROR(_("ewres3 field missing"), 0);
	if (!TEST(F_NSRES3))
	    ERROR(_("nsres3 field missing"), 0);
	if (!TEST(F_COLS3))
	    ERROR(_("cols3 field missing"), 0);
	if (!TEST(F_ROWS3))
	    ERROR(_("rows3 field missing"), 0);
    }
    else {			/* use 2D */
	cellhd->ew_res3 = cellhd->ew_res;
	cellhd->ns_res3 = cellhd->ns_res;
	cellhd->cols3 = cellhd->cols;
	cellhd->rows3 = cellhd->rows;
    }

    /* Adjust and complete the cell header  */
    if ((err = G_adjust_Cell_head(cellhd, TEST(F_ROWS), TEST(F_COLS))))
	ERROR(err, 0);


    return NULL;
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

static char *error(const char *msg, int line)
{
    char buf[1024];

    if (line)
	sprintf(buf, _("line %d: <%s>"), line, msg);
    else
	sprintf(buf, "<%s>", msg);

    return G_store(buf);
}
