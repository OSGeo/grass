/*!
 * \file lib/raster/raster.c
 *
 * \brief Raster Library - Raster cell value routines.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Compares raster values.
 *
 * \param v1,v2 values to be compared
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return  1 if p > q or only q is null value
 * \return -1 if p < q or only p is null value
 * \return  0 if p == q or p==q==null value
 */
int Rast_raster_cmp(const void *v1, const void *v2, RASTER_MAP_TYPE data_type)
{
    if (Rast_is_null_value(v1, data_type)) {
	if (Rast_is_null_value(v2, data_type))
	    return 0;
	else
	    return -1;
    }
    else if (Rast_is_null_value(v2, data_type))
	return 1;

    switch (data_type) {
    case CELL_TYPE:
	if (*((const CELL *)v1) > *((const CELL *)v2))
	    return 1;
	else if (*((const CELL *)v1) == *((const CELL *)v2))
	    return 0;
	else
	    return -1;
    case FCELL_TYPE:
	if (*((const FCELL *)v1) > *((const FCELL *)v2))
	    return 1;
	else if (*((const FCELL *)v1) == *((const FCELL *)v2))
	    return 0;
	else
	    return -1;
    case DCELL_TYPE:
	if (*((const DCELL *)v1) > *((const DCELL *)v2))
	    return 1;
	else if (*((const DCELL *)v1) == *((const DCELL *)v2))
	    return 0;
	else
	    return -1;
    }

    return 0;
}

/*!
 * \brief Copies raster values.
 *
 * If \p v2 is null value, sets \p v2 to null value.
 * \p n is typically size of the destination array
 * and the source array is at least that large.
 *
 * \param v1 destination array for raster values
 * \param v2 source array with raster values
 * \param n number of values to copy
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void Rast_raster_cpy(void *v1, const void *v2, int n,
		     RASTER_MAP_TYPE data_type)
{
    memcpy(v1, v2, n * Rast_cell_size(data_type));
}

/*!
 * \brief Places a CELL raster value
 *
 * If Rast_is_c_null_value() is true, sets p to null value. Converts CELL
 * val to data_type (type of p) and stores result in p. Used for
 * assigning CELL values to raster cells of any type.
 *
 * \param rast pointer to raster cell value
 * \param cval value to set
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void Rast_set_c_value(void *rast, CELL cval, RASTER_MAP_TYPE data_type)
{
    CELL c;

    c = cval;
    if (Rast_is_c_null_value(&c)) {
	Rast_set_null_value(rast, 1, data_type);
	return;
    }
    switch (data_type) {
    case CELL_TYPE:
	*((CELL *) rast) = cval;
	break;
    case FCELL_TYPE:
	*((FCELL *) rast) = (FCELL) cval;
	break;
    case DCELL_TYPE:
	*((DCELL *) rast) = (DCELL) cval;
	break;
    }
}

/*!
 * \brief Places a FCELL raster value
 *
 * If Rast_is_f_null_value() is true, sets p to null value. Converts
 * FCELL val to data_type (type of p) and stores result in p. Used for
 * assigning FCELL values to raster cells of any type.
 *
 * \param rast pointer to raster cell value
 * \param fval value to set
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void Rast_set_f_value(void *rast, FCELL fval, RASTER_MAP_TYPE data_type)
{
    FCELL f;

    f = fval;
    if (Rast_is_f_null_value(&f)) {
	Rast_set_null_value(rast, 1, data_type);
	return;
    }
    switch (data_type) {
    case CELL_TYPE:
	*((CELL *) rast) = (CELL) fval;
	break;
    case FCELL_TYPE:
	*((FCELL *) rast) = fval;
	break;
    case DCELL_TYPE:
	*((DCELL *) rast) = (DCELL) fval;
	break;
    }
}


/*!
 * \brief  Places a DCELL raster value
 *
 * If Rast_is_d_null_value() is true, sets p to null value. Converts
 * DCELL val to data_type (type of p) and stores result in p. Used for
 * assigning DCELL values to raster cells of any type.
 *
 * \param rast pointer to raster cell value
 * \param fval value to set
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void Rast_set_d_value(void *rast, DCELL dval, RASTER_MAP_TYPE data_type)
{
    DCELL d;

    d = dval;
    if (Rast_is_d_null_value(&d)) {
	Rast_set_null_value(rast, 1, data_type);
	return;
    }
    switch (data_type) {
    case CELL_TYPE:
	*((CELL *) rast) = (CELL) dval;
	break;
    case FCELL_TYPE:
	*((FCELL *) rast) = (FCELL) dval;
	break;
    case DCELL_TYPE:
	*((DCELL *) rast) = dval;
	break;
    }
}

/*!
 * \brief Retrieves the value of give type from pointer p
 *
 * Retrieves the value of type data_type from pointer p, converts it
 * to CELL type and returns the result. If null value is stored in p,
 * returns CELL null value.
 *
 * Used for retrieving CELL values from raster cells of any type.
 *
 * Note: when data_type != CELL_TYPE, no quantization is used, only
 * type conversion.
 *
 * \param rast pointer to raster cell value
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return raster value
 */
CELL Rast_get_c_value(const void *rast, RASTER_MAP_TYPE data_type)
{
    CELL c;

    if (Rast_is_null_value(rast, data_type)) {
	Rast_set_c_null_value(&c, 1);
	return c;
    }
    switch (data_type) {
    case CELL_TYPE:
	return *((const CELL *)rast);
    case FCELL_TYPE:
	return (CELL) * ((const FCELL *)rast);
    case DCELL_TYPE:
	return (CELL) * ((const DCELL *)rast);
    }

    return 0;
}

/*!
 * \brief Retrieves the value of given raster type from pointer p (FCELL)
 *
 * Retrieves the value of type data_type from pointer p, converts it
 * to FCELL type and returns the result. If null value is stored in p,
 * returns FCELL null value.
 *
 * Used for retrieving FCELL values from raster cells of any type.
 *
 * \param rast pointer to raster cell value
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return raster value
 */
FCELL Rast_get_f_value(const void *rast, RASTER_MAP_TYPE data_type)
{
    FCELL f;

    if (Rast_is_null_value(rast, data_type)) {
	Rast_set_f_null_value(&f, 1);
	return f;
    }
    switch (data_type) {
    case CELL_TYPE:
	return (FCELL) * ((const CELL *)rast);
    case FCELL_TYPE:
	return *((const FCELL *)rast);
    case DCELL_TYPE:
	return (FCELL) * ((const DCELL *)rast);
    }

    return 0;
}


/*!
 * \brief Retrieves the value of given type from pointer p (DCELL)
 *
 * Retrieves the value of type data_type from pointer p, converts it
 * to DCELL type and returns the result. If null value is stored in p,
 * returns DCELL null value.

 * Used for retrieving DCELL values from raster cells of any type.
 *
 * \param rast pointer to raster cell value
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return raster value
 */
DCELL Rast_get_d_value(const void *rast, RASTER_MAP_TYPE data_type)
{
    DCELL d;

    if (Rast_is_null_value(rast, data_type)) {
	Rast_set_d_null_value(&d, 1);
	return d;
    }
    switch (data_type) {
    case CELL_TYPE:
	return (DCELL) * ((const CELL *)rast);
    case FCELL_TYPE:
	return (DCELL) * ((const FCELL *)rast);
    case DCELL_TYPE:
	return *((const DCELL *)rast);
    }

    return 0;
}
