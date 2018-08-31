/*!
 * \file lib/raster/color_free.c
 *
 * \brief Raster Library - Free Colors structure
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Free color structure memory
 *
 * The dynamically allocated memory associated with the <i>colors</i>
 * structure is freed.
 *
 * <b>Note:</b> This routine may be used after Rast_read_colors() as well
 * as after Rast_init_colors().
 *
 * \param colors pointer to Colors structure
 */
void Rast_free_colors(struct Colors *colors)
{
    Rast__color_reset(colors);
    Rast_init_colors(colors);
}

/*!
   \brief Free color rules structure

   Note: Only for internal use.

   \param cp pointer to _Color_Info structure
 */
void Rast__color_free_rules(struct _Color_Info_ *cp)
{
    struct _Color_Rule_ *rule, *next;

    for (rule = cp->rules; rule; rule = next) {
	next = rule->next;
	G_free(rule);
    }
    cp->rules = NULL;
}

/*!
   \brief Free color rules structure

   Note: Only for internal use.

   \param cp pointer to _Color_Info structure
 */
void Rast__color_free_lookup(struct _Color_Info_ *cp)
{
    if (cp->lookup.active) {
	G_free(cp->lookup.red);
	G_free(cp->lookup.blu);
	G_free(cp->lookup.grn);
	G_free(cp->lookup.set);
	cp->lookup.active = 0;
    }
}

/*!
   \brief Free color rules structure

   Note: Only for internal use.

   \param cp pointer to _Color_Info structure
 */
void Rast__color_free_fp_lookup(struct _Color_Info_ *cp)
{
    if (cp->fp_lookup.active) {
	G_free(cp->fp_lookup.vals);
	G_free(cp->fp_lookup.rules);
	cp->fp_lookup.active = 0;
	cp->fp_lookup.nalloc = 0;
    }
}

/*!
   \brief Reset colors structure

   Note: Only for internal use.

   This routine should NOT init the colors.

   \param colors pointer to Colors structure
 */
void Rast__color_reset(struct Colors *colors)
{
    Rast__color_free_lookup(&colors->fixed);
    Rast__color_free_lookup(&colors->modular);
    Rast__color_free_rules(&colors->fixed);
    Rast__color_free_rules(&colors->modular);
    colors->version = 0;
}
