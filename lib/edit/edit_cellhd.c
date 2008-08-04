
/****************************************************************************
 *
 * MODULE:       edit library functions
 * AUTHOR(S):    Originally part of gis lib dir in CERL GRASS code
 *               Subsequent (post-CVS) contributors:
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Eric G. Miller <egm2 jps.net>,
 *               Markus Neteler <neteler itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Bernhard Reiter <bernhard intevation.de>
 * PURPOSE:      libraries for interactively editing raster support data
 * COPYRIGHT:    (C) 1996-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define AS_CELLHD 1
#define AS_WINDOW 0
#define AS_DEF_WINDOW -1

/* modified 26nov to use word region instead of window
 * as far as the USER is concerned.
 */
/*
 **********************************************************************
 *
 *   E_edit_cellhd (cellhd, type)
 *      struct Cell_head *cellhd   (cellhd to be defined)
 *      int type 
 *        0 = region - user input resolutions
 *       -1 = default region - user input resolutions
 *        1 = cellhd - rows and cols must be set
 *
 *   Screen oriented user interactive session for modifying a cell header
 *   or region.
 *   Uses the visual_ask V_ask routines.  As such, programs including
 *   this must load the GRASS library $(VASKLIB) and include $(CURSES) in
 *   in the compile line
 *
 *   returns:
 *      -1 error of some sort, or user cancels the edit
 *       0 ok
 *
 **********************************************************************/

/* Documentation moved here from the g.region man page:
 *  (g.region no longer does interactive; lib/init/set_data.c is the
 *   only thing left using E_edit_cellhd() AFAICT.  --HB 28 Jan 2008)
 *
 * <h2>REGION EDIT PROMPT</h2>
 *
 * Most of the options will require the user to edit a
 * geographic region, be it the current geographic region or
 * one stored in the user's named region definitions
 * (the <kbd>windows</kbd> directory).  A standard prompt is
 * used to perform this edit.  An example is shown below:
 *
 *
 *
 * <pre>
 * ---------------------------------------------------------------
 *  |                         IDENTIFY REGION                     |
 *  |                                                             |
 *  |           ===========  DEFAULT REGION  ==========           |
 *  |           |    Default North: 3402025.00        |           |
 *  |           |                                     |           |
 *  |           |          ===YOUR REGION===          |           |
 *  |           |          |  NORTH EDGE   |          |           |
 *  |           |          |  3402025.00_  |          |           |
 *  |           |          |               |          |           |
 *  | Def West: |WEST EDGE |               |EAST EDGE | Def.East: |
 *  | 233975.00 |233975.00_|               |236025.00_| 236025.00 |
 *  |           |          |  SOUTH EDGE   |          |           |
 *  |           |          |  3399975.00_  |          |           |
 *  |           |          =================          |           |
 *  |           |                                     |           |
 *  |           |    Default South: 3399975.00        |           |
 *  |           =======================================           |
 *  |                                                             |
 *  |              Default   GRID RESOLUTION   Region             |
 *  |               50.00   --- East-West ---  50.00__            |
 *  |               50.00   -- North-South --  50.00__            |
 *  |                                                             |
 *  |                                                             |
 *  |     AFTER COMPLETING ALL ANSWERS, HIT &lt;ESC&gt; TO CONTINUE     |
 *  ---------------------------------------------------------------
 *  </pre>
 *
 *  The fields NORTH EDGE, SOUTH EDGE, WEST EDGE and EAST EDGE,
 *  are the boundaries of the geographic region that the user
 *  can change.  The fields Default North, Default South, Def
 *  West and Def East are the boundaries of the default
 *  geographic region that are displayed for reference and
 *  <em>cannot</em> be changed.  The two GRID RESOLUTION Region
 *  fields (east-west, and north-south) are the geographic
 *  region's cell resolutions that the user can change.  The
 *  two GRID RESOLUTION Default fields list the resolutions of
 *  the default geographic region;  these are displayed for
 *  reference and cannot be changed here by the user.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <grass/vask.h>
#include <grass/gis.h>
#include <grass/edit.h>


static void format_value(int (*func) (double, char *, int),
			 double x, char *buf, int projection);
static void format_northing(double north, char *buf, int projection);
static void format_easting(double east, char *buf, int projection);
static void format_resolution(double res, char *buf, int projection);
static int hitreturn(void);


static char *cellhd_screen[] = {
    "                           IDENTIFY CELL HEADER",
    "",
    "           ============================= DEFAULT REGION ========",
    "           |          Default North:                           |",
    "           |                                                   |",
    "           |           =======  CELL HEADER  =======           |",
    "           |           | NORTH EDGE:               |           |",
    "           |           |                           |           |",
    " Def. West |WEST EDGE  |                           |EAST EDGE  | Def. East",
    "           |           |                           |           |",
    "           |           | SOUTH EDGE:               |           |",
    "           |           =============================           |",
    "           |                                                   |",
    "           |          Default South:                           |",
    "           =====================================================",
    "           PROJECTION:                                ZONE:",
    NULL
};

static char *window_screen[] = {
    "                              IDENTIFY REGION",
    "",
    "           ============================= DEFAULT REGION ========",
    "           |          Default North:                           |",
    "           |                                                   |",
    "           |           =======  YOUR REGION  =======           |",
    "           |           | NORTH EDGE:               |           |",
    "           |           |                           |           |",
    " Def. West |WEST EDGE  |                           |EAST EDGE  | Def. East",
    "           |           |                           |           |",
    "           |           | SOUTH EDGE:               |           |",
    "           |           =============================           |",
    "           |                                                   |",
    "           |          Default South:                           |",
    "           =====================================================",
    "           PROJECTION:                                ZONE:",
    "",
    "                   Default   GRID RESOLUTION   Region",
    "                            --- East-West ---",
    "                            -- North-South --",
    NULL
};

static char *def_window_screen[] = {
    "                         DEFINE THE DEFAULT REGION",
    "",
    "",
    "",
    "",
    "                       ====== DEFAULT REGION =======",
    "                       | NORTH EDGE:               |",
    "                       |                           |",
    "            WEST EDGE  |                           |EAST EDGE",
    "                       |                           |",
    "                       | SOUTH EDGE:               |",
    "                       =============================",
    "",
    "",
    "",
    "           PROJECTION:                                ZONE:",
    "",
    "                             GRID RESOLUTION",
    "                                 East-West:",
    "                               North-South:",
    NULL
};


static void format_value(int (*func) (double, char *, int),
			 double x, char *buf, int projection)
{
    if (projection != PROJECTION_XY) {
	int k = (projection == PROJECTION_LL) ? 3600 : 1;
	int i = (int)(x * k + (x < 0 ? -0.5 : 0.5));

	x = (double)i / k;
    }

    func(x, buf, projection);
    buf[10] = '\0';
}


static void format_northing(double north, char *buf, int projection)
{
    format_value(G_format_northing, north, buf, projection);
}


static void format_easting(double east, char *buf, int projection)
{
    format_value(G_format_easting, east, buf, projection);
}


static void format_resolution(double res, char *buf, int projection)
{
    format_value(G_format_resolution, res, buf, projection);
}


int E_edit_cellhd(struct Cell_head *cellhd, int type)
{
    char ll_north[20];
    char ll_south[20];
    char ll_east[20];
    char ll_west[20];
    char ll_nsres[20];
    char ll_ewres[20];
    char ll_def_north[20];
    char ll_def_south[20];
    char ll_def_east[20];
    char ll_def_west[20];
    char ll_def_ewres[20];
    char ll_def_nsres[20];
    char projection[80];
    char **screen;

    struct Cell_head def_wind;
    double north, south, east, west;
    double nsres, ewres;
    char buf[64], buf2[30], *p;
    short ok;
    int line;
    char *prj;
    char *err;

    if (type == AS_CELLHD && (cellhd->rows <= 0 || cellhd->cols <= 0)) {
	G_message("E_edit_cellhd() - programmer error");
	G_message("  ** rows and cols must be positive **");
	return -1;
    }
    if (type != AS_DEF_WINDOW) {
	if (G_get_default_window(&def_wind) != 1)
	    return -1;

	if (cellhd->proj < 0) {
	    cellhd->proj = def_wind.proj;
	    cellhd->zone = def_wind.zone;
	}
	else if (cellhd->zone < 0)
	    cellhd->zone = def_wind.zone;
    }

    prj = G__projection_name(cellhd->proj);
    if (!prj)
	prj = "** unknown **";
    sprintf(projection, "%d (%s)", cellhd->proj, prj);

    if (type != AS_DEF_WINDOW) {
	if (cellhd->west >= cellhd->east || cellhd->south >= cellhd->north) {
	    cellhd->north = def_wind.north;
	    cellhd->south = def_wind.south;
	    cellhd->west = def_wind.west;
	    cellhd->east = def_wind.east;

	    if (type != AS_CELLHD) {
		cellhd->ew_res = def_wind.ew_res;
		cellhd->ns_res = def_wind.ns_res;
		cellhd->rows = def_wind.rows;
		cellhd->cols = def_wind.cols;
	    }
	}

	if (cellhd->proj != def_wind.proj) {
	    if (type == AS_CELLHD)
		G_message
		    ("header projection %d differs from default projection %d",
		     cellhd->proj, def_wind.proj);
	    else
		G_message
		    ("region projection %d differs from default projection %d",
		     cellhd->proj, def_wind.proj);

	    if (!G_yes("do you want to make them match? ", 1))
		return -1;

	    cellhd->proj = def_wind.proj;
	    cellhd->zone = def_wind.zone;
	}

	if (cellhd->zone != def_wind.zone) {
	    if (type == AS_CELLHD)
		G_message("header zone %d differs from default zone %d",
			  cellhd->zone, def_wind.zone);
	    else
		G_message("region zone %d differs from default zone %d",
			  cellhd->zone, def_wind.zone);

	    if (!G_yes("do you want to make them match? ", 1))
		return -1;

	    cellhd->zone = def_wind.zone;
	}

	*ll_def_north = 0;
	*ll_def_south = 0;
	*ll_def_east = 0;
	*ll_def_west = 0;
	*ll_def_ewres = 0;
	*ll_def_nsres = 0;
	format_northing(def_wind.north, ll_def_north, def_wind.proj);
	format_northing(def_wind.south, ll_def_south, def_wind.proj);
	format_easting(def_wind.east, ll_def_east, def_wind.proj);
	format_easting(def_wind.west, ll_def_west, def_wind.proj);
	format_resolution(def_wind.ew_res, ll_def_ewres, def_wind.proj);
	format_resolution(def_wind.ns_res, ll_def_nsres, def_wind.proj);
    }

    *ll_north = 0;
    *ll_south = 0;
    *ll_east = 0;
    *ll_west = 0;
    *ll_ewres = 0;
    *ll_nsres = 0;
    format_northing(cellhd->north, ll_north, cellhd->proj);
    format_northing(cellhd->south, ll_south, cellhd->proj);
    format_easting(cellhd->east, ll_east, cellhd->proj);
    format_easting(cellhd->west, ll_west, cellhd->proj);
    format_resolution(cellhd->ew_res, ll_ewres, cellhd->proj);
    format_resolution(cellhd->ns_res, ll_nsres, cellhd->proj);

    while (1) {
	ok = 1;

	/* List window options on the screen for the user to answer */
	switch (type) {
	case AS_CELLHD:
	    screen = cellhd_screen;
	    break;
	case AS_DEF_WINDOW:
	    screen = def_window_screen;
	    break;
	default:
	    screen = window_screen;
	    break;
	}

	V_clear();
	line = 0;
	while (*screen)
	    V_line(line++, *screen++);

	/* V_ques ( variable, type, row, col, length) ; */
	V_ques(ll_north, 's', 6, 36, 10);
	V_ques(ll_south, 's', 10, 36, 10);
	V_ques(ll_west, 's', 9, 12, 10);
	V_ques(ll_east, 's', 9, 52, 10);

	if (type != AS_CELLHD) {
	    V_ques(ll_ewres, 's', 18, 48, 10);
	    V_ques(ll_nsres, 's', 19, 48, 10);
	}

	if (type != AS_DEF_WINDOW) {
	    V_const(ll_def_north, 's', 3, 36, 10);
	    V_const(ll_def_south, 's', 13, 36, 10);
	    V_const(ll_def_west, 's', 9, 1, 10);
	    V_const(ll_def_east, 's', 9, 65, 10);

	    if (type != AS_CELLHD) {
		V_const(ll_def_ewres, 's', 18, 21, 10);
		V_const(ll_def_nsres, 's', 19, 21, 10);
	    }
	}

	V_const(projection, 's', 15, 23, (int)strlen(projection));
	V_const(&cellhd->zone, 'i', 15, 60, 3);

	V_intrpt_ok();
	if (!V_call())
	    return -1;

	G_squeeze(ll_north);
	G_squeeze(ll_south);
	G_squeeze(ll_east);
	G_squeeze(ll_west);

	if (type != AS_CELLHD) {
	    G_squeeze(ll_ewres);
	    G_squeeze(ll_nsres);
	}

	if (!G_scan_northing(ll_north, &cellhd->north, cellhd->proj)) {
	    G_warning("Illegal value for north: %s", ll_north);
	    ok = 0;
	}

	if (!G_scan_northing(ll_south, &cellhd->south, cellhd->proj)) {
	    G_warning("Illegal value for south: %s", ll_south);
	    ok = 0;
	}

	if (!G_scan_easting(ll_east, &cellhd->east, cellhd->proj)) {
	    G_warning("Illegal value for east: %s", ll_east);
	    ok = 0;
	}

	if (!G_scan_easting(ll_west, &cellhd->west, cellhd->proj)) {
	    G_warning("Illegal value for west: %s", ll_west);
	    ok = 0;
	}

	if (type != AS_CELLHD) {
	    if (!G_scan_resolution(ll_ewres, &cellhd->ew_res, cellhd->proj)) {
		G_warning("Illegal east-west resolution: %s", ll_ewres);
		ok = 0;
	    }

	    if (!G_scan_resolution(ll_nsres, &cellhd->ns_res, cellhd->proj)) {
		G_warning("Illegal north-south resolution: %s", ll_nsres);
		ok = 0;
	    }
	}

	if (!ok) {
	    hitreturn();
	    continue;
	}

	/* Adjust and complete the cell header */
	north = cellhd->north;
	south = cellhd->south;
	east = cellhd->east;
	west = cellhd->west;
	nsres = cellhd->ns_res;
	ewres = cellhd->ew_res;

	if ((err =
	     G_adjust_Cell_head(cellhd, type == AS_CELLHD,
				type == AS_CELLHD))) {
	    G_message("%s", err);
	    hitreturn();
	    continue;
	}

	if (type == AS_CELLHD) {
	    nsres = cellhd->ns_res;
	    ewres = cellhd->ew_res;
	}

      SHOW:
	fprintf(stderr, "\n\n");
	G_message("  projection:   %s", projection);
	G_message("  zone:         %d", cellhd->zone);

	G_format_northing(cellhd->north, buf, cellhd->proj);
	G_format_northing(north, buf2, cellhd->proj);
	fprintf(stderr, "  north:       %s", buf);

	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to match resolution)");
	}
	fprintf(stderr, "\n");

	G_format_northing(cellhd->south, buf, cellhd->proj);
	G_format_northing(south, buf2, cellhd->proj);
	fprintf(stderr, "  south:       %s", buf);
	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to match resolution)");
	}
	fprintf(stderr, "\n");

	G_format_easting(cellhd->east, buf, cellhd->proj);
	G_format_easting(east, buf2, cellhd->proj);
	fprintf(stderr, "  east:        %s", buf);
	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to match resolution)");
	}
	fprintf(stderr, "\n");

	G_format_easting(cellhd->west, buf, cellhd->proj);
	G_format_easting(west, buf2, cellhd->proj);
	fprintf(stderr, "  west:        %s", buf);
	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to match resolution)");
	}
	fprintf(stderr, "\n\n");

	G_format_resolution(cellhd->ew_res, buf, cellhd->proj);
	G_format_resolution(ewres, buf2, cellhd->proj);
	fprintf(stderr, "  e-w res:     %s", buf);
	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to conform to grid)");
	}
	fprintf(stderr, "\n");

	G_format_resolution(cellhd->ns_res, buf, cellhd->proj);
	G_format_resolution(nsres, buf2, cellhd->proj);
	fprintf(stderr, "  n-s res:     %s", buf);
	if (strcmp(buf, buf2) != 0) {
	    ok = 0;
	    fprintf(stderr, "  (Changed to conform to grid)");
	}
	fprintf(stderr, "\n\n");

	G_message("  total rows:  %15d", cellhd->rows);
	G_message("  total cols:  %15d", cellhd->cols);

	sprintf(buf, "%lf", (double)cellhd->rows * cellhd->cols);
	*(p = strchr(buf, '.')) = 0;
	G_insert_commas(buf);
	G_message("  total cells: %15s", buf);
	fprintf(stderr, "\n");

	if (type != AS_DEF_WINDOW) {
	    if (cellhd->north > def_wind.north) {
		G_warning("north falls outside the default region");
		ok = 0;
	    }

	    if (cellhd->south < def_wind.south) {
		G_warning("south falls outside the default region");
		ok = 0;
	    }

	    if (cellhd->proj != PROJECTION_LL) {
		if (cellhd->east > def_wind.east) {
		    G_warning("east falls outside the default region");
		    ok = 0;
		}

		if (cellhd->west < def_wind.west) {
		    G_warning("west falls outside the default region");
		    ok = 0;
		}
	    }
	}

      ASK:
	fflush(stdin);
	if (type == AS_CELLHD)
	    fprintf(stderr, "\nDo you accept this header? (y/n) [%s] > ",
		    ok ? "y" : "n");
	else
	    fprintf(stderr, "\nDo you accept this region? (y/n) [%s] > ",
		    ok ? "y" : "n");

	if (!G_gets(buf))
	    goto SHOW;

	G_strip(buf);
	switch (*buf) {
	case 0:
	    break;
	case 'y':
	case 'Y':
	    ok = 1;
	    break;
	case 'n':
	case 'N':
	    ok = 0;
	    break;
	default:
	    goto ASK;
	}

	if (ok)
	    return 0;
    }
}


static int hitreturn(void)
{
    char buf[100];

    fprintf(stderr, "hit RETURN -->");
    if (!fgets(buf, 100, stdin))
	exit(EXIT_SUCCESS);

    G_strip(buf);
    if (!strncmp(buf, "exit", sizeof(buf)))
	exit(EXIT_SUCCESS);

    return 0;
}
