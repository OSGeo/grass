#include <grass/gis.h>
#include <grass/parson.h>

void write_json_rule(DCELL *val, DCELL *min, DCELL *max, int r, int g, int b,
                     JSON_Array *root_array, int perc);

void print_json_colors(struct Colors *colors, DCELL min, DCELL max, FILE *fp,
                       int perc);
