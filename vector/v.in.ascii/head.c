#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/Vect.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int read_head(FILE * dascii, struct Map_info *Map)
{
    char buff[1024];
    char *ptr;

    for (;;) {
	if (0 == G_getl2(buff, sizeof(buff) - 1, dascii))
	    return (0);

	/* Last line of header */
	if (strncmp(buff, "VERTI:", 6) == 0)
	    return (0);

	if (!(ptr = G_index(buff, ':')))
	    G_fatal_error(_("Unexpected data in vector head:\n[%s]"), buff);

	ptr++;			/* Search for the start of text */
	while (*ptr == ' ')
	    ptr++;

	if (strncmp(buff, "ORGANIZATION:", 12) == 0)
	    Vect_set_organization(Map, ptr);
	else if (strncmp(buff, "DIGIT DATE:", 11) == 0)
	    Vect_set_date(Map, ptr);
	else if (strncmp(buff, "DIGIT NAME:", 11) == 0)
	    Vect_set_person(Map, ptr);
	else if (strncmp(buff, "MAP NAME:", 9) == 0)
	    Vect_set_map_name(Map, ptr);
	else if (strncmp(buff, "MAP DATE:", 9) == 0)
	    Vect_set_map_date(Map, ptr);
	else if (strncmp(buff, "MAP SCALE:", 10) == 0)
	    Vect_set_scale(Map, atoi(ptr));
	else if (strncmp(buff, "OTHER INFO:", 11) == 0)
	    Vect_set_comment(Map, ptr);
	else if (strncmp(buff, "ZONE:", 5) == 0 ||
		 strncmp(buff, "UTM ZONE:", 9) == 0)
	    Vect_set_zone(Map, atoi(ptr));
	else if (strncmp(buff, "WEST EDGE:", 10) == 0) {
	}
	else if (strncmp(buff, "EAST EDGE:", 10) == 0) {
	}
	else if (strncmp(buff, "SOUTH EDGE:", 11) == 0) {
	}
	else if (strncmp(buff, "NORTH EDGE:", 11) == 0) {
	}
	else if (strncmp(buff, "MAP THRESH:", 11) == 0)
	    Vect_set_thresh(Map, atof(ptr));
	else {
	    G_warning(_("Unknown keyword <%s> in vector head"), buff);
	}
    }
    /* NOTREACHED */
}
