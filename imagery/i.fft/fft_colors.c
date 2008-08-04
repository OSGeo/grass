#include <grass/gis.h>
#include "globals.h"

int fft_colors(void)
{
    struct Colors colors;
    struct Range range;
    CELL min, max;

    /* make a real component color table */
    G_read_range(Cellmap_real, G_mapset(), &range);
    G_get_range_min_max(&range, &min, &max);
    G_make_wave_colors(&colors, min, max);
    G_write_colors(Cellmap_real, G_mapset(), &colors);
    G_free_colors(&colors);

    /* make a imag component color table */
    G_read_range(Cellmap_imag, G_mapset(), &range);
    G_get_range_min_max(&range, &min, &max);
    G_make_wave_colors(&colors, min, max);
    G_write_colors(Cellmap_imag, G_mapset(), &colors);

    return 0;
}
