
/**********************************************************************
 *
 * G_init_colors (colors)
 *      struct Colors *colors         structure to hold color info
 *
 * Initializes the color structure for subsequent calls to G_add_color_rule()
 *********************************************************************/

#include <grass/gis.h>


/*!
 * \brief initialize color structure
 *
 * The <b>colors</b> structure is initialized for subsequent calls
 * to <i>G_add_color_rule</i> and<i>G_set_color.</i>
 *
 *  \param colors
 *  \return int
 */

int G_init_colors(struct Colors *colors)
{
    colors->version = 0;
    colors->null_set = 0;
    colors->undef_set = 0;
    colors->shift = 0.0;
    colors->invert = 0;
    colors->cmin = 0;
    colors->is_float = 0;
    colors->cmax = -1;
    colors->fixed.min = 0;
    colors->fixed.max = -1;
    colors->fixed.rules = NULL;
    colors->fixed.n_rules = 0;
    colors->fixed.lookup.active = 0;
    colors->fixed.fp_lookup.active = 0;
    colors->fixed.fp_lookup.nalloc = 0;
    colors->modular.min = 0;
    colors->modular.max = -1;
    colors->modular.rules = NULL;
    colors->modular.n_rules = 0;
    colors->modular.lookup.active = 0;
    colors->modular.fp_lookup.active = 0;
    colors->modular.fp_lookup.nalloc = 0;

    return 0;
}
