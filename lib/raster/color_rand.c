#include <time.h>		/*  For time()  */
#include <stdio.h>		/*  For NULL */
#include <stdlib.h>		/*  For rand() and srand() */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define MAX_COLORS 1024
#define DEVIATION 128


/*!
 * \brief make random colors
 *
 * Generates random colors. Good as a first pass at a
 * color table for nominal data.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return
 */

void Rast_make_random_colors(struct Colors *colors, CELL min, CELL max)
{
    unsigned char red, grn, blu;
    int count;
    CELL n;

    Rast_init_colors(colors);
    if (min > max)
	G_fatal_error(_("Rast_make_random_colors: min (%d) > max (%d)"),
		      min, max);

    srand(time(NULL));

    count = MAX_COLORS - DEVIATION + rand() % DEVIATION;
    if (count > max - min + 1)
	count = max - min + 1;

    for (n = 1; n <= count; n++) {
	red = rand() & 0377;
	grn = rand() & 0377;
	blu = rand() & 0377;
	Rast_add_modular_c_color_rule(&n, red, grn, blu,
				      &n, red, grn, blu, colors);
    }
    Rast_set_c_color_range(min, max, colors);
}
