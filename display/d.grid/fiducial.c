#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "local_proto.h"


void plot_cross(double easting, double northing, int color, double rotation)
{
    plot_symbol(easting, northing, color, rotation, "basic/cross1",
                MARK_CROSS);
}

void plot_fiducial(double easting, double northing, int color,
                   double rotation)
{
    plot_symbol(easting, northing, color, rotation + 45.0, "extra/fiducial",
                MARK_FIDUCIAL);
}

void plot_dot(double easting, double northing, int color)
{
    plot_symbol(easting, northing, color, 0.0, "basic/point", MARK_DOT);
}

void plot_symbol(double easting, double northing, int color, double rotation,
                 char *symbol_name, int mark_type)
{
    SYMBOL *Symb;
    RGBA_Color *line_color, *fill_color;
    int R, G, B;
    double size = 16.0;
    int tolerance = 0;

    line_color = G_malloc(sizeof(RGBA_Color));
    fill_color = G_malloc(sizeof(RGBA_Color));

    if (D_color_number_to_RGB(color, &R, &G, &B) == 0)
        /* fall back to black on failure */
        G_str_to_color(DEFAULT_FG_COLOR, &R, &G, &B);

    line_color->r = (unsigned char)R;
    line_color->g = (unsigned char)G;
    line_color->b = (unsigned char)B;
    line_color->a = RGBA_COLOR_OPAQUE;

    fill_color->a = RGBA_COLOR_NONE;

    if (mark_type == MARK_DOT) {
        size = 5;
        fill_color->r = (unsigned char)R;
        fill_color->g = (unsigned char)G;
        fill_color->b = (unsigned char)B;
        fill_color->a = RGBA_COLOR_OPAQUE;
    }

    Symb = S_read(symbol_name);
    if (!Symb)
        G_fatal_error(_("Reading symbol"));

    S_stroke(Symb, size, rotation, tolerance);
    D_symbol(Symb, easting, northing, line_color, fill_color);

    G_free(line_color);
    G_free(fill_color);
}
