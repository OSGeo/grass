#include <math.h>
#include <grass/gis.h>

int G_set_color_range(CELL min, CELL max, struct Colors *colors)
{
    if (min < max) {
	colors->cmin = (DCELL) min;
	colors->cmax = (DCELL) max;
    }
    else {
	colors->cmin = (DCELL) max;
	colors->cmax = (DCELL) min;
    }

    return 0;
}

int G_set_d_color_range(DCELL min, DCELL max, struct Colors *colors)
{
    if (min < max) {
	colors->cmin = min;
	colors->cmax = max;
    }
    else {
	colors->cmin = max;
	colors->cmax = min;
    }

    return 0;
}

/* returns min and max category in the range or huge numbers
   if the co,lor table is defined on floating cell values and
   not on categories */


/*!
 * \brief
 *
 *  \param G_get_color_range
 *  \return int
 */

int G_get_color_range(CELL * min, CELL * max, const struct Colors *colors)
{
    if (!colors->is_float) {
	*min = (CELL) floor(colors->cmin);
	*max = (CELL) ceil(colors->cmax);
    }
    else {
	*min = -255 * 255 * 255;
	*max = 255 * 255 * 255;
    }

    return 0;
}

/* returns min and max values in the range */
int G_get_d_color_range(DCELL * min, DCELL * max, const struct Colors *colors)
{
    *min = colors->cmin;
    *max = colors->cmax;

    return 0;
}
