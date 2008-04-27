#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/Vect.h>
#include <grass/gis.h>

int write_head(FILE * dascii, struct Map_info *Map)
{
    fprintf(dascii, "ORGANIZATION: %s\n", Vect_get_organization(Map));
    fprintf(dascii, "DIGIT DATE:   %s\n", Vect_get_date(Map));
    fprintf(dascii, "DIGIT NAME:   %s\n", Vect_get_person(Map));
    fprintf(dascii, "MAP NAME:     %s\n", Vect_get_map_name(Map));
    fprintf(dascii, "MAP DATE:     %s\n", Vect_get_map_date(Map));
    fprintf(dascii, "MAP SCALE:    %d\n", Vect_get_scale(Map));
    fprintf(dascii, "OTHER INFO:   %s\n", Vect_get_comment(Map));
    fprintf(dascii, "ZONE:         %d\n", Vect_get_zone(Map));
    fprintf(dascii, "MAP THRESH:   %f\n", Vect_get_thresh(Map));
    return (0);
}
