/*!
   \file lib/raster/color_rule.c

   \brief Raster Library - Color rules.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

#define LIMIT(x)      \
    if (x < 0)        \
        x = 0;        \
    else if (x > 255) \
        x = 255;

static void add_color_rule(const void *, int, int, int, const void *, int, int,
                           int, struct _Color_Info_ *, int, DCELL *, DCELL *,
                           RASTER_MAP_TYPE);

/*!
   \brief Adds the floating-point color rule (DCELL version)

   See Rast_add_color_rule() for details.

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure
 */
void Rast_add_d_color_rule(const DCELL *val1, int r1, int g1, int b1,
                           const DCELL *val2, int r2, int g2, int b2,
                           struct Colors *colors)
{
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->fixed,
                   colors->version, &colors->cmin, &colors->cmax, DCELL_TYPE);
}

/*!
   \brief Adds the floating-point color rule (FCELL version)

   See Rast_add_color_rule() for details.

   \param cat1 cell value
   \param r1,g1,b1 color value
   \param cat2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure
 */
void Rast_add_f_color_rule(const FCELL *cat1, int r1, int g1, int b1,
                           const FCELL *cat2, int r2, int g2, int b2,
                           struct Colors *colors)
{
    add_color_rule(cat1, r1, g1, b1, cat2, r2, g2, b2, &colors->fixed,
                   colors->version, &colors->cmin, &colors->cmax, FCELL_TYPE);
}

/*!
   \brief Adds the integer color rule (CELL version)

   See Rast_add_color_rule() for details.

   \param cat1 cell value
   \param r1,g1,b1 color value
   \param cat2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure
 */
void Rast_add_c_color_rule(const CELL *cat1, int r1, int g1, int b1,
                           const CELL *cat2, int r2, int g2, int b2,
                           struct Colors *colors)
{
    add_color_rule(cat1, r1, g1, b1, cat2, r2, g2, b2, &colors->fixed,
                   colors->version, &colors->cmin, &colors->cmax, CELL_TYPE);
}

/*!
   \brief Adds the color rule

   Adds the floating-point rule that the range [<em>v1,v2</em>] gets a
   linear ramp of colors from [<em>r1,g1,b1</em>] to
   [<em>r2,g2,b2</em>].
   If either <em>v1</em> or <em>v2</em> is the NULL-value, this call is
   converted ino <tt>Rast_set_null_value_color (r1, g1, b1, colors)</tt>

   - If <em>map_type</em> is CELL_TYPE, calls Rast_add_c_color_rule()
   - If <em>map_type</em> is FCELL_TYPE, calls Rast_add_f_color_rule()
   - If <em>map_type</em> is DCELL_TYPE, calls Rast_add_d_color_rule()

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure
   \param data_type raster data type (CELL, FCELL, DCELL)
 */
void Rast_add_color_rule(const void *val1, int r1, int g1, int b1,
                         const void *val2, int r2, int g2, int b2,
                         struct Colors *colors, RASTER_MAP_TYPE data_type)
{
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->fixed,
                   colors->version, &colors->cmin, &colors->cmax, data_type);
}

/*!
   \brief Add modular floating-point color rule (DCELL version)

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure

   \return -1 on failure
   \return 1 on success
 */
int Rast_add_modular_d_color_rule(const DCELL *val1, int r1, int g1, int b1,
                                  const DCELL *val2, int r2, int g2, int b2,
                                  struct Colors *colors)
{
    DCELL min, max;

    if (colors->version < 0)
        return -1; /* can't use this on 3.0 colors */
    min = colors->cmin;
    max = colors->cmax;
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->modular, 0,
                   &colors->cmin, &colors->cmax, DCELL_TYPE);
    colors->cmin = min; /* don't reset these */
    colors->cmax = max;

    return 1;
}

/*!
   \brief Add modular floating-point color rule (FCELL version)

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure

   \return -1 on failure
   \return 1 on success
 */
int Rast_add_modular_f_color_rule(const FCELL *val1, int r1, int g1, int b1,
                                  const FCELL *val2, int r2, int g2, int b2,
                                  struct Colors *colors)
{
    DCELL min, max;

    if (colors->version < 0)
        return -1; /* can;t use this on 3.0 colors */
    min = colors->cmin;
    max = colors->cmax;
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->modular, 0,
                   &colors->cmin, &colors->cmax, FCELL_TYPE);
    colors->cmin = min; /* don't reset these */
    colors->cmax = max;

    return 1;
}

/*!
   \brief Add modular integer color rule (CELL version)

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure

   \return -1 on failure
   \return 1 on success
 */
int Rast_add_modular_c_color_rule(const CELL *val1, int r1, int g1, int b1,
                                  const CELL *val2, int r2, int g2, int b2,
                                  struct Colors *colors)
{
    CELL min, max;

    if (colors->version < 0)
        return -1; /* can;t use this on 3.0 colors */
    min = colors->cmin;
    max = colors->cmax;
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->modular, 0,
                   &colors->cmin, &colors->cmax, CELL_TYPE);
    colors->cmin = min; /* don't reset these */
    colors->cmax = max;

    return 1;
}

/*!
   \brief Add modular color rule

   \todo Question: shouldn't this function call
   G_add_modular_<data_type>_raster_color_rule() instead?

   \param val1 cell value
   \param r1,g1,b1 color value
   \param val2 cell value
   \param r2,g2,b2 color value
   \param[in,out] colors pointer to color table structure
   \param data_type raster data type

   \return -1 on failure
   \return 1 on success
 */
int Rast_add_modular_color_rule(const void *val1, int r1, int g1, int b1,
                                const void *val2, int r2, int g2, int b2,
                                struct Colors *colors,
                                RASTER_MAP_TYPE data_type)
{
    CELL min, max;

    if (colors->version < 0)
        return -1; /* can't use this on 3.0 colors */
    min = colors->cmin;
    max = colors->cmax;
    add_color_rule(val1, r1, g1, b1, val2, r2, g2, b2, &colors->modular, 0,
                   &colors->cmin, &colors->cmax, data_type);
    colors->cmin = min; /* don't reset these */
    colors->cmax = max;

    return 1;
}

static void add_color_rule(const void *pt1, int r1, int g1, int b1,
                           const void *pt2, int r2, int g2, int b2,
                           struct _Color_Info_ *cp, int version, DCELL *cmin,
                           DCELL *cmax, RASTER_MAP_TYPE data_type)
{
    struct _Color_Rule_ *rule, *next;
    unsigned char red, grn, blu;
    DCELL min, max, val1, val2;
    CELL cat;

    val1 = Rast_get_d_value(pt1, data_type);
    val2 = Rast_get_d_value(pt2, data_type);
    /* allocate a low:high rule */
    rule = (struct _Color_Rule_ *)G_malloc(sizeof(*rule));
    rule->next = rule->prev = NULL;

    /* make sure colors are in the range [0,255] */
    LIMIT(r1);
    LIMIT(g1);
    LIMIT(b1);
    LIMIT(r2);
    LIMIT(g2);
    LIMIT(b2);

    /* val1==val2, use average color */
    /* otherwise make sure low < high */
    if (val1 == val2) {
        rule->low.value = rule->high.value = val1;
        rule->low.red = rule->high.red = (r1 + r2) / 2;
        rule->low.grn = rule->high.grn = (g1 + g2) / 2;
        rule->low.blu = rule->high.blu = (b1 + b2) / 2;
    }
    else if (val1 < val2) {
        rule->low.value = val1;
        rule->low.red = r1;
        rule->low.grn = g1;
        rule->low.blu = b1;

        rule->high.value = val2;
        rule->high.red = r2;
        rule->high.grn = g2;
        rule->high.blu = b2;
    }
    else {
        rule->low.value = val2;
        rule->low.red = r2;
        rule->low.grn = g2;
        rule->low.blu = b2;

        rule->high.value = val1;
        rule->high.red = r1;
        rule->high.grn = g1;
        rule->high.blu = b1;
    }

    /* keep track of the overall min and max, excluding null */
    if (Rast_is_d_null_value(&(rule->low.value))) {
        G_free(rule);
        return;
    }
    if (Rast_is_d_null_value(&(rule->high.value))) {
        G_free(rule);
        return;
    }
    min = rule->low.value;
    max = rule->high.value;
    if (min <= max) {
        if (cp->min > cp->max) {
            cp->min = min;
            cp->max = max;
        }
        else {
            if (cp->min > min)
                cp->min = min;
            if (cp->max < max)
                cp->max = max;
        }
    }
    if (*cmin > *cmax) {
        *cmin = cp->min;
        *cmax = cp->max;
    }
    else {
        if (*cmin > cp->min)
            *cmin = cp->min;
        if (*cmax < cp->max)
            *cmax = cp->max;
    }

    /* If version is old style (i.e., pre 4.0),
     *     interpolate this rule from min to max
     *     and insert each cat into the lookup table.
     *     Then free the rule.
     * Otherwise, free the lookup table, if active.
     *     G_organize_colors() will regenerate it
     *     Link this rule into the list of rules
     */

    if (version < 0) {
        for (cat = (CELL)min; cat <= (CELL)max; cat++) {
            Rast__interpolate_color_rule((DCELL)cat, &red, &grn, &blu, rule);
            Rast__insert_color_into_lookup(cat, (int)red, (int)grn, (int)blu,
                                           cp);
        }
        G_free(rule);
    }
    else {
        if (cp->rules)
            cp->rules->prev = rule;
        rule->next = cp->rules;
        cp->rules = rule;

        /* prune the rules:
         * remove all rules that are contained by this rule
         */
        min = rule->low.value;  /* mod 4.1 */
        max = rule->high.value; /* mod 4.1 */
        cp->n_rules++;
        for (rule = rule->next; rule; rule = next) {
            next = rule->next; /* has to be done here, not in for stmt */
            if (min <= rule->low.value && max >= rule->high.value) {
                if ((rule->prev->next = next)) /* remove from the list */
                    next->prev = rule->prev;
                G_free(rule);
                cp->n_rules--;
            }
        }

        /* free lookup array, if allocated */
        Rast__color_free_lookup(cp);
        Rast__color_free_fp_lookup(cp);
    }
}
