#include <grass/gis.h>
#include "G.h"


/*!
 * \brief 
 *
 * Reads a row of raster data and converts it to red,
 * green and blue components according to the <em>colors</em> parameter.
 * This provides a convenient way to treat a raster layer as a color
 * image without having to explictly cater for each of <tt>CELL</tt>, <tt>FCELL</tt> and <tt>DCELL</tt> types
 *
 *  \param fd
 *  \param row
 *  \param colors
 *  \param red
 *  \param grn
 *  \param blu
 *  \param nul
 *  \return int
 */

int
G_get_raster_row_colors(int fd, int row, struct Colors *colors,
			unsigned char *red, unsigned char *grn,
			unsigned char *blu, unsigned char *nul)
{
    int cols = G_window_cols();
    int type = G_get_raster_map_type(fd);
    int size = G_raster_size(type);
    void *array;
    unsigned char *set;
    void *p;
    int i;

    array = G__alloca(cols * size);

    if (G_get_raster_row(fd, array, row, type) < 0) {
	G__freea(array);
	return -1;
    }

    if (nul)
	for (i = 0, p = array; i < cols; i++, p = G_incr_void_ptr(p, size))
	    nul[i] = G_is_null_value(p, type);

    set = G__alloca(cols);

    G_lookup_raster_colors(array, red, grn, blu, set, cols, colors, type);

    G__freea(array);
    G__freea(set);

    return 0;
}
