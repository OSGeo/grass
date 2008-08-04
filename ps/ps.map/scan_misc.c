#include "ps_info.h"
#include <string.h>

static int scan_percent(char *, double *, double, double);

int scan_easting(char *buf, double *f)
{
    if (scan_percent(buf, f, PS.w.west, PS.w.east))
	return 1;
    return G_scan_easting(buf, f, PS.w.proj);
}

int scan_northing(char *buf, double *f)
{
    if (scan_percent(buf, f, PS.w.south, PS.w.north))
	return 1;
    return G_scan_northing(buf, f, PS.w.proj);
}

int scan_resolution(char *buf, double *f)
{
    return G_scan_resolution(buf, f, PS.w.proj);
}

static int scan_percent(char *buf, double *f, double min, double max)
{
    char percent[3];

    *percent = 0;
    if (sscanf(buf, "%lf%2s", f, percent) != 2)
	return 0;
    if (strcmp(percent, "%") != 0)
	return 0;
    *f = min + (max - min) * (*f / 100.0);
    return 1;
}
