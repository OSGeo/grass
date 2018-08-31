
/******************************************************************************
 G_lat_scan (buf, lat)
     char *buf;
     double *lat;

 G_lon_scan (buf, lon)
     char *buf;
     double *lon;

 G_llres_scan (buf, res)
     char *buf;
     double *res;

 Convert ascii string representations of latitude/longitude to a double.
 The string format is:

       dd:mm:ss.ffh

 where:
       dd is degrees, 0-90 for latitude, 0-180 for longitude
       mm is minutes, 0-59
       ss is seconds, 0-59
       ff is fractions of a second, >= 0
       h  is 'n' or 's' for latitude,
             'e' or 'w' for longitude.
	     missing for resolution

 lat (or lon) is set to the double value for the lat/lon represented in buf.

 lat is always in the range  -90 thru  90,
 lon is always in the range -180 thru 180.

 note: southern latitude and western longitude are returned as negative values.

 returns 1 if input format is ok, 0 otherwise.
******************************************************************************/
#include <grass/gis.h>

#define LL_TOLERANCE 10

static int scan_ll(const char *, const char *, double *, int);
static int check_minutes(const char *);
static int check_seconds(const char *);

int G_lat_scan(const char *buf, double *lat)
{
    return scan_ll(buf, "sn", lat, 90 + LL_TOLERANCE);
}

int G_lon_scan(const char *buf, double *lon)
{
    return scan_ll(buf, "we", lon, 360 + LL_TOLERANCE);
}

int G_llres_scan(const char *buf, double *res)
{
    char tbuf[100];

    sprintf(tbuf, "%se", buf);
    return scan_ll(tbuf, "we", res, 0);
}

#define MARKER 1
static int scan_ll(const char *buf, const char *dir, double *result, int max)
{
    char h[100];
    int d, m, s;
    char ps[20], *pps;
    double p, f;
    double pm = 0.0;
    char tbuf[100];

    sprintf(tbuf, "%s%c", buf, MARKER);	/* add a marker at end of string */
    buf = tbuf;

    if (sscanf(buf, "%d:%d:%d.%[0123456789]%[^\n]", &d, &m, &s, ps, h) == 5) {
	p = 0.0;
	f = .1;
	for (pps = ps; *pps; pps++) {
	    p += (*pps - '0') * f;
	    f /= 10.0;
	}
    }
    else if (sscanf(buf, "%d:%d:%d%[^\n]", &d, &m, &s, h) == 4) {
	p = 0.0;
    }
    else if (sscanf(buf, "%d:%d.%[0123456789]%[^\n]", &d, &m, ps, h) == 4) {
	s = 0;
	p = 0.0;
	f = .1;
	for (pps = ps; *pps; pps++) {
	    pm += (*pps - '0') * f;
	    f /= 10.0;
	}
    }
    else if (sscanf(buf, "%d:%d%[^\n]", &d, &m, h) == 3) {
	p = 0.0;
	s = 0;
    }
    else if (sscanf(buf, "%d%[^\n]", &d, h) == 2) {
	p = 0.0;
	s = m = 0;
    }
    else
	return 0;

    if (d < 0)
	return 0;
    if (m < 0 || m >= 60)
	return 0;
    if (s < 0 || s >= 60)
	return 0;

    if (max) {
	if (d > max)
	    return 0;
	if (d == max && (m > 0 || s > 0 || p > 0.0))
	    return 0;
    }

    if (m && !check_minutes(buf))
	return 0;
    if (s && !check_seconds(buf))
	return 0;

    *result = d + (m + pm) / 60.0 + (s + p) / 3600.0;

    G_strip(h);

    if (*result == 0.0 && *h == MARKER)
	return (1);

    if (*h >= 'A' && *h <= 'Z')
	*h += 'a' - 'A';

    if (*h != dir[0] && *h != dir[1])
	return 0;

    if (h[1] != MARKER)
	return 0;

    if (*h == dir[0] && *result != 0.0)
	*result = -(*result);

    return 1;
}

static int check_minutes(const char *buf)
{
    /* skip over degrees */
    while (*buf != ':')
	if (*buf++ == 0)
	    return 1;
    buf++;

    /* must have 2 digits for minutes */
    if (*buf < '0' || *buf > '9')
	return 0;
    buf++;
    if (*buf < '0' || *buf > '9')
	return 0;
    buf++;
    return (*buf < '0' || *buf > '9');
}

static int check_seconds(const char *buf)
{
    /* skip over degrees */
    while (*buf != ':')
	if (*buf++ == 0)
	    return 1;
    buf++;
    /* skip over minutes */
    while (*buf != ':')
	if (*buf++ == 0)
	    return 1;
    buf++;

    /* must have 2 digits for seconds */
    if (*buf < '0' || *buf > '9')
	return 0;
    buf++;
    if (*buf < '0' || *buf > '9')
	return 0;
    buf++;
    return (*buf < '0' || *buf > '9');
}
