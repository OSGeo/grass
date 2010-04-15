#include <grass/imagery.h>
#include <string.h>
#include <grass/gis.h>

/* makes a three part title with location, mapset info */
char *I_location_info(const char *middle)
{
    char left[80];
    char right[80];
    char *buf;
    int len, buf_len;

    G_snprintf(left, 80, "LOCATION: %s", G_location());
    G_snprintf(right, 80, "MAPSET: %s", G_mapset());
    len = 79 - strlen(left) - strlen(middle) - strlen(right);
    buf_len = len + strlen(left) + strlen(middle) + strlen(right);
    buf = (char*)G_calloc(buf_len, sizeof(char));
    G_snprintf(buf, buf_len, "%s%*s%s%*s%s",
	    left, len / 2, "", middle, len / 2, "", right);

    return buf;
}
