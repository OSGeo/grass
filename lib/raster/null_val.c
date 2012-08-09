/*!
 * \file lib/raster/null_val.c
 *
 * \brief Raster Library - NULL value management
 *
 * To provide functionality to handle NULL values for data types CELL,
 * FCELL, and DCELL. May need more...
 *
 * (C) 2001-2009 GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author unknown - probably CERL
 * \author Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
 */

/* System include files */
#include <string.h>

/* Grass and local include files */
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static void EmbedGivenNulls(void *, char *, RASTER_MAP_TYPE, int);

/*!
   \brief To insert null values into a map. Needs more.....

   \param cell raster values
   \param nulls raster null values
   \param map_type type of raster - CELL, FCELL, DCELL
   \param ncols number of columns
 */
void EmbedGivenNulls(void *cell, char *nulls, RASTER_MAP_TYPE map_type,
		     int ncols)
{
    CELL *c;
    FCELL *f;
    DCELL *d;
    int i;

    c = (CELL *) cell;
    f = (FCELL *) cell;
    d = (DCELL *) cell;

    for (i = 0; i < ncols; i++) {
	if (nulls[i]) {
	    switch (map_type) {
	    case CELL_TYPE:
		Rast_set_c_null_value((CELL *) (c + i), 1);
		break;

	    case FCELL_TYPE:
		Rast_set_f_null_value((FCELL *) (f + i), 1);
		break;

	    case DCELL_TYPE:
		Rast_set_d_null_value((DCELL *) (d + i), 1);
		break;

	    default:
		G_warning(_("EmbedGivenNulls: wrong data type"));
	    }
	}
    }
}

/*!
   \brief To set one or more raster values to null.

   It also sets null to zero if null_is_zero is TRUE.

   \param rast pointer to values to set to null
   \param numVals number of values to set to null
   \param null_is_zero flag to indicate if NULL = 0
   \param data_type type of raster - CELL, FCELL, DCELL
 */
void Rast__set_null_value(void *rast, int numVals, int null_is_zero,
			  RASTER_MAP_TYPE data_type)
{
    if (null_is_zero) {
	G_zero((char *)rast, numVals * Rast_cell_size(data_type));
	return;
    }

    Rast_set_null_value(rast, numVals, data_type);
}

/*!
   \brief To set one or more raster values to null.

   \param buf pointer to values to set to null
   \param numVals number of values to set to null
   \param data_type type of raster - CELL, FCELL, DCELL
 */
void Rast_set_null_value(void *buf, int numVals, RASTER_MAP_TYPE data_type)
{
    switch (data_type) {
    case CELL_TYPE:
	Rast_set_c_null_value((CELL *) buf, numVals);
	break;

    case FCELL_TYPE:
	Rast_set_f_null_value((FCELL *) buf, numVals);
	break;

    case DCELL_TYPE:
	Rast_set_d_null_value((DCELL *) buf, numVals);
	break;

    default:
	G_warning(_("Rast_set_null_value: wrong data type!"));
    }
}

/*!
   \brief To set a number of CELL raster values to NULL.

   \param cellVals pointer to CELL values to set to null
   \param numVals  number of values to set to null
 */
void Rast_set_c_null_value(CELL * cellVals, int numVals)
{
    int i;			/* counter */

    for (i = 0; i < numVals; i++)
	cellVals[i] = (int)0x80000000;
}

/*!
   \brief To set a number of FCELL raster values to NULL.

   \param fcellVals pointer to FCELL values to set to null
   \param numVals number of values to set to null
 */
void Rast_set_f_null_value(FCELL * fcellVals, int numVals)
{
    static const unsigned char null_bits[4] = {
	0xFF, 0xFF, 0xFF, 0xFF
    };
    int i;

    for (i = 0; i < numVals; i++)
	memcpy(&fcellVals[i], null_bits, sizeof(null_bits));
}

/*!
   \brief To set a number of DCELL raster values to NULL.

   \param dcellVals pointer to DCELL values to set to null
   \param numVals number of values to set to null
 */
void Rast_set_d_null_value(DCELL * dcellVals, int numVals)
{
    static const unsigned char null_bits[8] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    int i;

    for (i = 0; i < numVals; i++)
	memcpy(&dcellVals[i], null_bits, sizeof(null_bits));
}

/*!
   \brief To check if a raster value is set to NULL

   - If the <em>data_type</em> is CELL_TYPE, calls Rast_is_c_null_value()
   - If the <em>data_type</em> is FCELL_TYPE, calls Rast_is_f_null_value()
   - If the <em>data_type</em> is DCELL_TYPE, calls Rast_is_d_null_value()

   \param rast raster value to check 
   \param data_type type of raster - CELL, FCELL, DCELL

   \return TRUE if raster value is NULL
   \return FALSE otherwise
 */
int Rast_is_null_value(const void *rast, RASTER_MAP_TYPE data_type)
{
    switch (data_type) {
    case CELL_TYPE:
	return (Rast_is_c_null_value((CELL *) rast));

    case FCELL_TYPE:
	return (Rast_is_f_null_value((FCELL *) rast));

    case DCELL_TYPE:
	return (Rast_is_d_null_value((DCELL *) rast));

    default:
	G_warning("Rast_is_null_value: wrong data type!");
	return FALSE;
    }
}

/*!
   \brief To check if a CELL raster value is set to NULL

   Returns 1 if <em>cell</em> is NULL, 0 otherwise. This will test if the
   value <em>cell</em> is the largest <tt>int</tt>.

   \param cellVal CELL raster value to check

   \return TRUE if CELL raster value is NULL
   \return FALSE otherwise
 */
#ifndef Rast_is_c_null_value
int Rast_is_c_null_value(const CELL * cellVal)
{
    /* Check if the CELL value matches the null pattern */
    return *cellVal == (CELL) 0x80000000;
}
#endif

/*!
   \brief To check if a FCELL raster value is set to NULL

   Returns 1 if <em>fcell</em> is NULL, 0 otherwise. This will test if
   the value <em>fcell</em> is a NaN. It isn't good enough to test for
   a particular NaN bit pattern since the machine code may change this
   bit pattern to a different NaN. The test will be

   \code
   if(fcell==0.0) return 0;
   if(fcell>0.0) return 0;
   if(fcell<0.0) return 0;
   return 1;
   \endcode

   or (as suggested by Mark Line)
   \code
   return (fcell != fcell);
   \endcode

   \param fcellVal FCELL raster value to check

   \return TRUE if FCELL raster value is NULL
   \return FALSE otherwise
 */
#ifndef Rast_is_f_null_value
int Rast_is_f_null_value(const FCELL * fcellVal)
{
    return *fcellVal != *fcellVal;
}
#endif

/*!
   \brief To check if a DCELL raster value is set to NULL

   Returns 1 if <em>dcell</em> is NULL, 0 otherwise. This will test if
   the value <em>dcell</em> is a NaN. Same test as in
   Rast_is_f_null_value().

   \param dcellVal DCELL raster value to check

   \return TRUE if DCELL raster value is NULL
   \return FALSE otherwise
 */
#ifndef Rast_is_d_null_value
int Rast_is_d_null_value(const DCELL * dcellVal)
{
    return *dcellVal != *dcellVal;
}
#endif

/*!
   \brief To insert null values into a map.

   - If the <em>data_type</em> is CELL_TYPE, calls Rast_insert_c_null_values()
   - If the <em>data_type</em> is FCELL_TYPE, calls Rast_insert_f_null_values()
   - If the <em>data_type</em> is DCELL_TYPE, calls Rast_insert_d_null_values()

   \param rast pointer raster values
   \param null_row null row
   \param ncols number of columns
   \param data_type type of raster - CELL, FCELL, DCELL
 */
void Rast_insert_null_values(void *rast, char *null_row, int ncols,
			     RASTER_MAP_TYPE data_type)
{
    EmbedGivenNulls(rast, null_row, data_type, ncols);
}

/*!
   \brief To insert null values into an integer raster map (CELL)

   For each of the <em>count</em> <em>flags</em> which is true(!=0),
   set the corresponding <em>cell</em> to the NULL value.

   \param rast pointer raster values
   \param null_row null row
   \param ncols number of columns
 */
void Rast_insert_c_null_values(CELL * cellVal, char *null_row, int ncols)
{
    EmbedGivenNulls((void *)cellVal, null_row, CELL_TYPE, ncols);
}

/*!
   \brief To insert null values into an floating-point raster map (FCELL)

   \param fcellVal pointer raster values
   \param null_row null row
   \param ncols number of columns
 */
void Rast_insert_f_null_values(FCELL * fcellVal, char *null_row, int ncols)
{
    EmbedGivenNulls((void *)fcellVal, null_row, FCELL_TYPE, ncols);
}

/*!
   \brief To insert null values into an floating-point raster map (FCELL)

   For each for the <em>count</em> <em>flag</em> which is true(!=0), set
   the corresponding <em>dcell</em> to the NULL value.

   \param dcellVal pointer raster values
   \param null_row null row
   \param ncols number of columns
 */
void Rast_insert_d_null_values(DCELL * dcellVal, char *null_row, int ncols)
{
    EmbedGivenNulls((void *)dcellVal, null_row, DCELL_TYPE, ncols);
}

/*!
   \brief Check NULL

   Note: Only for internal use.

   \param flags null bitmap
   \param bit_num index of bit to check
   \param n size of null bitmap (in bits)

   \return 1 if set, 0 if unset
 */
int Rast__check_null_bit(const unsigned char *flags, int bit_num, int n)
{
    int ind;
    int offset;

    /* check that bit_num is in range */
    if (bit_num < 0 || bit_num >= n)
	G_fatal_error("Rast__check_null_bit: index %d out of range (size = %d).",
		      bit_num, n);


    /* find the index of the unsigned char in which this bit appears */
    ind = bit_num / 8;

    offset = bit_num & 7;

    return ((flags[ind] & ((unsigned char)0x80 >> offset)) != 0);
}

/*!
   \brief Given array of 0/1 of length n starting from column.

   Note: Only for internal use.

   Given array of 0/1 of length n starting from column set the
   corresponding bits of flags; total number of bits in flags is ncols.

   \param zero_ones
   \param flags
   \param col
   \param n
   \param ncols

   \return 0
   \return 1
 */
int G__set_flags_from_01_random(const char *zero_ones, unsigned char *flags,
				int col, int n, int ncols)
{
    unsigned char v;
    int count;
    int size;
    int i, k;

    if (col == 0 && n == ncols) {
	Rast__convert_01_flags(zero_ones, flags, n);
	return 0;
    }

    count = 0;
    size = Rast__null_bitstream_size(ncols);

    for (i = 0; i < size; i++) {
	v = 0;
	k = 8;

	while (k-- > 0) {
	    if (count >= col && count < (col + n)) {
		v = v | ((unsigned char)zero_ones[count - col] << k);
	    }
	    else if (count < ncols) {
		v = v |
		    ((unsigned char)Rast__check_null_bit(flags, count, ncols)
		     << k);
	    }

	    /* otherwise  keep this bit the same as it was */
	    count++;
	}

	flags[i] = v;
    }

    return 1;
}

/*!
   \brief ?

   Note: Only for internal use.

   \param zero_ones
   \param flags
   \param n
 */
void Rast__convert_01_flags(const char *zero_ones, unsigned char *flags,
			    int n)
{
    unsigned char *v;
    int count;
    int size;
    int i, k;

    /* pad the flags with 0's to make size multiple of 8 */
    v = flags;
    size = Rast__null_bitstream_size(n);
    count = 0;

    for (i = 0; i < size; i++) {
	*v = 0;
	k = 8;

	while (k-- > 0) {
	    if (count < n) {
		*v = *v | ((unsigned char)zero_ones[count] << k);
	    }

	    count++;
	}

	v++;
    }
}

/*!
   \brief ?

   Note: Only for internal use.

   \param zero_ones
   \param flags
   \param n
 */
void Rast__convert_flags_01(char *zero_ones, const unsigned char *flags,
			    int n)
{
    const unsigned char *v;
    int count;
    int size;
    int i, k;

    count = 0;
    v = flags;
    size = Rast__null_bitstream_size(n);

    for (i = 0; i < size; i++) {
	k = 8;

	while (k-- > 0) {
	    if (count < n) {
		zero_ones[count] = ((*v & ((unsigned char)1 << k)) != 0);
		count++;
	    }
	}

	v++;
    }
}

/*!
   \brief ?

   Note: Only for internal use.

   \param flags
   \param cols
 */
void Rast__init_null_bits(unsigned char *flags, int cols)
{
    unsigned char *v;
    int size;
    int i;

    /* pad the flags with 0's to make size multiple of 8 */
    v = flags;
    size = Rast__null_bitstream_size(cols);

    for (i = 0; i < size; i++) {
	if ((i + 1) * 8 <= cols) {
	    *v = (unsigned char)255;
	}
	else {
	    *v = (unsigned char)255 << ((i + 1) * 8 - cols);
	}

	v++;
    }
}
