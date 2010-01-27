#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


#define DOT 	   "."		/* for determining data type -tw */
#define INT	   "int"
#define FLOAT      "float"
#define DOUBLE     "double"
#define TMPBUFSIZE 8192

static int missing(int, char *);
static int extract(int, char *, char *, void *, int, int (*)());
static int scan_int(char *, int *, int);

const char gs_ascii_flag[5] = { "DSAA" };


int getgrdhead(FILE * fd, struct Cell_head *cellhd)
{
    char grd_flag[6];
    int nc, nr;
    float xmin, xmax, ymin, ymax, zmin, zmax;

    /* make sure the input file is positioned at the beginning */
    rewind(fd);

    /* read and check the flag on the first line */
    fgets(grd_flag, sizeof(grd_flag), fd);
    if (strncmp(gs_ascii_flag, grd_flag, strlen(gs_ascii_flag))) {
	G_warning(_("input file is not a Surfer ascii grid file"));
	return 0;
    }

    /* read the row and column dimensions */
    if (fscanf(fd, "%d %d \n", &nc, &nr) != 2) {
	G_warning(_("error reading the column and row dimension from the Surfer grid file"));
	return 0;
    }

    /* read the range of x values */
    if (fscanf(fd, "%f %f \n", &xmin, &xmax) != 2) {
	G_warning(_("error reading the X range from the Surfer grid file"));
	return 0;
    }

    /* read the range of y values */
    if (fscanf(fd, "%f %f \n", &ymin, &ymax) != 2) {
	G_warning(_("error reading the Y range from the Surfer grid file"));
	return 0;
    }

    /* read the range of z values (not used) */
    if (fscanf(fd, "%f %f \n", &zmin, &zmax) != 2) {
	G_warning(_("error reading the Z range from the Surfer grid file"));
	return 0;
    }

    /* initialize the cell header */
    cellhd->zone = G_zone();
    cellhd->proj = G_projection();
    cellhd->rows = nr;
    cellhd->cols = nc;

    cellhd->ew_res = (double)(xmax - xmin) / (nc - 1);
    cellhd->ns_res = (double)(ymax - ymin) / (nr - 1);
    /* the Surfer grid specifies x,y locations of gridded points.  The GRASS raster
       specifies an area covered by rectangular cells centerd at gridded points.
       That difference requires an adjustment */
    cellhd->north = ymax + cellhd->ns_res / 2.;
    cellhd->south = ymin - cellhd->ns_res / 2.;
    cellhd->east = xmax + cellhd->ew_res / 2.;
    cellhd->west = xmin - cellhd->ew_res / 2.;

    return 1;

}

int gethead(FILE * fd,
	    struct Cell_head *cellhd,
	    RASTER_MAP_TYPE * d_type, DCELL * mult, char **nval)
{
    int n, s, e, w, r, c;
    char label[100], value[100];
    char buf[1024];
    int ret, len;

    /* rsb fix */
    fpos_t p;

    n = s = e = w = r = c = 0;

    cellhd->zone = G_zone();
    cellhd->proj = G_projection();

    /*      while (n == 0 || s== 0 || e == 0 || w == 0 || r == 0 || c == 0) */

    while (1) {
	/* rsb fix */
	if (fgetpos(fd, &p) != 0)
	    G_fatal_error(_("error getting file position"));

	if (!G_getl2(buf, sizeof(buf), fd))
	    break;

	len = strlen(buf);

	*label = *value = '\0';
	if (NULL == strstr(buf, ":"))
	    break;
	if (sscanf(buf, "%[^:]:%s", label, value) != 2)
	    break;
	if (*label == '\0')
	    continue;		/* ignore blank lines */

	if (strcmp(label, "north") == 0) {
	    extract(n++, label, value, &cellhd->north, cellhd->proj,
		    G_scan_northing);
	    continue;
	}

	if (strcmp(label, "south") == 0) {
	    extract(s++, label, value, &cellhd->south, cellhd->proj,
		    G_scan_northing);
	    continue;
	}

	if (strcmp(label, "east") == 0) {
	    extract(e++, label, value, &cellhd->east, cellhd->proj,
		    G_scan_easting);
	    continue;
	}

	if (strcmp(label, "west") == 0) {
	    extract(w++, label, value, &cellhd->west, cellhd->proj,
		    G_scan_easting);
	    continue;
	}

	if (strcmp(label, "rows") == 0) {
	    extract(r++, label, value, &cellhd->rows, cellhd->proj, scan_int);
	    continue;
	}

	if (strcmp(label, "cols") == 0) {
	    extract(c++, label, value, &cellhd->cols, cellhd->proj, scan_int);
	    continue;
	}
	/* try to read optional header fields */

	if (strcmp(label, "type") == 0) {	/* if not exist then the file scan */
	    if (*d_type < 0) {	/* if data type not set on command line */
		if (!strncmp(value, INT, strlen(INT)))
		    *d_type = CELL_TYPE;
		else if (!strncmp(value, FLOAT, strlen(FLOAT)))
		    *d_type = FCELL_TYPE;
		else if (!strncmp(value, DOUBLE, strlen(DOUBLE)))
		    *d_type = DCELL_TYPE;
		else {
		    G_warning(_("illegal type field: using type int"));
		    *d_type = CELL_TYPE;
		}
	    }
	    else
		G_warning(_("ignoring type filed in header, type is set on command line"));
	    continue;
	}

	if (strcmp(label, "multiplier") == 0) {
	    if (Rast_is_d_null_value(mult)) {	/* if mult not set on commant line */
		if (sscanf(value, "%lf", mult) != 1) {
		    G_warning(_("illegal multiplier field: using 1.0"));
		    *mult = 1.0;
		}
	    }
	    else
		G_warning(_("ignoring multiplier filed in header, multiplier is set on command line"));
	    continue;
	}

	if (strcmp(label, "null") == 0) {
	    if (!(*nval))	/* if null val string not set on command line */
		*nval = G_store(value);
	    else
		G_warning(_("ignoring null filed in header, null string is set on command line"));
	    continue;
	}

    }				/* while */
    /* the line read was not a header line, but actually
       the first data line, so put it back on the stack and break */
    /* rsb fix */
    fsetpos(fd, &p);

    missing(n, "north");
    missing(s, "south");
    missing(e, "east");
    missing(w, "west");
    missing(r, "rows");
    missing(c, "cols");

    if (!(*nval))
	*nval = G_store("*");
    if (Rast_is_d_null_value(mult))
	*mult = 1.0;
    /* if data type is not set, then scan data to find out data type */
    if (*d_type < 0) {
	ret = file_scan(fd);
	if (!ret)
	    *d_type = DCELL_TYPE;
	else if (ret == 1)
	    *d_type = CELL_TYPE;
	else {
	    G_warning(_("error in ascii data format"));
	    return 0;
	}
    }

    G_adjust_Cell_head(cellhd, 1, 1);

    return 1;
}

static int scan_int(char *s, int *i, int proj)
{
    char dummy[3];

    *dummy = 0;

    if (sscanf(s, "%d%1s", i, dummy) != 1)
	return 0;
    if (*dummy)
	return 0;
    if (*i <= 0)
	return 0;
    return 1;
}

static int extract(int count,
		   char *label, char *value,
		   void *data, int proj, int (*scanner) ())
{
    if (count) {
	G_warning(_("Duplicate \"%s\" field in header"), label);
	return 0;
    }
    if (scanner(value, data, proj))
	return 1;
    G_warning(_("Illegal \"%s\" value in header: %s"), label, value);
    return 0;
}

static int missing(int count, char *label)
{
    if (count)
	return 0;
    G_warning(_("\"%s\" field missing from header"), label);
    return 1;
}

/* file_scan(): determine data type in ascii format */
int file_scan(FILE * fd)
{
    long curpos;
    char tmpbuf[TMPBUFSIZE];
    size_t size = TMPBUFSIZE;

    if ((curpos = G_ftell(fd)) == -1)
	return -1;
    while (!feof(fd)) {
	if (size != fread(tmpbuf, sizeof(char), size, fd)) {
	    if (!feof(fd))
		return -1;
	}
	if (strstr(tmpbuf, DOT) != NULL) {
	    G_fseek(fd, curpos - 1L, SEEK_SET);
	    return 0;
	}
    }
    G_fseek(fd, curpos - 1L, SEEK_SET);
    return 1;
}
