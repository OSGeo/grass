#include <stdlib.h>
#include <grass/gis.h>


/*!
 * \brief  Advance void pointer
 *
 * Advances void pointer by <em>size</em> bytes
 * Returns new pointer value.
 *
 * Useful in raster row processing loops, substitutes
 \code
 CELL *cell; 
 cell += n;
 \endcode
 *
 * Now 
 \code
 rast = G_incr_void_ptr(rast, G_raster_size(data_type))
 \endcode
 *
 * (where rast is void* and data_type is RASTER_MAP_TYPE can be used instead
 * of rast++.)
 *
 * very useful to generalize the row processing - loop i.e.
 *   void * buf_ptr += G_raster_size(data_type)
 *
 *  \param ptr
 *  \param size
 *  \return void * 
 */

void *G_incr_void_ptr(const void *ptr, const size_t size)
{
    /* assuming that the size of unsigned char is 1 */
    return (void *)((const unsigned char *)ptr + size);
}


/*!
 * \brief  Compares raster values p and q
 *
 * Returns:
 *   1 if p > q or only q is null value
 *  -1 if p < q or only p is null value
 *   0 if p == q or p==q==null value
 *
 *  \param p
 *  \param q
 *  \param data_type
 *  \return int
 */

int G_raster_cmp(const void *v1, const void *v2, RASTER_MAP_TYPE data_type)
{
    if (G_is_null_value(v1, data_type)) {
	if (G_is_null_value(v2, data_type))
	    return 0;
	else
	    return -1;
    }
    else if (G_is_null_value(v2, data_type))
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
 * \brief  Copies raster values q into p
 *
 * If q is null value, sets q to null value.
 *
 *  \param p
 *  \param q
 *  \param n
 *  \param data_type
 *  \return int
 */

int G_raster_cpy(void *v1, const void *v2, int n, RASTER_MAP_TYPE data_type)
{
    G_copy((char *)v1, (char *)v2, n * G_raster_size(data_type));
    return 0;
}


/*!
 * \brief  Places a CELL raster value
 *
 * If G_is_c_null_value(val) is true, sets p to null value.
 * Converts CELL val to data_type (type of p) and stores result in p.
 * Used for assigning CELL values to raster cells of any type.
 *
 *  \param p
 *  \param val
 *  \param data_type
 *  \return int
 */

int G_set_raster_value_c(void *rast, CELL cval, RASTER_MAP_TYPE data_type)
{
    CELL c;

    c = cval;
    if (G_is_c_null_value(&c)) {
	G_set_null_value(rast, 1, data_type);
	return 0;
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

    return 0;
}


/*!
 * \brief  Places a FCELL raster value
 *
 * If G_is_f_null_value(val) is true, sets p to null value.
 * Converts FCELL val to data_type (type of p) and stores result in p.
 * Used for assigning FCELL values to raster cells of any type.
 *
 *  \param p
 *  \param val
 *  \param data_type
 *  \return int
 */

int G_set_raster_value_f(void *rast, FCELL fval, RASTER_MAP_TYPE data_type)
{
    FCELL f;

    f = fval;
    if (G_is_f_null_value(&f)) {
	G_set_null_value(rast, 1, data_type);
	return 0;
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

    return 0;
}


/*!
 * \brief  Places a DCELL raster value
 *
 * If G_is_d_null_value(val) is true, sets p to null value.
 * Converts DCELL val to data_type (type of p) and stores result in p.
 * Used for assigning DCELL values to raster cells of any type.
 *
 *  \param p
 *  \param val
 *  \param data_type
 *  \return int
 */

int G_set_raster_value_d(void *rast, DCELL dval, RASTER_MAP_TYPE data_type)
{
    DCELL d;

    d = dval;
    if (G_is_d_null_value(&d)) {
	G_set_null_value(rast, 1, data_type);
	return -1;
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

    return 0;
}


/*!
 * \brief  Retrieves the value of type data_type from pointer p
 *
 * Retrieves the value of type data_type from pointer p,
 * converts it to CELL type and returns the result.
 * If null value is stored in p, returns CELL null value.
 *
 * Used for retrieving CELL values from raster cells of any type.
 *
 * NOTE: when data_type != CELL_TYPE, no quantization is used, only
 * type conversion.
 *
 *  \param p
 *  \param data_type
 *  \return CELL
 */

CELL G_get_raster_value_c(const void *rast, RASTER_MAP_TYPE data_type)
{
    CELL c;

    if (G_is_null_value(rast, data_type)) {
	G_set_c_null_value(&c, 1);
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
 * \brief  Retrieves the value of type data_type from pointer p
 *
 * Retrieves the value of type data_type from pointer p,
 * converts it to FCELL type and returns the result.
 * If null value is stored in p, returns FCELL null value.
 *
 * Used for retrieving FCELL values from raster cells of any type.
 *
 *  \param p
 *  \param data_type
 *  \return FCELL
 */

FCELL G_get_raster_value_f(const void *rast, RASTER_MAP_TYPE data_type)
{
    FCELL f;

    if (G_is_null_value(rast, data_type)) {
	G_set_f_null_value(&f, 1);
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
 * \brief  Retrieves the value of type data_type from pointer p,
 *
 * Retrieves the value of type data_type from pointer p,
 * converts it to DCELL type and returns the result.
 * If null value is stored in p, returns DCELL null value.

 * Used for retrieving DCELL values from raster cells of any type.
 *
 *  \param p
 *  \param data_type
 *  \return DCELL
 */

DCELL G_get_raster_value_d(const void *rast, RASTER_MAP_TYPE data_type)
{
    DCELL d;

    if (G_is_null_value(rast, data_type)) {
	G_set_d_null_value(&d, 1);
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
