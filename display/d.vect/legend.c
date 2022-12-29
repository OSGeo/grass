#include <grass/gis.h>
#include <grass/vector.h>
#include "local_proto.h"

void write_into_legfile(struct Map_info *Map, int type, const char *leglab, const char *name_map, const char *icon,
                       const char *size, const char *color, const char *fcolor, const char *width, const char *icon_area,
                       const char *icon_line, const char *size_column)
{
    int nfeatures;
    FILE *fd;
    char *leg_file;
    char map[GNAME_MAX];
    char *ptr;
    strcpy(map, name_map);
    strtok_r(map, "@", &ptr);

    if (size_column)
        size = "-1";

    /* Write into legend file */
    leg_file = getenv("GRASS_LEGEND_FILE");
    if (leg_file) {
        fd = fopen(leg_file, "a");

        /* Point */
        if (type & GV_POINT){
            nfeatures = Vect_get_num_primitives(Map, GV_POINT);
            if (nfeatures > 0) {
                if (leglab)
                    fprintf(fd, "%s|", leglab);
                else
                    fprintf(fd, "%s|", map);
                fprintf(fd, "%s|%s|lf|%s|%s|%s", icon, size, color, fcolor, width);
                fprintf(fd, "|%s|%d\n", "point", nfeatures);
            }
        }

        /* Line */
        if (type & GV_LINE){
            nfeatures = Vect_get_num_primitives(Map, GV_LINE);
            if (nfeatures > 0){
                if (leglab)
                    fprintf(fd, "%s|", leglab);
                else
                    fprintf(fd, "%s|", map);
                fprintf(fd, "%s|%s|lf|%s|%s|%s", icon_line, size, color, fcolor, width);
                fprintf(fd, "|%s|%d\n", "line", nfeatures);
            }
        }

        /* Area */
        if (type & GV_AREA){
            nfeatures = Vect_get_num_primitives(Map, GV_BOUNDARY);
            if (nfeatures > 0) {
                if (leglab)
                    fprintf(fd, "%s|", leglab);
                else
                    fprintf(fd, "%s|", map);
                fprintf(fd, "%s|%s|lf|%s|%s|%s", icon_area, size, color, fcolor, width);
                fprintf(fd, "|%s|%d\n", "area", nfeatures);
            }
        }
        /* Centroid */
        if (type & GV_CENTROID){
            nfeatures = Vect_get_num_primitives(Map, GV_CENTROID);
            if (nfeatures > 0) {
                if (leglab)
                    fprintf(fd, "%s|", leglab);
                else
                    fprintf(fd, "%s|", map);
                fprintf(fd, "%s|%s|lf|%s|%s|%s", icon, size, color, fcolor, width);
                fprintf(fd, "|%s|%d\n", "centroid", nfeatures);
            }
        }

        fclose(fd);
    }
}
