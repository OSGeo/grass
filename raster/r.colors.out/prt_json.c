#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/parson.h>
#include <grass/raster.h>

#include "local_proto.h"

#define COLOR_STRING_LENGTH 30

/*!
   \brief Closes the file if it is not stdout.

   \param fp file where to print color table rules
 */
static void close_file(FILE *fp)
{
    if (fp != stdout)
        fclose(fp);
}

/*!
   \brief Converts RGB color values to HSV format.

   \note This implementation is experimental and may be subject to change.

   \param r red component of the RGB color
   \param g green component of the RGB color
   \param b blue component of the RGB color
   \param[out] h pointer to store the calculated hue
   \param[out] s pointer to store the calculated saturation
   \param[out] v pointer to store the calculated value
 */
static void rgb_to_hsv(int r, int g, int b, float *h, float *s, float *v)
{
    float r_norm = (float)r / 255.0f;
    float g_norm = (float)g / 255.0f;
    float b_norm = (float)b / 255.0f;

    float cmax = MAX(r_norm, MAX(g_norm, b_norm));
    float cmin = MIN(r_norm, MIN(g_norm, b_norm));
    float diff = cmax - cmin;

    if (cmax == cmin) {
        *h = 0;
    }
    else if (cmax == r_norm) {
        *h = fmodf((60.0f * ((g_norm - b_norm) / diff) + 360.0f), 360.0f);
    }
    else if (cmax == g_norm) {
        *h = fmodf((60.0f * ((b_norm - r_norm) / diff) + 120.0f), 360.0f);
    }
    else {
        *h = fmodf((60.0f * ((r_norm - g_norm) / diff) + 240.0f), 360.0f);
    }

    if (cmax == 0) {
        *s = 0;
    }
    else {
        *s = (diff / cmax) * 100.0f;
    }

    *v = cmax * 100.0f;
}

/*!
   \brief Writes color entry in JSON in specified clr_frmt.

   \param r red component of RGB color
   \param g green component of RGB color
   \param b blue component of RGB color
   \param clr_frmt color format to be used (RGB, HEX, HSV, TRIPLET).
   \param color_object pointer to the JSON object
 */
static void set_color(int r, int g, int b, enum ColorFormat clr_frmt,
                      JSON_Object *color_object)
{
    char color_string[COLOR_STRING_LENGTH];
    float h, s, v;

    switch (clr_frmt) {
    case RGB:
        snprintf(color_string, sizeof(color_string), "rgb(%d, %d, %d)", r, g,
                 b);
        json_object_set_string(color_object, "rgb", color_string);
        break;

    case HEX:
        snprintf(color_string, sizeof(color_string), "#%02X%02X%02X", r, g, b);
        json_object_set_string(color_object, "hex", color_string);
        break;

    case HSV:
        rgb_to_hsv(r, g, b, &h, &s, &v);
        snprintf(color_string, sizeof(color_string), "hsv(%d, %d, %d)", (int)h,
                 (int)s, (int)v);
        json_object_set_string(color_object, "hsv", color_string);
        break;

    case TRIPLET:
        snprintf(color_string, sizeof(color_string), "%d:%d:%d", r, g, b);
        json_object_set_string(color_object, "triplet", color_string);
        break;
    }
}

/*!
   \brief Writes a JSON rule for a specific color entry.

   \param val pointer to the DCELL value
   \param min,max minimum and maximum value for percentage output (used only
           when \p perc is non-zero)
   \param r red component of RGB color
   \param g green component of RGB color
   \param b blue component of RGB color
   \param root_array pointer to the JSON array
   \param perc TRUE for percentage output
   \param clr_frmt color format to be used (RBG, HEX, HSV, TRIPLET).
   \param fp file where to print color table rules
   \param root_value pointer to json value
 */
static void write_json_rule(DCELL *val, DCELL *min, DCELL *max, int r, int g,
                            int b, JSON_Array *root_array, int perc,
                            enum ColorFormat clr_frmt, FILE *fp,
                            JSON_Value *root_value)
{
    static DCELL v0;
    static int r0 = -1, g0 = -1, b0 = -1;

    // Skip writing if the current color is the same as the last one
    if (v0 == *val && r0 == r && g0 == g && b0 == b)
        return;
    // Update last processed values
    v0 = *val, r0 = r, g0 = g, b0 = b;

    JSON_Value *color_value = json_value_init_object();
    if (color_value == NULL) {
        json_value_free(root_value);
        close_file(fp);
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));
    }
    JSON_Object *color_object = json_object(color_value);

    // Set the value as a percentage if requested, otherwise set it as-is
    if (perc)
        json_object_set_number(color_object, "value",
                               100 * (*val - *min) / (*max - *min));
    else
        json_object_set_number(color_object, "value", *val);

    set_color(r, g, b, clr_frmt, color_object);

    json_array_append_value(root_array, color_value);
}

/*!
   \brief Print color table in JSON format

   \param colors pointer to Colors structure
   \param min,max minimum and maximum value for percentage output (used only
           when \p perc is non-zero)
   \param fp file where to print color table rules
   \param perc TRUE for percentage output
   \param clr_frmt color format to be used (RBG, HEX, HSV, TRIPLET).
 */
void print_json_colors(struct Colors *colors, DCELL min, DCELL max, FILE *fp,
                       int perc, enum ColorFormat clr_frmt)
{
    JSON_Value *root_value = json_value_init_array();
    if (root_value == NULL) {
        close_file(fp);
        G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
    }
    JSON_Array *root_array = json_array(root_value);

    if (colors->version < 0) {
        /* 3.0 format */
        CELL lo, hi;

        // Retrieve the integer color range
        Rast_get_c_color_range(&lo, &hi, colors);

        for (int i = lo; i <= hi; i++) {
            unsigned char r, g, b, set;
            DCELL val = (DCELL)i;

            // Look up the color for the current value and write JSON rule
            Rast_lookup_c_colors(&i, &r, &g, &b, &set, 1, colors);
            write_json_rule(&val, &min, &max, r, g, b, root_array, perc,
                            clr_frmt, fp, root_value);
        }
    }
    else {
        // Get the count of floating-point color rules
        int count = Rast_colors_count(colors);

        for (int i = 0; i < count; i++) {
            DCELL val1, val2;
            unsigned char r1, g1, b1, r2, g2, b2;

            // Retrieve the color rule values and their respective RGB colors
            Rast_get_fp_color_rule(&val1, &r1, &g1, &b1, &val2, &r2, &g2, &b2,
                                   colors, count - 1 - i);

            // write JSON rule
            write_json_rule(&val1, &min, &max, r1, g1, b1, root_array, perc,
                            clr_frmt, fp, root_value);
            write_json_rule(&val2, &min, &max, r2, g2, b2, root_array, perc,
                            clr_frmt, fp, root_value);
        }
    }

    //  Add special color entries for "null" and "default" values
    {
        int r, g, b;

        // Get RGB color for null values and create JSON entry
        Rast_get_null_value_color(&r, &g, &b, colors);
        JSON_Value *nv_value = json_value_init_object();
        if (nv_value == NULL) {
            json_value_free(root_value);
            close_file(fp);
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        JSON_Object *nv_object = json_object(nv_value);
        json_object_set_string(nv_object, "value", "nv");
        set_color(r, g, b, clr_frmt, nv_object);
        json_array_append_value(root_array, nv_value);

        // Get RGB color for default values and create JSON entry
        Rast_get_default_color(&r, &g, &b, colors);
        JSON_Value *default_value = json_value_init_object();
        if (default_value == NULL) {
            json_value_free(root_value);
            close_file(fp);
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        JSON_Object *default_object = json_object(default_value);
        json_object_set_string(default_object, "value", "default");
        set_color(r, g, b, clr_frmt, default_object);
        json_array_append_value(root_array, default_value);
    }

    // Serialize JSON array to a string and print to the file
    char *json_string = json_serialize_to_string_pretty(root_value);
    if (!json_string) {
        json_value_free(root_value);
        close_file(fp);
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }

    fputs(json_string, fp);

    json_free_serialized_string(json_string);
    json_value_free(root_value);

    close_file(fp);
}
