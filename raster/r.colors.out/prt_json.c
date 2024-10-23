#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/parson.h>

void write_json_rule(DCELL *val, DCELL *min, DCELL *max, int r, int g, int b,
                     JSON_Array *root_array, int perc)
{
    static DCELL v0;
    static int r0 = -1, g0 = -1, b0 = -1;

    if (v0 == *val && r0 == r && g0 == g && b0 == b)
        return;
    v0 = *val, r0 = r, g0 = g, b0 = b;
    JSON_Value *color_value = json_value_init_object();
    JSON_Object *color_object = json_object(color_value);

    char rgb_string[20];
    snprintf(rgb_string, sizeof(rgb_string), "rgb(%d, %d, %d)", r, g, b);

    if (perc)
        json_object_set_number(color_object, "value",
                               100 * (*val - *min) / (*max - *min));
    else
        json_object_set_number(color_object, "value", *val);

    json_object_set_string(color_object, "rgb", rgb_string);

    json_array_append_value(root_array, color_value);
}

void print_json_colors(struct Colors *colors, DCELL min, DCELL max, FILE *fp,
                       int perc)
{
    JSON_Value *root_value = json_value_init_array();
    JSON_Array *root_array = json_array(root_value);

    int i, count;

    count = 0;
    if (colors->version < 0) {
        CELL lo, hi;

        Rast_get_c_color_range(&lo, &hi, colors);

        for (i = lo; i <= hi; i++) {
            unsigned char r, g, b, set;
            DCELL val = (DCELL)i;

            Rast_lookup_c_colors(&i, &r, &g, &b, &set, 1, colors);
            write_json_rule(&val, &min, &max, r, g, b, root_array, perc);
        }
    }
    else {
        count = Rast_colors_count(colors);

        for (i = 0; i < count; i++) {
            DCELL val1, val2;
            unsigned char r1, g1, b1, r2, g2, b2;

            Rast_get_fp_color_rule(&val1, &r1, &g1, &b1, &val2, &r2, &g2, &b2,
                                   colors, count - 1 - i);

            write_json_rule(&val1, &min, &max, r1, g1, b1, root_array, perc);
            write_json_rule(&val2, &min, &max, r2, g2, b2, root_array, perc);
        }
    }

    {
        int r, g, b;

        Rast_get_null_value_color(&r, &g, &b, colors);
        JSON_Value *nv_value = json_value_init_object();
        JSON_Object *nv_object = json_object(nv_value);
        char nv_rgb_string[20];
        snprintf(nv_rgb_string, sizeof(nv_rgb_string), "rgb(%d, %d, %d)", r, g,
                 b);
        json_object_set_string(nv_object, "value", "nv");
        json_object_set_string(nv_object, "rgb", nv_rgb_string);
        json_array_append_value(root_array, nv_value);

        Rast_get_default_color(&r, &g, &b, colors);
        JSON_Value *default_value = json_value_init_object();
        JSON_Object *default_object = json_object(default_value);
        char default_rgb_string[20];
        snprintf(default_rgb_string, sizeof(default_rgb_string),
                 "rgb(%d, %d, %d)", r, g, b);
        json_object_set_string(default_object, "value", "default");
        json_object_set_string(default_object, "rgb", default_rgb_string);
        json_array_append_value(root_array, default_value);
    }

    char *json_string = json_serialize_to_string_pretty(root_value);
    if (!json_string) {
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }

    fputs(json_string, fp);

    json_free_serialized_string(json_string);
    json_value_free(root_value);

    if (fp != stdout)
        fclose(fp);
}
