#include <grass/imagery.h>
#include <string.h>
#include <grass/gis.h>

/* makes a three part title with location, mapset info */
int I_location_info(char *buf, const char *middle)
{
    char left[80];
    char right[80];
    int len;

    sprintf(left, "LOCATION: %s", G_location());
    sprintf(right, "MAPSET: %s", G_mapset());
    len = 79 - strlen(left) - strlen(middle) - strlen(right);
    sprintf(buf, "%s%*s%s%*s%s",
	    left, len / 2, "", middle, len / 2, "", right);

    return 0;
}
