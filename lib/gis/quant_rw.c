#include <grass/gis.h>
#include <grass/glocale.h>
#include <string.h>

/*********************************************************************
*
*   G_quantize_fp_map(name, mapset, min, max)
*   char *name, *mapset;   name of the map
*   CELL min, max;         resulting int range
*
*   Writes necessary quant rules for map <name> so that
*   a floating range of <name> is mapped into integer range (min, max)
*
**********************************************************************
* 
*   G_quantize_fp_map_range(name, mapset, d_min, d_max, min, max)
*   char *name, *mapset;   name of the map
*   CELL min, max;         resulting int range
*   DCELL d_min, d_max;    floating point range
*
*   Make a rule for map <name> that maps floating range (d_min, d_max)
*   into integer range (min, max)
*   This function is useful when the quant rule doesn't depend of the
*   range of produced float data, for example the slope map whould
*   want to have a quant rule: 0.0, 90.0 -> 0 , 90
*   no matter what the min and max slope of this map is.
*
**********************************************************************
* 
*   G_write_quant(name, mapset, quant)
*        char *name, *mapset;
*        struct Quant *quant;
*   writes the quant rule table for the map <name>
*
**********************************************************************
* 
*   G_read_quant(name, mapset, quant)
*        char *name, *mapset;
*
*   reads the quant table for name@mapset
*
**********************************************************************
*
*   G_truncate_fp_map(name, mapset)
*        char *name, *mapset;
*        struct Quant *quant;
*
*   writes the quant rules which indicate that all floating numbers
*   should be truncated instead of applying any quant rules from
*   floats to integers
*
**********************************************************************
*
*   G_round_fp_map(name, mapset)
*        char *name, *mapset;
*        struct Quant *quant;
*
*   writes the quant rules which indicate that all floating numbers
*   should be rounded instead of applying any quant rules from
*   floats to integers
*
**********************************************************************/

int G_truncate_fp_map(const char *name, const char *mapset)
{
    char buf[300];
    struct Quant quant;

    G_quant_init(&quant);
    G_quant_truncate(&quant);
    /* quantize the map */
    if (G_write_quant(name, mapset, &quant) < 0) {
	sprintf(buf, "G_truncate_fp_map: can't write quant rules for map %s",
		name);
	G_warning(buf);
	return -1;
    }
    return 1;
}

int G_round_fp_map(const char *name, const char *mapset)
{
    char buf[300];
    struct Quant quant;

    G_quant_init(&quant);
    G_quant_round(&quant);
    /* round the map */
    if (G_write_quant(name, mapset, &quant) < 0) {
	sprintf(buf, "G_truncate_fp_map: can't write quant rules for map %s",
		name);
	G_warning(buf);
	return -1;
    }
    return 1;
}


/*!
 * \brief 
 *
 * Writes
 * the <tt>f_quant</tt> file for the raster map <em>name</em> with one rule. The rule
 * is generated using the floating-point range in <tt>f_range</tt> producing the
 * integer range [<em>cmin,cmax</em>].
 *
 *  \param name
 *  \param cmin
 *  \param cmax
 *  \return int
 */

int G_quantize_fp_map(const char *name, const char *mapset,
		      CELL min, CELL max)
{
    char buf[300];
    DCELL d_min, d_max;
    struct FPRange fp_range;

    if (G_read_fp_range(name, mapset, &fp_range) < 0) {
	sprintf(buf, "G_quantize_fp_map: can't read fp range for map %s",
		name);
	G_warning(buf);
	return -1;
    }
    G_get_fp_range_min_max(&fp_range, &d_min, &d_max);
    if (G_is_d_null_value(&d_min) || G_is_d_null_value(&d_max)) {
	sprintf(buf, "G_quantize_fp_map: raster map %s is empty", name);
	G_warning(buf);
	return -1;
    }
    return G_quantize_fp_map_range(name, mapset, d_min, d_max, min, max);
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief 
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
 *  \param name
 *  \param dmin
 *  \param dmax
 *  \param cmin
 *  \param cmax
 *  \return int
 */

int G_quantize_fp_map_range(const char *name, const char *mapset,
			    DCELL d_min, DCELL d_max, CELL min, CELL max)
{
    char buf[300];
    struct Quant quant;

    G_quant_init(&quant);
    G_quant_add_rule(&quant, d_min, d_max, min, max);
    /* quantize the map */
    if (G_write_quant(name, mapset, &quant) < 0) {
	sprintf(buf,
		"G_quantize_fp_map_range: can't write quant rules for map %s",
		name);
	G_warning(buf);
	return -1;
    }
    return 1;
}

/*-------------------------------------------------------------------------*/



/*!
 * \brief 
 *
 * Writes the <tt>f_quant</tt> file for the raster map <em>name</em> from <em>q</em>.
 * if mapset==G_mapset() i.e. the map is in current mapset, then the original
 * quant file in cell_misc/map/f_quant is written. Otherwise <em>q</em> is
 * written into quant2/mapset/name (much like colr2 element). This results in
 * map@mapset being read using quant rules stored in <em>q</em> from
 * G_mapset().  See G_read_quant() for detailes.
 *
 *  \param name
 *  \param mapset
 *  \param q
 *  \return int
 */

int G_write_quant(const char *name, const char *mapset,
		  const struct Quant *quant)
{
    CELL cell_min, cell_max;
    DCELL d_min, d_max;
    char buf[300];

    if (G_raster_map_type(name, mapset) == CELL_TYPE) {
	sprintf(buf, _("Cannot write quant rules: map %s is integer"), name);
	G_warning(buf);
	return -1;
    }

    G_quant_get_limits(quant, &d_min, &d_max, &cell_min, &cell_max);

    /* first actually write the rules */
    if (G__quant_export(name, mapset, quant) < 0) {
	sprintf(buf, _("Cannot write quant rules for map %s"), name);
	G_warning(buf);
	return -1;
    }

    return 1;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * reads quantization rules for <tt>"name"</tt> in <tt>"mapset"</tt> and stores them
 * in the quantization structure <tt>"quant"</tt>. If the map is in another
 * mapset, first checks for quant2 table for this map in current mapset.
 * Return codes:
 * -2 if raster map is of type integer
 * -1 if (! G__name_is_fully_qualified ())
 * 0 if quantization file does not exist, or the file is empty or has wrong
 * format.
 * 1 if non-empty quantization file exists.
 *
 *  \param name
 *  \param mapset
 *  \param q
 *  \return int
 */

int G_read_quant(const char *name, const char *mapset, struct Quant *quant)
{
    G_quant_init(quant);
    return G__quant_import(name, mapset, quant);
}
