#include <grass/gis.h>
#include <grass/parson.h>

enum ColorFormat { RGB, HEX, HSV, TRIPLET };

void print_json_colors(struct Colors *colors, DCELL min, DCELL max, FILE *fp,
                       int perc, enum ColorFormat clr_frmt);
