/*!
  \file lib/gis/rd_cellhd.c
  
  \brief GIS Library - Read cell header or window
  
  (C) 1999-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author USACERL and others
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

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

/*!
  \brief Read cell header (for internal use only)
  
  \param fp file descriptor
  \param[out] cellhd pointer to Cell_head structure
  \param is_cellhd ? (unused)
*/
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

    array = (char **)G_calloc(count + 1, sizeof(char *));

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

/*!
  \brief Read window from NULL terminated array of strings (for internal use only)

  \param array array of strings
  \param[out] cellhd pointer to Cell_head structure
  \param is_cellhd ? (unused)
*/
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
	    G_fatal_error(_("Syntax error in cell header, line %d: %s"),
	                  line, buf);
	case 0:
	    continue;
	case 1:
	    break;
	}
	if (strncmp(label, "proj", 4) == 0) {
	    if (TEST(F_PROJ))
		G_fatal_error(_("Duplicate projection field"));

	    if (!scan_int(value, &cellhd->proj))
		G_fatal_error(_("Invalid projection field: %s"), value);

	    SET(F_PROJ);
	    continue;
	}
	if (strncmp(label, "zone", 4) == 0) {
	    if (TEST(F_ZONE))
		G_fatal_error(_("Duplicate zone field"));

	    if (!scan_int(value, &cellhd->zone))
		G_fatal_error(_("Invalid zone field: %s"), value);

	    SET(F_ZONE);
	    continue;
	}
    }
    if (!TEST(F_PROJ))
	G_fatal_error(_("Field <%s> missing"), "projection");
    if (!TEST(F_ZONE))
	G_fatal_error(_("Field <%s> missing"), "zone");

    /* read the other info */
    i = 0;
    for (line = 1; (buf = array[i++]); line++) {
	G_debug(3, "region item: %s", buf);
	switch (scan_item(buf, label, value)) {
	case -1:
	    G_fatal_error(_("Syntax error in cell header, line %d: %s"),
	                  line, buf);
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
		G_fatal_error(_("Duplicate north field"));
	    if (!G_scan_northing(value, &cellhd->north, cellhd->proj))
		G_fatal_error(_("Invalid north field: %s"), value);
	    SET(F_NORTH);
	    continue;
	}
	if (strncmp(label, "sout", 4) == 0) {
	    if (TEST(F_SOUTH))
		G_fatal_error(_("Duplicate south field"));
	    if (!G_scan_northing(value, &cellhd->south, cellhd->proj))
		G_fatal_error(_("Invalid south field: %s"), value);
	    SET(F_SOUTH);
	    continue;
	}
	if (strncmp(label, "east", 4) == 0) {
	    if (TEST(F_EAST))
		G_fatal_error(_("Duplicate east field"));
	    if (!G_scan_easting(value, &cellhd->east, cellhd->proj))
		G_fatal_error(_("Invalid east field: %s"), value);
	    SET(F_EAST);
	    continue;
	}
	if (strncmp(label, "west", 4) == 0) {
	    if (TEST(F_WEST))
		G_fatal_error(_("Duplicate west field"));
	    if (!G_scan_easting(value, &cellhd->west, cellhd->proj))
		G_fatal_error(_("Invalid west field: %s"), value);
	    SET(F_WEST);
	    continue;
	}
	if (strncmp(label, "top", 3) == 0) {
	    if (TEST(F_TOP))
		G_fatal_error(_("Duplicate top field"));
	    if (!scan_double(value, &cellhd->top))
		G_fatal_error(_("Invalid top field: %s"), value);
	    SET(F_TOP);
	    continue;
	}
	if (strncmp(label, "bottom", 6) == 0) {
	    if (TEST(F_BOTTOM))
		G_fatal_error(_("Duplicate bottom field"));
	    if (!scan_double(value, &cellhd->bottom))
		G_fatal_error(_("Invalid bottom field: %s"), value);
	    SET(F_BOTTOM);
	    continue;
	}
	if (strncmp(label, "e-w ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_EWRES))
		G_fatal_error(_("Duplicate e-w resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ew_res, cellhd->proj))
		G_fatal_error(_("Invalid e-w resolution field: %s"), value);
	    if (cellhd->ew_res <= 0.0)
		G_fatal_error(_("Invalid e-w resolution field: %s"), value);
	    SET(F_EWRES);
	    continue;
	}
	if (strncmp(label, "e-w resol3", 10) == 0) {
	    if (TEST(F_EWRES3))
		G_fatal_error(_("Duplicate 3D e-w resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ew_res3, cellhd->proj))
		G_fatal_error(_("Invalid 3D e-w resolution field: %s"), value);
	    if (cellhd->ew_res3 <= 0.0)
		G_fatal_error(_("Invalid 3D e-w resolution field: %s"), value);
	    SET(F_EWRES3);
	    continue;
	}
	if (strncmp(label, "n-s ", 4) == 0 && strlen(label) == 9) {
	    if (TEST(F_NSRES))
		G_fatal_error(_("Duplicate n-s resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ns_res, cellhd->proj))
		G_fatal_error(_("Invalid n-s resolution field: %s"), value);
	    if (cellhd->ns_res <= 0.0)
		G_fatal_error(_("Invalid n-s resolution field: %s"), value);
	    SET(F_NSRES);
	    continue;
	}
	if (strncmp(label, "n-s resol3", 10) == 0) {
	    if (TEST(F_NSRES3))
		G_fatal_error(_("Duplicate 3D n-s resolution field"));
	    if (!G_scan_resolution(value, &cellhd->ns_res3, cellhd->proj))
		G_fatal_error(_("Invalid 3D n-s resolution field: %s"), value);
	    if (cellhd->ns_res3 <= 0.0)
		G_fatal_error(_("Invalid 3D n-s resolution field: %s"), value);
	    SET(F_NSRES3);
	    continue;
	}
	if (strncmp(label, "t-b ", 4) == 0) {
	    if (TEST(F_TBRES))
		G_fatal_error(_("Duplicate t-b resolution field"));
	    if (!scan_double(value, &cellhd->tb_res))
		G_fatal_error(_("Invalid t-b resolution field: %s"), value);
	    if (cellhd->tb_res <= 0.0)
		G_fatal_error(_("Invalid t-b resolution field: %s"), value);
	    SET(F_TBRES);
	    continue;
	}
	if (strncmp(label, "rows", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_ROWS))
		G_fatal_error(_("Duplicate rows field"));
	    if (!scan_int(value, &cellhd->rows))
		G_fatal_error(_("Invalid rows field: %s"), value);
	    if (cellhd->rows <= 0)
		G_fatal_error(_("Invalid rows field: %s"), value);
	    SET(F_ROWS);
	    continue;
	}
	if (strncmp(label, "rows3", 5) == 0) {
	    if (TEST(F_ROWS3))
		G_fatal_error(_("Duplicate 3D rows field"));
	    if (!scan_int(value, &cellhd->rows3))
		G_fatal_error(_("Invalid 3D rows field: %s"), value);
	    if (cellhd->rows3 <= 0)
		G_fatal_error(_("Invalid 3D rows field: %s"), value);
	    SET(F_ROWS3);
	    continue;
	}
	if (strncmp(label, "cols", 4) == 0 && strlen(label) == 4) {
	    if (TEST(F_COLS))
		G_fatal_error(_("Duplicate cols field"));
	    if (!scan_int(value, &cellhd->cols))
		G_fatal_error(_("Invalid cols field: %s"), value);
	    if (cellhd->cols <= 0)
		G_fatal_error(_("Invalid cols field: %s"), value);
	    SET(F_COLS);
	    continue;
	}
	if (strncmp(label, "cols3", 5) == 0) {
	    if (TEST(F_COLS3))
		G_fatal_error(_("Duplicate 3D cols field"));
	    if (!scan_int(value, &cellhd->cols3))
		G_fatal_error(_("Invalid 3D cols field: %s"), value);
	    if (cellhd->cols3 <= 0)
		G_fatal_error(_("Invalid 3D cols field: %s"), value);
	    SET(F_COLS3);
	    continue;
	}
	if (strncmp(label, "depths", 6) == 0) {
	    if (TEST(F_DEPTHS))
		G_fatal_error(_("Duplicate depths field"));
	    if (!scan_int(value, &cellhd->depths))
		G_fatal_error(_("Invalid depths field: %s"), value);
	    if (cellhd->depths <= 0)
		G_fatal_error(_("Invalid depths field: %s"), value);
	    SET(F_DEPTHS);
	    continue;
	}
	if (strncmp(label, "form", 4) == 0) {
	    if (TEST(F_FORMAT))
		G_fatal_error(_("Duplicate format field"));
	    if (!scan_int(value, &cellhd->format))
		G_fatal_error(_("Invalid format field: %s"), value);
	    SET(F_FORMAT);
	    continue;
	}
	if (strncmp(label, "comp", 4) == 0) {
	    if (TEST(F_COMP))
		G_fatal_error(_("Duplicate compressed field"));
	    if (!scan_int(value, &cellhd->compressed))
		G_fatal_error(_("Invalid compressed field: %s"), value);
	    SET(F_COMP);
	    continue;
	}
	G_fatal_error(_("Syntax error in cell header, line %d: %s"),
	              line, buf);
    }

    /* check some of the fields */
    if (!TEST(F_NORTH))
	G_fatal_error(_("Field <%s> missing"), "north");
    if (!TEST(F_SOUTH))
	G_fatal_error(_("Field <%s> missing"), "south");
    if (!TEST(F_WEST))
	G_fatal_error(_("Field <%s> missing"), "west");
    if (!TEST(F_EAST))
	G_fatal_error(_("Field <%s> missing"), "east");
    if (!TEST(F_EWRES) && !TEST(F_COLS))
	G_fatal_error(_("Field <%s> missing"), "cols");
    if (!TEST(F_NSRES) && !TEST(F_ROWS))
	G_fatal_error(_("Field <%s> missing"), "rows");
    /* This next stmt is commented out to allow wr_cellhd.c to write
     * headers that will be readable by GRASS 3.1
     if ((TEST(F_ROWS) && TEST(F_NSRES))
     ||  (TEST(F_COLS) && TEST(F_EWRES)))
     ERROR ("row/col and resolution information can not both appear ",0);
     */

    /* 3D defined? */
    if (TEST(F_EWRES3) || TEST(F_NSRES3) || TEST(F_COLS3) || TEST(F_ROWS3)) {
	if (!TEST(F_EWRES3))
	    G_fatal_error(_("Field <%s> missing"), "ewres3");
	if (!TEST(F_NSRES3))
	    G_fatal_error(_("Field <%s> missing"), "nsres3");
	if (!TEST(F_COLS3))
	    G_fatal_error(_("Field <%s> missing"), "cols3");
	if (!TEST(F_ROWS3))
	    G_fatal_error(_("Field <%s> missing"), "rows3");
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

