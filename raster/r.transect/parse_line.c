#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

#define PI M_PI
#define Radians(x) ((x) * PI/180.0)
#define Degrees(x) ((x) * 180.0/PI)


int
parse_line(const char *key, char **s, double *e1, double *n1, double *e2,
	   double *n2, int projection)
{
    int err;
    double distance, azimuth;

    err = 0;
    if (!G_scan_easting(s[0], e1, projection))
	err |= 1;
    if (!G_scan_northing(s[1], n1, projection))
	err |= 2;
    if (sscanf(s[2], "%lf", &azimuth) != 1)
	err |= 4;
    if (sscanf(s[3], "%lf", &distance) != 1 || distance < 0.0)
	err |= 8;

    if (err) {
	char warningbuff[256];
	char partbuf[64];

	sprintf(warningbuff, "%s=", key);
	sprintf(partbuf, "%s%s%s,",
		err & 1 ? "<" : "", s[0], err & 1 ? ">" : "");
	strcat(warningbuff, partbuf);

	sprintf(partbuf, "%s%s%s,",
		err & 2 ? "<" : "", s[1], err & 2 ? ">" : "");
	strcat(warningbuff, partbuf);

	sprintf(partbuf, "%s%s%s,",
		err & 4 ? "<" : "", s[2], err & 4 ? ">" : "");
	strcat(warningbuff, partbuf);

	sprintf(partbuf, "%s%s%s",
		err & 8 ? "<" : "", s[3], err & 8 ? ">" : "");
	strcat(warningbuff, partbuf);

	G_warning("%s %s", warningbuff, " invalid values(s)");
	return err;
    }

    *e2 = *e1 + distance * sin(Radians(azimuth));
    *n2 = *n1 + distance * cos(Radians(azimuth));

    return 0;
}
