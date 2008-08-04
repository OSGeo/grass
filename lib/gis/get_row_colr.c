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
    static void *array;
    static int array_size;
    static unsigned char *set;
    static int set_size;

    int cols = G__.window.cols;
    int type = G__.fileinfo[fd].map_type;
    int size = G_raster_size(type);
    void *p;
    int i;

    if (array_size < cols * size) {
	array_size = cols * size;
	array = (DCELL *) G_realloc(array, array_size);
    }

    if (set_size < cols) {
	set_size = cols;
	set = G_realloc(set, set_size);
    }

    if (G_get_raster_row(fd, array, row, type) < 0)
	return -1;

    if (nul)
	for (i = 0, p = array; i < cols; i++, p = G_incr_void_ptr(p, size))
	    nul[i] = G_is_null_value(p, type);

    G_lookup_raster_colors(array, red, grn, blu, set, cols, colors, type);

    return 0;
}
