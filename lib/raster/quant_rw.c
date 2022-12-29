/*!
 * \file lib/raster/quant_rw.c
 * 
 * \brief Raster Library - Quantization rules (read/write).
 *
 * (C) 1999-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author USACERL and many others
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/*!
   \brief Writes the quant rules.

   Writes the quant rules which indicate that all floating numbers
   should be truncated instead of applying any quant rules from
   floats to integers.

   \param name map name
   \param mapset mapset name
 */
void Rast_truncate_fp_map(const char *name, const char *mapset)
{
    struct Quant quant;

    Rast_quant_init(&quant);
    Rast_quant_truncate(&quant);
    /* quantize the map */
    Rast_write_quant(name, mapset, &quant);
}

/*!
   \brief Writes the quant rules.

   Writes the quant rules which indicate that all floating numbers
   should be rounded instead of applying any quant rules from
   floats to integers.

   \param name map name
   \param mapset mapset name
 */
void Rast_round_fp_map(const char *name, const char *mapset)
{
    struct Quant quant;

    Rast_quant_init(&quant);
    Rast_quant_round(&quant);
    /* round the map */
    Rast_write_quant(name, mapset, &quant);
}

/*!
 * \brief Write quant rules (f_quant) for floating-point raster map.
 *
 * Writes the <tt>f_quant</tt> file for the raster map <em>name</em>
 * with one rule. The rule is generated using the floating-point range
 * in <tt>f_range</tt> producing the integer range
 * [<em>cmin,cmax</em>].
 *
 * Make a rule for map \<name\> that maps floating range (d_min, d_max)
 * into integer range (min, max)
 * This function is useful when the quant rule doesn't depend of the
 * range of produced float data, for example the slope map whould
 * want to have a quant rule: 0.0, 90.0 -> 0 , 90
 * no matter what the min and max slope of this map is.

 * \param name map name
 * \param mapset mapset name
 * \param cmin minimum value
 * \param cmax maximum value
 */
void Rast_quantize_fp_map(const char *name, const char *mapset,
			  CELL min, CELL max)
{
    DCELL d_min, d_max;
    struct FPRange fp_range;

    if (Rast_read_fp_range(name, mapset, &fp_range) < 0)
	G_fatal_error(_("Unable to read fp range for raster map <%s>"),
		      G_fully_qualified_name(name, mapset));
    Rast_get_fp_range_min_max(&fp_range, &d_min, &d_max);
    if (Rast_is_d_null_value(&d_min) || Rast_is_d_null_value(&d_max))
	G_fatal_error(_("Raster map <%s> is empty"),
		      G_fully_qualified_name(name, mapset));
    Rast_quantize_fp_map_range(name, mapset, d_min, d_max, min, max);
}

/*!
 * \brief Write quant rules (f_quant) for floating-point raster map.
 *
 * Writes the <tt>f_quant</tt> file for the raster map
 * <em>name</em> with one rule. The rule is generated using the floating-point
 * range [<em>dmin,dmax</em>] and the integer range
 * [<em>min,max</em>].
 * This routine differs from the one above in that the application controls the
 * floating-point range. For example, r.slope.aspect will use this routine to
 * quantize the slope map from [0.0, 90.0] to [0,
 * 90] even if the range of slopes is not 0-90. The aspect map would be
 * quantized from [0.0, 360.0] to [0, 360].
 *
 * Make a rule for map \<name\> that maps floating range (d_min, d_max)
 * into integer range (min, max)
 * This function is useful when the quant rule doesn't depend of the
 * range of produced float data, for example the slope map whould
 * want to have a quant rule: 0.0, 90.0 -> 0 , 90
 * no matter what the min and max slope of this map is.
 *
 * \param name map name
 * \param mapset mapset name
 * \param d_min minimum fp value
 * \param d_max maximum fp value
 * \param min minimum value
 * \param max maximum value
 */
void Rast_quantize_fp_map_range(const char *name, const char *mapset,
				DCELL d_min, DCELL d_max, CELL min, CELL max)
{
    struct Quant quant;

    Rast_quant_init(&quant);
    Rast_quant_add_rule(&quant, d_min, d_max, min, max);
    /* quantize the map */
    Rast_write_quant(name, mapset, &quant);
}

/*!
 * \brief Writes the quant rule table for the raster map
 *
 * Writes the <tt>f_quant</tt> file for the raster map <em>name</em>
 * from <em>q</em>.  if mapset==G_mapset() i.e. the map is in current
 * mapset, then the original quant file in cell_misc/map/f_quant is
 * written. Otherwise <em>q</em> is written into quant2/mapset/name
 * (much like colr2 element). This results in map@mapset being read
 * using quant rules stored in <em>q</em> from G_mapset(). See
 * Rast_read_quant() for detailes.
 *
 * \param name map name
 * \param mapset mapset name
 * \param quant pointer to Quant structure which hold quant rules info
 */
void Rast_write_quant(const char *name, const char *mapset,
		      const struct Quant *quant)
{
    CELL cell_min, cell_max;
    DCELL d_min, d_max;

    if (Rast_map_type(name, mapset) == CELL_TYPE) {
	G_warning(_("Unable to write quant rules: raster map <%s> is integer"),
		  name);
	return;
    }

    Rast_quant_get_limits(quant, &d_min, &d_max, &cell_min, &cell_max);

    /* first actually write the rules */
    if (Rast__quant_export(name, mapset, quant) < 0)
	G_fatal_error(_("Unable to write quant rules for raster map <%s>"), name);
}

/*!
 * \brief 
 *
 * Reads quantization rules for <i>name</i> in <i>mapset</i> and
 * stores them in the quantization structure. If the map is in another
 * mapset, first checks for quant2 table for this map in current
 * mapset. 
 *  \param name
 *  \param mapset
 *  \param q
 *
 * \return -2 if raster map is of type integer
 * \return -1 if (!G_name_is_fully_qualified())
 * \return 0 if quantization file does not exist, or the file is empty or has wrong format
 * \return 1 if non-empty quantization file exists
 *
 */
int Rast_read_quant(const char *name, const char *mapset, struct Quant *quant)
{
    Rast_quant_init(quant);
    return Rast__quant_import(name, mapset, quant);
}
