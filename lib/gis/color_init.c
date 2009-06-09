/*!
 * \file gis/color_init.c
 *
 * \brief GIS Library - Initialize Colors structure
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Initialize color structure
 *
 * The <i>colors</i> structure is initialized for subsequent calls
 * to G_add_color_rule() and G_set_color().
 *
 * \param colors pointer to Colors structure
 */
void G_init_colors(struct Colors *colors)
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
    colors->organizing = 0;
}
