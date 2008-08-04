#include <string.h>
#include <ctype.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>

static int scan_int(char *, void *, int);
static int scan_res(char *, void *, int);
static int scan_easting(char *, void *, int);
static int scan_northing(char *, void *, int);
static int scan_cellsize(char *, void *, int);
static int extract(int, char *, char *, void *, int,
		   int (*)(char *, void *, int));
static int missing(int, char *);

int gethead(FILE * fd, struct Cell_head *cellhd, int *missingval)
{
    int i, ok;
    int nodata, res, s, w, r, c;
    char label[100], value[100];
    char buf[1024];
    char *err;

    s = nodata = res = w = r = c = 0;

    cellhd->zone = G_zone();
    cellhd->proj = G_projection();

    while (nodata == 0 || s == 0 || res == 0 || w == 0 || r == 0 || c == 0) {
	if (!G_getl2(buf, sizeof buf, fd))
	    break;
	*label = *value = '\0';
	sscanf(buf, "%s %s", label, value);
	if (*label == '\0')
	    continue;		/* ignore blank lines */
	for (i = 0; i < strlen(label); i++) {
	    label[i] = tolower(label[i]);
	}


	if (strcmp(label, "ncols") == 0) {
	    if (!extract(c++, label, value, &cellhd->cols, cellhd->proj,
			 scan_int))
		ok = 0;
	    continue;
	}

	if (strcmp(label, "nrows") == 0) {
	    if (!extract(r++, label, value, &cellhd->rows, cellhd->proj,
			 scan_int))
		ok = 0;
	    continue;
	}

	if (strcmp(label, "xllcorner") == 0) {
	    if (!extract(w++, label, value, &cellhd->west, cellhd->proj,
			 scan_easting))
		ok = 0;
	    continue;
	}

	if (strcmp(label, "yllcorner") == 0) {
	    if (!extract(s++, label, value, &cellhd->south, cellhd->proj,
			 scan_northing))
		ok = 0;
	    continue;
	}

	if (strcmp(label, "cellsize") == 0) {
	    if (!extract(res++, label, value, &cellhd->ew_res, cellhd->proj,
			 scan_cellsize))
		ok = 0;

	    cellhd->ns_res = cellhd->ew_res;
	    cellhd->north = cellhd->south + (cellhd->ns_res * cellhd->rows);
	    cellhd->east = cellhd->west + (cellhd->ew_res * cellhd->cols);

	    continue;
	}

	if (strcmp(label, "nodata_value") == 0) {
	    if (!extract(nodata++, label, value, missingval, cellhd->proj,
			 scan_res))
		ok = 0;
	    continue;
	}

	G_warning(_("Illegal line in header"));
	G_warning(buf);

	missing(s, "yllcorner");
	missing(w, "xllcorner");
	missing(r, "nrows");
	missing(c, "ncols");
	missing(res, "cellsize");
	missing(nodata, "nodata_value");
	return 0;
    }

    if (err = G_adjust_Cell_head(cellhd, 1, 1)) {
	G_warning(err);
	return 0;
    }

    return 1;
}

static int scan_int(char *s, void *v, int proj)
{
    char dummy[3];
    int *i = v;

    *dummy = 0;

    if (sscanf(s, "%d%1s", i, dummy) != 1)
	return 0;
    if (*dummy)
	return 0;
    if (*i <= 0)
	return 0;
    return 1;
}

static int scan_res(char *s, void *v, int proj)
{
    char dummy[3];
    int *i = v;

    *dummy = 0;

    if (sscanf(s, "%d%1s", i, dummy) != 1)
	return 0;
    if (*dummy)
	return 0;
    if (*i <= -9999999)
	return 0;
    return 1;
}


static int scan_easting(char *s, void *v, int i)
{
    return G_scan_easting(s, (double *)v, i);
}

static int scan_northing(char *s, void *v, int i)
{
    return G_scan_northing(s, (double *)v, i);
}

static int scan_cellsize(char *s, void *v, int i)
{
    return G_scan_resolution(s, (double *)v, i);
}

static int extract(int count, char *label, char *value,
		   void *data, int proj, int (*scanner) (char *, void *, int))
{
    if (count) {
	G_warning(_("Duplicate \"%s\" field in header"), label);
	return 0;
    }
    if (scanner(value, data, proj))
	return 1;
    G_warning(_("Illegal \"%s\" value in header: \"%s\""), label, value);
    return 0;
}

static int missing(int count, char *label)
{
    if (count)
	return 0;
    G_warning(_("\"%s\" field missing from header"), label);
    return 1;
}
