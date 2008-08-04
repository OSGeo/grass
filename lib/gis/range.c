
/**********************************************************************
 *
 *  G_read_range (name, mapset, range)
 *      const char *name             name of map
 *      const char *mapset           mapset that map belongs to
 *      struct Range *range          struct to hold range info
 *
 *  Reads the data range information associated with map layer "map"
 *  in mapset "mapset" 
 *
 *   returns:    1  if successful
 *               2  range is empty
 *               3  map is fp: get range from quant rules
 *              -1  on fail
 *
 *  note:   a warning message is printed if the file is missing or incorrect
 *
 **********************************************************************
 *
 *  G_read_fp_range (name, mapset, range)
 *      const char *name               name of map
 *      const char *mapset             mapset that map belongs to
 *      struct FPRange *range          struct to hold range info
 *
 *  Reads the fp data range information associated with map layer "map"
 *  in mapset "mapset" . If map is integer, the integer range is read
 *  and the max and min values are casted to doubles and copied to FPRange
 *
 *   returns:    1  if successful
 *               2  range is empty
 *              -1  on fail
 *
 *  note:   a warning message is printed if the file is missing or incorrect
 *
 **********************************************************************
 *
 *  G_write_[fp_]range (name, range)
 *      const char *name                name of map
 *      struct [FP]Range *range         struct holding range info
 *
 *  Writes the range information associated with map layer "map"
 *
 *   returns:    0  if successful
 *              -1  on fail (or if the map is fp )
 *
 **********************************************************************
 *
 * G_init_[fp_]range (range)
 *      struct [FP]Range *range         struct for range info
 *
 * initializes range structure for call to G_update_[fp_]range()
 *
 **********************************************************************
 *
 * G_construct_default_range (range)
 *      struct Range *range         struct for range info
 *
 *  returns 1 and range is set to DEFAULT_CELL_MIN
 *  and DEFAULT_SET_MAX, otherwise returns -1
 *
 **********************************************************************
 *
 * G_update_[fp_]range (cat, range)
 *    DCELL cat                    cat to be factored into range
 *    struct [FP]Range *range      struct for range info
 **********************************************************************
 *
 * G_row_update_[fp_]range (rast, range, data_type)
 *    void *rast                   raster row to be factored into range
 *    struct [FP]Range *range      struct for range info
 *    RASTER_MAP_TYPE data_type;
 **********************************************************************
 *
 * G_get_[fp_]range_min_max (range, min, max)
 *    struct [FP]Range *range;
 *    [D]CELL *min, *max;
 **********************************************************************/

#include <unistd.h>
#include <rpc/types.h>		/* need this for sgi */
#include <rpc/xdr.h>
#include "G.h"
#include <grass/glocale.h>
#define DEFAULT_CELL_MIN 1
#define DEFAULT_CELL_MAX 255

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/* range functions for type "Range" */

/*-------------------------------------------------------------------------*/
int G__remove_fp_range(const char *name)
{
    G_remove_misc("cell_misc", "f_range", name);

    return 0;
}


/*!
 * \brief 
 *
 * Sets the integer range <em>r</em> to [1,255]
 *
 *  \param r
 *  \return int
 */

int G_construct_default_range(struct Range *range)
{
    G_update_range(DEFAULT_CELL_MIN, range);
    G_update_range(DEFAULT_CELL_MAX, range);

    return 0;
}


/*!
 * \brief 
 *
 * Read the floating point range file <tt>f_range</tt>. This file is
 * written in binary using XDR format. If there is no defined min/max in <em>r</em>, 
 * an empty <tt>f_range</tt>file is created.
 * An empty range file indicates that the min, max are undefined. This is a
 * valid case, and the result should be an initialized range struct with no
 * defined min/max.
 * If the range file is missing and the map is a floating-point map, this
 * function will create a default range by calling <tt>G_construct_default_range()</tt>.
 *
 *  \param r
 *  \param name
 *  \param mapset
 *  \return int
 */

int G_read_fp_range(const char *name, const char *mapset,
		    struct FPRange *drange)
{
    struct Range range;
    int fd;
    char buf[200], xdr_buf[100];
    DCELL dcell1, dcell2;
    XDR xdr_str;

    G_init_fp_range(drange);

    if (G_raster_map_type(name, mapset) == CELL_TYPE) {
	/* if map is integer
	   read integer range and convert it to double */

	if (G_read_range(name, mapset, &range) >= 0) {
	    /* if the integer range is empty */
	    if (range.first_time)
		return 2;

	    G_update_fp_range((DCELL) range.min, drange);
	    G_update_fp_range((DCELL) range.max, drange);
	    return 1;
	}
	return -1;
    }

    fd = -1;

    if (G_find_file2_misc("cell_misc", "f_range", name, mapset)) {
	fd = G_open_old_misc("cell_misc", "f_range", name, mapset);
	if (fd < 0)
	    goto error;

	if (read(fd, xdr_buf, 2 * XDR_DOUBLE_NBYTES) != 2 * XDR_DOUBLE_NBYTES)
	    return 2;

	xdrmem_create(&xdr_str, xdr_buf, (u_int) XDR_DOUBLE_NBYTES * 2,
		      XDR_DECODE);

	/* if the f_range file exists, but empty */
	if (!xdr_double(&xdr_str, &dcell1) || !xdr_double(&xdr_str, &dcell2))
	    goto error;

	G_update_fp_range(dcell1, drange);
	G_update_fp_range(dcell2, drange);
	close(fd);
	return 1;
    }

  error:
    if (fd > 0)
	close(fd);
    sprintf(buf, _("can't read f_range file for [%s in %s]"), name, mapset);
    G_warning(buf);
    return -1;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief read raster range
 *
 * This routine reads the range information for the
 * raster map <b>name</b> in <b>mapset</b> into the <b>range</b>
 * structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * reading the range file. Otherwise, 0 is returned.
 *
 * Old range file (those with 4 numbers) should
 * treat zeros in this file as NULL-values. New range files (those with just 2
 * numbers) should treat these numbers as real data (zeros are real data in this
 * case).
 * An empty range file indicates that the min, max are undefined. This is a
 * valid case, and the result should be an initialized range struct with no
 * defined min/max.
 * If the range file is missing and the map is a floating-point map, this
 * function will create a default range by calling <tt>G_construct_default_range()</tt>.
 *
 *  \param name
 *  \param mapset
 *  \param range
 *
 *  \return int
 */

int G_read_range(const char *name, const char *mapset, struct Range *range)
{
    FILE *fd;
    CELL x[4];
    char buf[200];
    int n, count;
    struct Quant quant;
    struct FPRange drange;

    G_init_range(range);
    fd = NULL;

    /* if map is not integer, read quant rules, and get limits */
    if (G_raster_map_type(name, mapset) != CELL_TYPE) {
	DCELL dmin, dmax;

	if (G_read_quant(name, mapset, &quant) < 0) {
	    sprintf(buf,
		    "G_read_range(): can't read quant rules for fp map %s@%s",
		    name, mapset);
	    G_warning(buf);
	    return -1;
	}
	if (G_quant_is_truncate(&quant) || G_quant_is_round(&quant)) {
	    if (G_read_fp_range(name, mapset, &drange) >= 0) {
		G_get_fp_range_min_max(&drange, &dmin, &dmax);
		if (G_quant_is_truncate(&quant)) {
		    x[0] = (CELL) dmin;
		    x[1] = (CELL) dmax;
		}
		else {		/* round */

		    if (dmin > 0)
			x[0] = (CELL) (dmin + .5);
		    else
			x[0] = (CELL) (dmin - .5);
		    if (dmax > 0)
			x[1] = (CELL) (dmax + .5);
		    else
			x[1] = (CELL) (dmax - .5);
		}
	    }
	    else
		return -1;
	}
	else
	    G_quant_get_limits(&quant, &dmin, &dmax, &x[0], &x[1]);

	G_update_range(x[0], range);
	G_update_range(x[1], range);
	return 3;
    }

    if (G_find_file2_misc("cell_misc", "range", name, mapset)) {
	fd = G_fopen_old_misc("cell_misc", "range", name, mapset);
	if (!fd)
	    goto error;

	/* if range file exists but empty */
	if (!fgets(buf, sizeof buf, fd))
	    return 2;

	x[0] = x[1] = x[2] = x[3] = 0;
	count = sscanf(buf, "%d%d%d%d", &x[0], &x[1], &x[2], &x[3]);

	/* if wrong format */
	if (count <= 0)
	    goto error;

	for (n = 0; n < count; n++) {
	    /* if count==4, the range file is old (4.1) and 0's in it
	       have to be ignored */
	    if (count < 4 || x[n])
		G_update_range((CELL) x[n], range);
	}
	fclose(fd);
	return 1;
    }

  error:
    if (fd)
	fclose(fd);
    sprintf(buf, _("can't read range file for [%s in %s]"), name, mapset);
    G_warning(buf);
    return -1;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief write raster range file
 *
 * This routine writes the range information for the raster map
 * <b>name</b> in the current mapset from the <b>range</b> structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * writing the range file. Otherwise, 0 is returned.
 *
 * This routine only writes 2 numbers (min,max) to the range
 * file, instead of the 4 (pmin,pmax,nmin,nmax) previously written.
 * If there is no defined min,max, an empty file is written.
 *
 *  \param name
 *  \param range
 *
 *  \return int
 */

int G_write_range(const char *name, const struct Range *range)
{
    FILE *fd;
    char buf[200];

    if (G_raster_map_type(name, G_mapset()) != CELL_TYPE) {
	sprintf(buf, "G_write_range(): the map is floating point!");
	goto error;
    }
    fd = G_fopen_new_misc("cell_misc", "range", name);
    if (!fd)
	goto error;

    if (range->first_time)
	/* if range hasn't been updated */
    {
	fclose(fd);
	return 0;
    }
    fprintf(fd, "%ld %ld\n", (long)range->min, (long)range->max);
    fclose(fd);
    return 0;

  error:
    G_remove_misc("cell_misc", "range", name);	/* remove the old file with this name */
    sprintf(buf, _("can't write range file for [%s in %s]"),
	    name, G_mapset());
    G_warning(buf);
    return -1;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Write the floating point range
 * file <tt>f_range</tt>. This file is written in binary using XDR format. If
 * there is no defined min/max in <em>r</em>, an empty <tt>f_range</tt>file is
 * created.
 *
 *  \param r
 *  \return int
 */

int G_write_fp_range(const char *name, const struct FPRange *range)
{
    int fd;
    char buf[200], xdr_buf[100];
    XDR xdr_str;

    sprintf(buf, "cell_misc/%s", name);
    fd = G_open_new(buf, "f_range");
    if (fd < 0)
	goto error;

    if (range->first_time)
	/* if range hasn't been updated, write empty file meaning Nulls */
    {
	close(fd);
	return 0;
    }

    xdrmem_create(&xdr_str, xdr_buf, (u_int) XDR_DOUBLE_NBYTES * 2,
		  XDR_ENCODE);

    if (!xdr_double(&xdr_str, (double *)&(range->min)))
	goto error;
    if (!xdr_double(&xdr_str, (double *)&(range->max)))
	goto error;

    write(fd, xdr_buf, XDR_DOUBLE_NBYTES * 2);
    close(fd);
    return 0;

  error:
    G_remove(buf, "f_range");	/* remove the old file with this name */
    sprintf(buf, _("can't write range file for [%s in %s]"),
	    name, G_mapset());
    G_warning(buf);
    return -1;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief update range structure
 *
 * Compares the <b>cat</b> value with the minimum and maximum
 * values in the <b>range</b> structure, modifying the range if <b>cat</b>
 * extends the range.
 *
 * NULL-values must be detected and ignored.
 *
 *  \param cat
 *  \param range
 *
 *  \return int
 */

int G_update_range(CELL cat, struct Range *range)
{
    if (!G_is_c_null_value(&cat)) {
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = cat;
	    range->max = cat;
	    return 0;
	}
	if (cat < range->min)
	    range->min = cat;
	if (cat > range->max)
	    range->max = cat;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/

int G_update_fp_range(DCELL val, struct FPRange *range)
{
    if (!G_is_d_null_value(&val)) {
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = val;
	    range->max = val;
	    return 0;
	}
	if (val < range->min)
	    range->min = val;
	if (val > range->max)
	    range->max = val;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief update range structure
 *
 * This routine updates the <b>range</b> data
 * just like <i>G_update_range</i>, but for <b>n</b> values from the
 * <b>cell</b> array.
 *
 *  \param cell
 *  \param n
 *  \param range
 *  \return int
 */

int G_row_update_range(const CELL * cell, int n, struct Range *range)
{
    G__row_update_range(cell, n, range, 0);

    return 0;
}

/*-------------------------------------------------------------------------*/

int G__row_update_range(const CELL * cell, int n,
			struct Range *range, int ignore_zeros)
{
    CELL cat;

    while (n-- > 0) {
	cat = *cell++;
	if (G_is_c_null_value(&cat) || (ignore_zeros && !cat))
	    continue;
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = cat;
	    range->max = cat;
	    continue;
	}
	if (cat < range->min)
	    range->min = cat;
	if (cat > range->max)
	    range->max = cat;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/

int G_row_update_fp_range(const void *rast, int n,
			  struct FPRange *range, RASTER_MAP_TYPE data_type)
{
    DCELL val = 0L;

    while (n-- > 0) {
	switch (data_type) {
	case CELL_TYPE:
	    val = (DCELL) * ((CELL *) rast);
	    break;
	case FCELL_TYPE:
	    val = (DCELL) * ((FCELL *) rast);
	    break;
	case DCELL_TYPE:
	    val = *((DCELL *) rast);
	    break;
	}

	if (G_is_null_value(rast, data_type)) {
	    rast = G_incr_void_ptr(rast, G_raster_size(data_type));
	    continue;
	}
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = val;
	    range->max = val;
	}
	else {
	    if (val < range->min)
		range->min = val;
	    if (val > range->max)
		range->max = val;
	}

	rast = G_incr_void_ptr(rast, G_raster_size(data_type));
    }

    return 0;
}

/*-------------------------------------------------------------------------*/

/*!
 * \brief initialize range structure
 *
 * Initializes the <b>range</b> structure for updates by
 * <i>G_update_range</i> and <i>G_row_update_range.</i>
 *
 * Must set a flag in the range structure that indicates that
 * no min/max have been defined - probably a <tt>"first"</tt> boolean flag.
 *
 *  \param range
 *  \return int
 */

int G_init_range(struct Range *range)
{
    G_set_c_null_value(&(range->min), 1);
    G_set_c_null_value(&(range->max), 1);
    range->first_time = 1;

    return 0;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief get range min and max
 *
 * The <b>min</b>inum and <b>max</b>imum CELL
 * values are extracted from the <b>range</b> structure.
 *
 * If the range structure has no defined min/max
 * (first!=0) there will not be a valid range. In this case the min and
 * max returned must be the NULL-value.
 *
 *  \param range
 *  \param min
 *  \param max
 *  \return int
 */


int G_get_range_min_max(const struct Range *range, CELL * min, CELL * max)
{
    if (range->first_time) {
	G_set_c_null_value(min, 1);
	G_set_c_null_value(max, 1);
    }
    else {
	if (G_is_c_null_value(&(range->min)))
	    G_set_c_null_value(min, 1);
	else
	    *min = range->min;

	if (G_is_c_null_value(&(range->max)))
	    G_set_c_null_value(max, 1);
	else
	    *max = range->max;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Must set a flag in the range
 * structure that indicates that no min/max have been defined - probably a
 * <tt>"first"</tt> boolean flag.
 *
 *  \param r
 *  \return int
 */

int G_init_fp_range(struct FPRange *range)
{
    G_set_d_null_value(&(range->min), 1);
    G_set_d_null_value(&(range->max), 1);
    range->first_time = 1;

    return 0;
}

/*-------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Extract the min/max from the range structure <em>r</em>.
 * If the range structure has no defined min/max (first!=0) there will not be a
 * valid range. In this case the min and max returned must be the NULL-value.
 *
 *  \param r
 *  \param min
 *  \param max
 *  \return int
 */

int G_get_fp_range_min_max(const struct FPRange *range,
			   DCELL * min, DCELL * max)
{
    if (range->first_time) {
	G_set_d_null_value(min, 1);
	G_set_d_null_value(max, 1);
    }
    else {
	if (G_is_d_null_value(&(range->min)))
	    G_set_d_null_value(min, 1);
	else
	    *min = range->min;

	if (G_is_d_null_value(&(range->max)))
	    G_set_d_null_value(max, 1);
	else
	    *max = range->max;
    }

    return 0;
}
