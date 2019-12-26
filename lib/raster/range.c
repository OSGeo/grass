/*!
 * \file lib/raster/range.c
 *
 * \brief Raster Library - Raster range file management
 *
 * (C) 2001-2009 GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <unistd.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

#define DEFAULT_CELL_MIN 1
#define DEFAULT_CELL_MAX 255

static void init_rstats(struct R_stats *);

/*!
   \brief Remove floating-point range

   Note: For internal use only.

   \param name map name
 */
void Rast__remove_fp_range(const char *name)
{
    G_remove_misc("cell_misc", "f_range", name);
}

/*!
 * \brief Construct default range
 *
 * Sets the integer range to [1,255]
 *
 * \param[out] r pointer to Range structure which holds range info
 */
void Rast_construct_default_range(struct Range *range)
{
    Rast_update_range(DEFAULT_CELL_MIN, range);
    Rast_update_range(DEFAULT_CELL_MAX, range);
}

/*!
 * \brief Read floating-point range
 *
 * Read the floating point range file <i>drange</i>. This file is
 * written in binary using XDR format.
 *
 * An empty range file indicates that the min, max are undefined. This
 * is a valid case, and the result should be an initialized range
 * struct with no defined min/max.  If the range file is missing and
 * the map is a floating-point map, this function will create a
 * default range by calling G_construct_default_range().
 *
 * \param name map name
 * \param mapset mapset name
 * \param drange pointer to FPRange structure which holds fp range
 *
 * \return 1 on success
 * \return 2 range is empty
 * \return -1 on error
 */
int Rast_read_fp_range(const char *name, const char *mapset,
		       struct FPRange *drange)
{
    struct Range range;
    int fd;
    char xdr_buf[2][XDR_DOUBLE_NBYTES];
    DCELL dcell1, dcell2;

    Rast_init();
    Rast_init_fp_range(drange);

    if (Rast_map_type(name, mapset) == CELL_TYPE) {
	/* if map is integer
	   read integer range and convert it to double */

	if (Rast_read_range(name, mapset, &range) >= 0) {
	    /* if the integer range is empty */
	    if (range.first_time)
		return 2;

	    Rast_update_fp_range((DCELL) range.min, drange);
	    Rast_update_fp_range((DCELL) range.max, drange);
	    return 1;
	}
	return -1;
    }

    fd = -1;

    if (G_find_file2_misc("cell_misc", "f_range", name, mapset)) {
	fd = G_open_old_misc("cell_misc", "f_range", name, mapset);
	if (fd < 0) {
	    G_warning(_("Unable to read fp range file for <%s>"),
		      G_fully_qualified_name(name, mapset));
	    return -1;
	}

	if (read(fd, xdr_buf, sizeof(xdr_buf)) != sizeof(xdr_buf)) {
	    /* if the f_range file exists, but empty file, meaning Nulls */
	    close(fd);
	    G_debug(1, "Empty fp range file meaning Nulls for <%s>",
		      G_fully_qualified_name(name, mapset));
	    return 2;
	}

	G_xdr_get_double(&dcell1, xdr_buf[0]);
	G_xdr_get_double(&dcell2, xdr_buf[1]);

	Rast_update_fp_range(dcell1, drange);
	Rast_update_fp_range(dcell2, drange);
	close(fd);
    }

    return 1;
}

/*!
 * \brief Read raster range (CELL)
 *
 * This routine reads the range information for the raster map
 * <i>name</i> in <i>mapset</i> into the <i>range</i> structure.
 *
 * A diagnostic message is printed and -1 is returned if there is an error
 * reading the range file. Otherwise, 0 is returned.
 *
 * Old range file (those with 4 numbers) should treat zeros in this
 * file as NULL-values. New range files (those with just 2 numbers)
 * should treat these numbers as real data (zeros are real data in
 * this case).  An empty range file indicates that the min, max are
 * undefined. This is a valid case, and the result should be an
 * initialized range struct with no defined min/max. If the range file
 * is missing and the map is a floating-point map, this function will
 * create a default range by calling G_construct_default_range().
 *
 * \param name map name
 * \param mapset mapset name
 * \param[out] range pointer to Range structure which holds range info
 *
 * \return -1 on error
 * \return 1 on success
 * \return 2 if range is empty
 * \return 3 if raster map is floating-point, get range from quant rules
 */
int Rast_read_range(const char *name, const char *mapset, struct Range *range)
{
    FILE *fd;
    CELL x[4];
    char buf[200];
    int n, count;
    struct Quant quant;
    struct FPRange drange;
    Rast_init_range(range);
    fd = NULL;

    /* if map is not integer, read quant rules, and get limits */
    if (Rast_map_type(name, mapset) != CELL_TYPE) {
	DCELL dmin, dmax;

	if (Rast_read_quant(name, mapset, &quant) < 0) {
	    G_warning(_("Unable to read quant rules for raster map <%s>"),
		      G_fully_qualified_name(name, mapset));
	    return -1;
	}
	if (Rast_quant_is_truncate(&quant) || Rast_quant_is_round(&quant)) {
	    if (Rast_read_fp_range(name, mapset, &drange) >= 0) {
		Rast_get_fp_range_min_max(&drange, &dmin, &dmax);
		if (Rast_quant_is_truncate(&quant)) {
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
	    Rast_quant_get_limits(&quant, &dmin, &dmax, &x[0], &x[1]);

	Rast_update_range(x[0], range);
	Rast_update_range(x[1], range);
	return 3;
    }

    if (G_find_file2_misc("cell_misc", "range", name, mapset)) {
	fd = G_fopen_old_misc("cell_misc", "range", name, mapset);
	if (!fd) {
	    G_warning(_("Unable to read range file for <%s>"),
		      G_fully_qualified_name(name, mapset));
	    return -1;
	}

	/* if range file exists but empty */
        if (!fgets(buf, sizeof buf, fd)) {
            if (fd)
                fclose(fd);
	    return 2;
        }

	x[0] = x[1] = x[2] = x[3] = 0;
	count = sscanf(buf, "%d%d%d%d", &x[0], &x[1], &x[2], &x[3]);

	/* if wrong format */
	if (count <= 0) {
	    if (fd)
		fclose(fd);

	    G_warning(_("Unable to read range file for <%s>"),
		      G_fully_qualified_name(name, mapset));
	    return -1;
	}

	for (n = 0; n < count; n++) {
	    /* if count==4, the range file is old (4.1) and 0's in it
	       have to be ignored */
	    if (count < 4 || x[n])
		Rast_update_range((CELL) x[n], range);
	}
	fclose(fd);
    }

    return 1;
}

/*!
 * \brief Read raster stats
 *
 * Read the stats file <i>stats</i>. This file is
 * written in binary using XDR format.
 *
 * An empty stats file indicates that all cells are NULL. This
 * is a valid case, and the result should be an initialized rstats
 * struct with no defined stats.  If the stats file is missing
 * this function will create a default stats with count = 0.
 *
 * \param name map name
 * \param mapset mapset name
 * \param rstats pointer to R_stats structure which holds raster stats
 *
 * \return 1 on success
 * \return 2 stats is empty
 * \return -1 on error or stats file does not exist
 */
int Rast_read_rstats(const char *name, const char *mapset,
		       struct R_stats *rstats)
{
    int fd;
    char xdr_buf[2][XDR_DOUBLE_NBYTES];
    DCELL dcell1, dcell2;
    unsigned char cc[8];
    char nbytes;
    int i;
    grass_int64 count;

    Rast_init();
    init_rstats(rstats);

    fd = -1;

    if (!G_find_file2_misc("cell_misc", "stats", name, mapset)) {
	G_debug(1, "Stats file does not exist");
	return -1;
    }

    fd = G_open_old_misc("cell_misc", "stats", name, mapset);
    if (fd < 0) {
	G_warning(_("Unable to read stats file for <%s>"),
		  G_fully_qualified_name(name, mapset));
	return -1;
    }

    if (read(fd, xdr_buf, sizeof(xdr_buf)) != sizeof(xdr_buf)) {
	/* if the stats file exists, but empty file, meaning Nulls */
	close(fd);
	G_debug(1, "Empty stats file meaning Nulls for <%s>",
		  G_fully_qualified_name(name, mapset));
	return 2;
    }

    G_xdr_get_double(&dcell1, xdr_buf[0]);
    G_xdr_get_double(&dcell2, xdr_buf[1]);

    rstats->sum = dcell1;
    rstats->sumsq = dcell2;

    /* count; see cell_values_int() in get_row.c */
    nbytes = 1;
    if (read(fd, &nbytes, 1) != 1) {
	/* if the stats file exists, but empty file, meaning Nulls */
	close(fd);
	G_debug(1, "Unable to read byte count in stats file for <%s>",
		  G_fully_qualified_name(name, mapset));
	return -1;
    }

    count = 0;
    if (nbytes == 0)
	return 1;

    if (nbytes < 1 || nbytes > sizeof(grass_int64)) {
	close(fd);
	G_debug(1, "Invalid byte count in stats file for <%s>",
		  G_fully_qualified_name(name, mapset));
	return -1;
    }
    if (read(fd, cc, nbytes) != nbytes) {
	/* incorrect number of bytes for count */
	close(fd);
	G_debug(1, "Unable to read count in stats file for <%s>",
		  G_fully_qualified_name(name, mapset));
	return -1;
    }

    /* copy byte by byte */
    for (i = nbytes - 1; i >= 0; i--) {
	count = (count << 8);
	count = count + cc[i];
    }
    rstats->count = count;

    close(fd);

    return 1;
}

/*!
 * \brief Write raster range file
 *
 * This routine writes the range information for the raster map
 * <i>name</i> in the current mapset from the <i>range</i> structure.
 * A diagnostic message is printed and -1 is returned if there is an
 * error writing the range file. Otherwise, 0 is returned.
 *
 * This routine only writes 2 numbers (min,max) to the range
 * file, instead of the 4 (pmin,pmax,nmin,nmax) previously written.
 * If there is no defined min,max, an empty file is written.
 *
 * \param name map name
 * \param range pointer to Range structure which holds range info
 */
void Rast_write_range(const char *name, const struct Range *range)
{
    FILE *fp;

    Rast_write_rstats(name, &(range->rstats));

    if (Rast_map_type(name, G_mapset()) != CELL_TYPE) {
	G_remove_misc("cell_misc", "range", name);	/* remove the old file with this name */
	G_fatal_error(_("Unable to write range file for <%s>"), name);
    }

    fp = G_fopen_new_misc("cell_misc", "range", name);
    if (!fp) {
	G_remove_misc("cell_misc", "range", name);	/* remove the old file with this name */
	G_fatal_error(_("Unable to write range file for <%s>"), name);
    }

    /* if range has been updated */
    if (!range->first_time)
	fprintf(fp, "%ld %ld\n", (long)range->min, (long)range->max);

    fclose(fp);
}

/*!
 * \brief Write raster range file (floating-point)
 *
 * Write the floating point range file <tt>f_range</tt>. This file is
 * written in binary using XDR format. If there is no defined min/max
 * in <em>range</em>, an empty <tt>f_range</tt> file is created.
 *
 * \param name map name
 * \param range pointer to FPRange which holds fp range info
 */
void Rast_write_fp_range(const char *name, const struct FPRange *range)
{
    int fd;
    char xdr_buf[2][XDR_DOUBLE_NBYTES];

    Rast_init();

    Rast_write_rstats(name, &(range->rstats));

    fd = G_open_new_misc("cell_misc", "f_range", name);
    if (fd < 0) {
	G_remove_misc("cell_misc", "f_range", name);
	G_fatal_error(_("Unable to write range file for <%s>"), name);
    }

    /* if range hasn't been updated, write empty file meaning Nulls */
    if (range->first_time) {
	close(fd);
	return;
    }

    G_xdr_put_double(xdr_buf[0], &range->min);
    G_xdr_put_double(xdr_buf[1], &range->max);

    if (write(fd, xdr_buf, sizeof(xdr_buf)) != sizeof(xdr_buf)) {
	G_remove_misc("cell_misc", "f_range", name);
	G_fatal_error(_("Unable to write range file for <%s>"), name);
    }

    close(fd);
}

/*!
 * \brief Write raster stats file
 *
 * Write the stats file <tt>stats</tt>. This file is
 * written in binary using XDR format. If the count is < 1
 * in <em>rstats</em>, an empty <tt>stats</tt> file is created.
 *
 * \param name map name
 * \param rstats pointer to R_stats which holds stats info
 */
void Rast_write_rstats(const char *name, const struct R_stats *rstats)
{
    int fd;
    char xdr_buf[2][XDR_DOUBLE_NBYTES];
    unsigned char cc[8];
    char nbytes;
    int i;
    grass_int64 count;

    Rast_init();

    fd = G_open_new_misc("cell_misc", "stats", name);
    if (fd < 0) {
	G_remove_misc("cell_misc", "stats", name);
	G_fatal_error(_("Unable to write stats file for <%s>"), name);
    }

    /* if count is zero, write empty file meaning Nulls */
    if (rstats->count < 1) {
	close(fd);
	return;
    }

    G_xdr_put_double(xdr_buf[0], &rstats->sum);
    G_xdr_put_double(xdr_buf[1], &rstats->sumsq);

    if (write(fd, xdr_buf, sizeof(xdr_buf)) != sizeof(xdr_buf)) {
	G_remove_misc("cell_misc", "stats", name);
	G_fatal_error(_("Unable to write stats file for <%s>"), name);
    }

    /* count; see convert_int() in put_row.c */
    count = rstats->count;
    nbytes = 0;
    /* copy byte by byte */
    for (i = 0; i < sizeof(grass_int64); i++) {
	cc[i] = count & 0xff;
	count >>= 8;
	if (cc[i])
	    nbytes = i;
    }
    nbytes++;

    /* number of bytes needed for count */
    if (write(fd, &nbytes, 1) != 1) {
	G_remove_misc("cell_misc", "stats", name);
	G_fatal_error(_("Unable to write stats file for <%s>"), name);
    }

    if (nbytes > 0 && write(fd, cc, nbytes) != nbytes) {
	G_remove_misc("cell_misc", "stats", name);
	G_fatal_error(_("Unable to write stats file for <%s>"), name);
    }

    close(fd);
}

/*!
 * \brief Update range structure (CELL)
 *
 * Compares the <i>cat</i> value with the minimum and maximum values
 * in the <i>range</i> structure, modifying the range if <i>cat</i>
 * extends the range.
 *
 * NULL-values must be detected and ignored.
 *
 * \param cat raster value
 * \param range pointer to Range structure which holds range info
 */
void Rast_update_range(CELL cat, struct Range *range)
{
    if (!Rast_is_c_null_value(&cat)) {
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = cat;
	    range->max = cat;
	    return;
	}
	if (cat < range->min)
	    range->min = cat;
	if (cat > range->max)
	    range->max = cat;
    }
}

/*!
 * \brief Update range structure (floating-point)
 *
 * Compares the <i>cat</i> value with the minimum and maximum values
 * in the <i>range</i> structure, modifying the range if <i>cat</i>
 * extends the range.
 *
 * NULL-values must be detected and ignored.
 *
 * \param val raster value
 * \param range pointer to Range structure which holds range info
 */
void Rast_update_fp_range(DCELL val, struct FPRange *range)
{
    if (!Rast_is_d_null_value(&val)) {
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = val;
	    range->max = val;
	    return;
	}
	if (val < range->min)
	    range->min = val;
	if (val > range->max)
	    range->max = val;
    }
}

/*!
 * \brief Update range structure based on raster row (CELL)
 *
 * This routine updates the <i>range</i> data just like
 * Rast_update_range(), but for <i>n</i> values from the <i>cell</i>
 * array.
 *
 * \param cell raster values
 * \param n number of values
 * \param range pointer to Range structure which holds range info
 */
void Rast_row_update_range(const CELL * cell, int n, struct Range *range)
{
    Rast__row_update_range(cell, n, range, 0);
}

/*!
 * \brief Update range structure based on raster row
 *
 * Note: for internal use only.
 *
 * \param cell raster values
 * \param n number of values
 * \param range pointer to Range structure which holds range info
 * \param ignore_zeros ignore zeros
 */
void Rast__row_update_range(const CELL * cell, int n,
			    struct Range *range, int ignore_zeros)
{
    CELL cat;

    while (n-- > 0) {
	cat = *cell++;
	if (Rast_is_c_null_value(&cat) || (ignore_zeros && !cat))
	    continue;
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = cat;
	    range->max = cat;

	    range->rstats.sum = cat;
	    range->rstats.sumsq = (DCELL) cat * cat;
	    range->rstats.count = 1;

	    continue;
	}
	if (cat < range->min)
	    range->min = cat;
	if (cat > range->max)
	    range->max = cat;

	range->rstats.sum += cat;
	range->rstats.sumsq += (DCELL) cat * cat;
	range->rstats.count += 1;
    }
}

/*!
 * \brief Update range structure based on raster row (floating-point)
 *
 * This routine updates the <i>range</i> data just like
 * Rast_update_range(), but for <i>n</i> values from the <i>cell</i>
 * array.
 *
 * \param cell raster values
 * \param n number of values
 * \param range pointer to Range structure which holds range info
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void Rast_row_update_fp_range(const void *rast, int n,
			      struct FPRange *range,
			      RASTER_MAP_TYPE data_type)
{
    size_t size = Rast_cell_size(data_type);
    DCELL val = 0.0;

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

	if (Rast_is_null_value(rast, data_type)) {
	    rast = G_incr_void_ptr(rast, size);
	    continue;
	}
	if (range->first_time) {
	    range->first_time = 0;
	    range->min = val;
	    range->max = val;

	    range->rstats.sum = val;
	    range->rstats.sumsq = val * val;
	    range->rstats.count = 1;
	}
	else {
	    if (val < range->min)
		range->min = val;
	    if (val > range->max)
		range->max = val;

	    range->rstats.sum += val;
	    range->rstats.sumsq += val * val;
	    range->rstats.count += 1;
	}

	rast = G_incr_void_ptr(rast, size);
    }
}

/*!
 * \brief Initialize range structure
 *
 * Initializes the <i>range</i> structure for updates by
 * Rast_update_range() and Rast_row_update_range().
 *
 * Must set a flag in the range structure that indicates that no
 * min/max have been defined - probably a <tt>"first"</tt> boolean
 * flag.
 *
 * \param range pointer to Range structure which holds range info
 */
void Rast_init_range(struct Range *range)
{
    Rast_set_c_null_value(&(range->min), 1);
    Rast_set_c_null_value(&(range->max), 1);

    init_rstats(&range->rstats);

    range->first_time = 1;
}

/*!
 * \brief Get range min and max
 *
 * The minimum and maximum CELL values are extracted from the
 * <i>range</i> structure.
 *
 * If the range structure has no defined min/max (first!=0) there will
 * not be a valid range. In this case the min and max returned must be
 * the NULL-value.
 *
 * \param range pointer to Range structure which holds range info
 * \param[out] min minimum value
 * \param[out] max maximum value
 */
void Rast_get_range_min_max(const struct Range *range, CELL * min, CELL * max)
{
    if (range->first_time) {
	Rast_set_c_null_value(min, 1);
	Rast_set_c_null_value(max, 1);
    }
    else {
	if (Rast_is_c_null_value(&(range->min)))
	    Rast_set_c_null_value(min, 1);
	else
	    *min = range->min;

	if (Rast_is_c_null_value(&(range->max)))
	    Rast_set_c_null_value(max, 1);
	else
	    *max = range->max;
    }
}

/*!
 * \brief Initialize fp range
 *
 * Must set a flag in the range structure that indicates that no
 * min/max have been defined - probably a <tt>"first"</tt> boolean
 * flag.
 *
 * \param range pointer to FPRange which holds fp range info
 */
void Rast_init_fp_range(struct FPRange *range)
{
    Rast_set_d_null_value(&(range->min), 1);
    Rast_set_d_null_value(&(range->max), 1);

    init_rstats(&range->rstats);

    range->first_time = 1;
}

/*!
 * \brief Get minimum and maximum value from fp range
 *
 * Extract the min/max from the range structure <i>range</i>.  If the
 * range structure has no defined min/max (first!=0) there will not be
 * a valid range. In this case the min and max returned must be the
 * NULL-value.
 *
 * \param range pointer to FPRange which holds fp range info
 * \param[out] min minimum value
 * \param[out] max maximum value
 */
void Rast_get_fp_range_min_max(const struct FPRange *range,
			       DCELL * min, DCELL * max)
{
    if (range->first_time) {
	Rast_set_d_null_value(min, 1);
	Rast_set_d_null_value(max, 1);
    }
    else {
	if (Rast_is_d_null_value(&(range->min)))
	    Rast_set_d_null_value(min, 1);
	else
	    *min = range->min;

	if (Rast_is_d_null_value(&(range->max)))
	    Rast_set_d_null_value(max, 1);
	else
	    *max = range->max;
    }
}

static void init_rstats(struct R_stats *rstats)
{
    Rast_set_d_null_value(&(rstats->sum), 1);
    Rast_set_d_null_value(&(rstats->sumsq), 1);
    rstats->count = 0;
}
