#include <grass/gis.h>
#include <grass/parson.h>

enum OutputFormat { PLAIN, JSON };

void write_json_rule(DCELL *val, DCELL *min, DCELL *max, int r, int g, int b,
                     JSON_Array *root_array, int perc);

void Rast_json_print_colors(struct Colors *colors, DCELL min, DCELL max,
                            FILE *fp, int perc);
