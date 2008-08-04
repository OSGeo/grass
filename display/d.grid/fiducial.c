#include <grass/gis.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "local_proto.h"


void plot_cross(double easting, double northing, int color, double rotation)
{
    plot_symbol(easting, northing, color, rotation, "basic/cross1");
}


void plot_fiducial(double easting, double northing, int color,
		   double rotation)
{
    plot_symbol(easting, northing, color, rotation + 45.0, "extra/fiducial");
}


void plot_symbol(double easting, double northing, int color, double rotation,
		 char *symbol_name)
{
    SYMBOL *Symb;
    RGBA_Color *line_color, *fill_color;
    int R, G, B;
    int x0, y0;
    int size = 16;
    int tolerance = 0;

    x0 = (int)(D_u_to_d_col(easting) + 0.5);
    y0 = (int)(D_u_to_d_row(northing) + 0.5);

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


    Symb = S_read(symbol_name);
    if (!Symb)
	G_fatal_error(_("Reading symbol"));

    S_stroke(Symb, size, rotation, tolerance);
    D_symbol(Symb, x0, y0, line_color, fill_color);

    G_free(line_color);
    G_free(fill_color);
}
