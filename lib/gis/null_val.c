/*
*
*****************************************************************************
*
* MODULE:   	GRASS gis library
* AUTHOR(S):	Original author unknown - probably CERL
*   	    	Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
* PURPOSE:  	To provide functionality to handle NULL values for data types
*   	    	CELL, FCELL, and DCELL. May need more...
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

/*============================= Include Files ==============================*/

/* System include files */
#include <string.h>

/* Grass and local include files */
#include <grass/gis.h>
#include <grass/glocale.h>

/*======================= Internal Constants/Defines =======================*/

/* none */

/*========================== Internal Typedefs =============================*/

/* none */

/*====================== Static Variable Declaration =======================*/

/* Null pattern variables */
static CELL 	cellNullPattern;
static FCELL	fcellNullPattern;
static DCELL	dcellNullPattern;

/* Flag to indicate null patterns are initialized */
static int  	initialized = FALSE;

/*============================== Prototypes ================================*/

static int EmbedGivenNulls (void *, char *, RASTER_MAP_TYPE, int);
static void InitError (void);

/*======================= Internal Static Functions ========================*/

/****************************************************************************
* int EmbedGivenNulls (void *cell, char *nulls, RASTER_MAP_TYPE map_type,
*   int ncols)
*
* PURPOSE: 	To insert null values into a map. Needs more.....
* INPUT VARS:	cell	    =>	??
*   	    	nulls	    =>	??
*   	    	map_type    =>	type of raster - CELL, FCELL, DCELL
*   	    	ncols	    =>	??
* RETURN VAL:	??
*****************************************************************************/
static int EmbedGivenNulls (void *cell, char *nulls, RASTER_MAP_TYPE map_type,
    int ncols)
{
    CELL    *c;
    FCELL   *f;
    DCELL   *d;
    int     i;

    c = (CELL *) cell;
    f = (FCELL *) cell;
    d = (DCELL *) cell;

    for(i = 0; i < ncols; i++)
    {
    	if(nulls[i])
      	{
       	    switch (map_type)
            {
            	case CELL_TYPE:
	    	    G_set_c_null_value((CELL *) (c + i), 1);
		    break;
            
	    	case FCELL_TYPE:
	    	    G_set_f_null_value((FCELL *) (f + i), 1);
		    break;
            
	    	case DCELL_TYPE:
	    	    G_set_d_null_value((DCELL *) (d + i), 1);
		    break;
		
		default:
		    G_warning(_("EmbedGivenNulls: wrong data type!"));
            }
    	}
    }
    
    return 1;
}

/****************************************************************************
* void InitError (void)
*
* PURPOSE: 	To print an error message and exit the program. This function
*   	    	is called if something tries to access a null pattern before
*   	    	it is initialized.
* INPUT VARS:	none
* RETURN VAL:	none
*****************************************************************************/
static void InitError (void)
{
    char    errMsg[512];    /* array to hold error message */
    
    strcpy(errMsg, _("Null values have not been initialized. "));
    strcat(errMsg, _("G_gisinit() must be called first. "));
    strcat(errMsg, _("Please advise GRASS developers of this error.\n"));
    G_fatal_error(errMsg);

    return;
} 

/*========================== Library Functions =============================*/

/****************************************************************************
* void G__init_null_patterns (void)
*
* PURPOSE: 	To initialize the three null patterns for CELL, FCELL, and
*   	    	DCELL data types. It also sets the initialized flag to TRUE.
*   	    	This function is called by G_gisinit()
* INPUT VARS:	none
* RETURN VAL:	none
*****************************************************************************/
void G__init_null_patterns (void)
{
    unsigned char   *bytePtr;	    /* pointer to traverse FCELL and DCELL */
    int     	    numBits;	    /* number of bits for CELL type */
    int     	    i;	    	    /* counter */
    
    if (!initialized)
    {
    	/* Create the null pattern for the CELL data type - set the left */
	/* most bit to 1 and the rest to 0, basically INT_MIN. Since CELL is */
	/* some type of integer the bytes are not split into exponent and */
	/* mantissa. Thus a simple left shift can be used */
    	numBits = sizeof(CELL) * 8;
    
    	cellNullPattern = 1 << (numBits - 1);

    	/* Create the null pattern for the FCELL data type - set all bits */
	/* to 1, basically NaN. Need to use a byte pointer since bytes */
	/* represent the exponent and mantissa */
    	bytePtr = (unsigned char *) &fcellNullPattern;

    	for (i = 0; i < sizeof(FCELL); i++)
    	{
    	    *bytePtr = (unsigned char) 255;
	    bytePtr++;
    	}

    	/* Create the null pattern for the DCELL data type - set all bits */
	/* to 1, basically NaN. Need to use a byte pointer since bytes */
	/* represent the exponent and mantissa */
    	bytePtr = (unsigned char *) &dcellNullPattern;
    
    	for (i = 0; i < sizeof(DCELL); i++)
    	{
    	    *bytePtr = (unsigned char) 255;
	    bytePtr++;
    	}

    	/* Set the initialized flag to TRUE */
    	initialized = TRUE;
    }
    
    return;
}

/****************************************************************************
* void G__set_null_value (void *rast, int numVals, int null_is_zero,
*   RASTER_MAP_TYPE data_type)
*
* PURPOSE: 	To set one or more raster values to null. It also sets null
*   	    	to zero if null_is_zero is TRUE.
* INPUT VARS:	rast	    	=>  pointer to values to set to null
*   	    	numVals     	=>  number of values to set to null
*   	    	null_is_zero	=>  flag to indicate if NULL = 0
*   	    	data_type   	=>  type of raster - CELL, FCELL, DCELL
* RETURN VAL:	none
*****************************************************************************/
void G__set_null_value (void *rast, int numVals, int null_is_zero,
    RASTER_MAP_TYPE data_type)
{
    if (null_is_zero)
    {
    	G_zero((char *) rast, numVals * G_raster_size(data_type));
        return;
    }
    
    G_set_null_value(rast, numVals, data_type);

    return;
} 

/****************************************************************************
* void G_set_null_value (void *buf, int numVals, RASTER_MAP_TYPE data_type)
*
* PURPOSE: 	To set one or more raster values to null.
* INPUT VARS:	buf	    =>	pointer to values to set to null
*   	    	numVals     =>	number of values to set to null
*   	    	data_type   =>	type of raster - CELL, FCELL, DCELL
* RETURN VAL:	none
*****************************************************************************/
void G_set_null_value (void *buf, int numVals, RASTER_MAP_TYPE data_type)
{
    switch (data_type)
    {
    	case CELL_TYPE:
    	    G_set_c_null_value((CELL *) buf, numVals);
	    break;
	    
        case FCELL_TYPE:
	    G_set_f_null_value((FCELL *) buf, numVals);
	    break;
	    
        case DCELL_TYPE:
	    G_set_d_null_value((DCELL *) buf, numVals);
	    break;
		
	default:
	    G_warning(_("G_set_null_value: wrong data type!"));
    }

    return;
} 

/****************************************************************************
* void G_set_c_null_value (CELL *cellVals, int numVals)
*
* PURPOSE: 	To set a number of CELL raster values to NULL.
* INPUT VARS:	cellVals    =>	pointer to CELL values to set to null
*   	    	numVals     =>	number of values to set to null
* RETURN VAL:	none
*****************************************************************************/
void G_set_c_null_value (CELL *cellVals, int numVals)
{
    CELL    *cellPtr;	    /* pointer to CELL array to set to null */
    int     i;	    	    /* counter */
    
    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Set numVals consecutive CELL values to null */
    cellPtr = cellVals;
    
    for (i = 0; i < numVals; i++)
    {
    	*cellPtr = cellNullPattern;
	cellPtr++;
    }
    
    return;
} 

/****************************************************************************
* void G_set_f_null_value (FCELL *fcellVals, int numVals)
*
* PURPOSE: 	To set a number of FCELL raster values to NULL.
* INPUT VARS:	fcellVals   =>	pointer to FCELL values to set to null
*   	    	numVals     =>	number of values to set to null
* RETURN VAL:	none
*****************************************************************************/
void G_set_f_null_value (FCELL *fcellVals, int numVals)
{
    FCELL   *fcellPtr;	    /* pointer to FCELL array to set to null */
    int     i;	    	    /* counter */
    
    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Set numVals consecutive FCELL values to null */
    fcellPtr = fcellVals;
    
    for (i = 0; i < numVals; i++)
    {
    	*fcellPtr = fcellNullPattern;
	fcellPtr++;
    }

    return;
} 

/****************************************************************************
* void G_set_d_null_value (DCELL *dcellVals, int numVals)
*
* PURPOSE: 	To set a number of DCELL raster values to NULL.
* INPUT VARS:	dcellVals   =>	pointer to DCELL values to set to null
*   	    	numVals     =>	number of values to set to null
* RETURN VAL:	none
*****************************************************************************/
void G_set_d_null_value (DCELL *dcellVals, int numVals)
{
    DCELL   *dcellPtr;	    /* pointer to DCELL array to set to null */
    int     i;	    	    /* counter */
    
    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Set numVals consecutive DCELL values to null */
    dcellPtr = dcellVals;
    
    for (i = 0; i < numVals; i++)
    {
    	*dcellPtr = dcellNullPattern;
	dcellPtr++;
    }
    
    return;
} 

/****************************************************************************
* int G_is_null_value (void *rast, RASTER_MAP_TYPE data_type)
*
* PURPOSE: 	To check if a raster value is set to NULL
* INPUT VARS:	rast	    =>	raster value to check 
*   	    	data_type   =>	type of raster - CELL, FCELL, DCELL
* RETURN VAL:	TRUE if raster value is NULL FALSE otherwise
*****************************************************************************/

/*!
 * \brief 
 *
 * If the <em>data_type</em> is CELL_TYPE, calls G_is_c_null_value ((CELL *)
 * rast);
 * If the <em>data_type</em> is FCELL_TYPE, calls G_is_f_null_value ((FCELL
 * *) rast);
 * If the <em>data_type</em> is DCELL_TYPE, calls G_is_d_null_value ((DCELL
 * *) rast);
 *
 *  \param rast
 *  \param data_type
 *  \return int
 */

int G_is_null_value (const void *rast, RASTER_MAP_TYPE data_type)
{
    switch(data_type)
    {
      	case CELL_TYPE:
	    return (G_is_c_null_value((CELL *) rast));
        
	case FCELL_TYPE:
	    return (G_is_f_null_value((FCELL *) rast));
        
	case DCELL_TYPE:
	    return (G_is_d_null_value((DCELL *) rast));
        
	default:
	    G_warning("G_is_null_value: wrong data type!");
	    return FALSE;
    }
}

/****************************************************************************
* 
* int G_is_c_null_value (CELL *cellVal)
*
* PURPOSE: 	To check if a CELL raster value is set to NULL
* INPUT VARS:	cellVal    =>	CELL raster value to check
* RETURN VAL:	TRUE if CELL raster value is NULL FALSE otherwise
*****************************************************************************/

/*!
 * \brief 
 *
 * Returns 1 if <em>cell</em> is
 * NULL, 0 otherwise. This will test if the value <em>cell</em> is the largest <tt>int</tt>.
 *
 *  \param cell
 *  \return int
 */

int G_is_c_null_value (const CELL *cellVal)
{
    int     i;	    /* counter */

    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Check if the CELL value matches the null pattern */
    for(i = 0; i < sizeof (CELL); i++)
    {
    	if(((unsigned char *) cellVal)[i] !=
            ((unsigned char *) &cellNullPattern)[i])
	{
            return FALSE;
	}
    }
    
    return TRUE;
}

/****************************************************************************
* 
* int G_is_f_null_value (FCELL *fcellVal)
*
* PURPOSE: 	To check if a FCELL raster value is set to NULL
* INPUT VARS:	fcellVal    =>	FCELL raster value to check
* RETURN VAL:	TRUE if FCELL raster value is NULL FALSE otherwise
*****************************************************************************/

/*!
 * \brief 
 *
 * Returns 1 if <em>fcell</em>
 * is NULL, 0 otherwise. This will test if the value <em>fcell</em> is a NaN. It
 * isn't good enough to test for a particular NaN bit pattern since the machine
 * code may change this bit pattern to a different NaN. The test will be
 \code
  if(fcell==0.0) return 0;
  if(fcell>0.0) return 0;
  if(fcell<0.0) return 0;
  return 1;
 \endcode
 * or (as suggested by Mark Line)
 \code
  return (fcell != fcell);
 \endcode
 *
 *  \param fcell
 *  \return int
 */

int G_is_f_null_value (const FCELL *fcellVal)
{
    int     i;	    /* counter */

    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Check if the FCELL value matches the null pattern */
    for(i = 0; i < sizeof (FCELL); i++)
    {
    	if(((unsigned char *) fcellVal)[i] != 
            ((unsigned char *) &fcellNullPattern)[i])
	{
            return FALSE;
	}
    }
    
    return TRUE;
}

/****************************************************************************
* 
* int G_is_d_null_value (DCELL *dcellVal)
*
* PURPOSE: 	To check if a DCELL raster value is set to NULL
* INPUT VARS:	dcellVal    =>	DCELL raster value to check
* RETURN VAL:	TRUE if DCELL raster value is NULL FALSE otherwise
*****************************************************************************/

/*!
 * \brief 
 *
 * Returns 1 if <em>dcell</em> is
 * NULL, 0 otherwise. This will test if the value <em>dcell</em> is a NaN. Same
 * test as in <tt>G_is_f_null_value()</tt>.
 *
 *  \param dcell
 *  \return int
 */

int G_is_d_null_value (const DCELL *dcellVal)
{
    int     i;	    /* counter */

    /* Check if the null patterns have been initialized */
    if (!initialized)
    {
    	InitError();
    }
    
    /* Check if the DCELL value matches the null pattern */
    for(i = 0; i < sizeof (DCELL); i++)
    {
    	if(((unsigned char *) dcellVal)[i] != 
            ((unsigned char *) &dcellNullPattern)[i])
	{
    	    return FALSE;
	}
    }
    
    return TRUE;
}

/****************************************************************************
* 
* int G_insert_null_values (void *rast, char *null_row, int ncols,
*   RASTER_MAP_TYPE data_type)
*
* PURPOSE: 	To insert null values into a map. Needs more.....
* INPUT VARS:	rast	    =>	??
*   	    	null_row    =>	??
*   	    	ncols	    =>	??
*   	    	data_type   =>	type of raster - CELL, FCELL, DCELL
* RETURN VAL:	??
*****************************************************************************/

/*!
 * \brief Insert NULL value
 *
 * If the <em>data_type</em> is
 * CELL_TYPE, calls G_insert_c_null_values ((CELL *) rast, flags, count);
 * If the <em>data_type</em> is FCELL_TYPE, calls G_insert_f_null_values
 * ((FCELL *) rast, flags, count);
 * If the <em>data_type</em> is DCELL_TYPE, calls G_insert_d_null_values
 * ((DCELL *) rast, flags, count);
 *
 *  \param rast
 *  \param flags
 *  \param count
 *  \param data_type
 *  \return int
 */

int G_insert_null_values (void *rast, char *null_row, int ncols,
    RASTER_MAP_TYPE data_type)
{
    return (EmbedGivenNulls(rast, null_row, data_type, ncols));
}

/****************************************************************************
* 
* int G_insert_c_null_values (CELL *cellVal, char *null_row, int ncols)
*
* PURPOSE: 	To insert null values into a CELL map. Needs more.....
* INPUT VARS:	cellVal	    =>	??
*   	    	null_row    =>	??
*   	    	ncols	    =>	??
* RETURN VAL:	??
*****************************************************************************/

/*!
 * \brief Insert CELL NULL value
 *
 * For each of the <em>count</em> <em>flags</em>
 * which is true(!=0), set the corresponding <em>cell</em> to the NULL value.
 *
 *  \param cell
 *  \param flags
 *  \param count
 *  \return int
 */

int G_insert_c_null_values (CELL *cellVal, char *null_row, int ncols)
{
    return (EmbedGivenNulls((void *) cellVal, null_row, CELL_TYPE, ncols));
}

/****************************************************************************
* 
* int G_insert_f_null_values (FCELL *fcellVal, char *null_row, int ncols)
*
* PURPOSE: 	To insert null values into a FCELL map. Needs more.....
* INPUT VARS:	fcellVal    =>	??
*   	    	null_row    =>	??
*   	    	ncols	    =>	??
* RETURN VAL:	??
*****************************************************************************/

/*!
 * \brief Insert FCELL NULL value
 *
 * For each of the <em>count</em> <em>flags</em>
 * which is true(!=0), set the corresponding <em>fcell</em> to the NULL value.
 *
 *  \param fcell
 *  \param flags
 *  \param count
 *  \return int
 */

int G_insert_f_null_values (FCELL *fcellVal, char *null_row, int ncols)
{
    return (EmbedGivenNulls((void *) fcellVal, null_row, FCELL_TYPE, ncols));
}

/****************************************************************************
* 
* int G_insert_d_null_values (DCELL *dcellVal, char *null_row, int ncols)
*
* PURPOSE: 	To insert null values into a DCELL map. Needs more.....
* INPUT VARS:	dcellVal    =>	??
*   	    	null_row    =>	??
*   	    	ncols	    =>	??
* RETURN VAL:	??
*****************************************************************************/

/*!
 * \brief Insert DCELL NULL value
 *
 * For each for the <em>count</em> <em>flag</em>
 * which is true(!=0), set the corresponding <em>dcell</em> to the NULL value.
 *
 *  \param dcell
 *  \param flags
 *  \param count
 *  \return int
 */

int G_insert_d_null_values (DCELL *dcellVal, char *null_row, int ncols)
{
    return (EmbedGivenNulls((void *) dcellVal, null_row, DCELL_TYPE, ncols));
}

/****************************************************************************
* int G__check_null_bit (unsigned char *flags, int bit_num, int n)
*
* PURPOSE: 	To...
* INPUT VARS:	flags	=>  ??
*   	    	bit_num =>  ??
*   	    	n   	=>  ??
* RETURN VAL:	??
*****************************************************************************/
int G__check_null_bit (const unsigned char *flags, int bit_num, int n)
{
    int ind;
    int	offset;

    /* find the index of the unsigned char in which this bit appears */
    ind = G__null_bitstream_size(bit_num + 1) - 1;

    /* find how many unsigned chars the buffer with bit_num+1 (counting from 0
      has and subtract 1 to get unsigned char index */
    if (ind > G__null_bitstream_size(n) - 1 )
    {
    	G_warning("G__check_null_bit: can't access index %d. Size of flags is %d (bit # is %d", ind, G__null_bitstream_size(n) - 1, bit_num);
      	return -1;
    }
   
    offset = (ind+1)*8 - bit_num - 1;

    return ((flags[ind] & ( (unsigned char) 1 << offset)) != 0);
}

/****************************************************************************
* int G__set_flags_from_01_random (char *zero_ones, unsigned char *flags, 
*   int col, int n, int ncols)
*
* PURPOSE: 	given array of 0/1 of length n starting from column col
*   	    	set the corresponding  bits of flags; total number of bits
*   	    	in flags is ncols
* INPUT VARS:	zero_ones   =>	??
*   	    	flags	    =>	??
*   	    	col 	    =>	??
*   	    	n   	    =>	??
*   	    	ncols	    =>	??
* RETURN VAL:	??
*****************************************************************************/
int G__set_flags_from_01_random (const char *zero_ones, unsigned char *flags, 
    int col, int n, int ncols)
{
    unsigned char   v;
    int      	    count;
    int	    	    size;
    int	    	    i, k;

    if (col==0 && n == ncols)
    {
    	G__convert_01_flags(zero_ones, flags, n);
        return 0;
    }
   
    count = 0;
    size = G__null_bitstream_size(ncols);
   
    for (i = 0; i < size; i++)
    {
    	v = 0;
      	k = 8;
      	
	while (k-- > 0)
      	{
	    if (count >= col && count < (col+ n))
	    {  
	    	v = v | ((unsigned char) zero_ones[count - col] << k);
	    }
            else if(count < ncols)
	    {
            	v = v | 
		    ((unsigned char) G__check_null_bit(flags, count, ncols) << k);
	    }
            
	    /* otherwise  keep this bit the same as it was */
            count++;
      	}
      	
	flags[i] = v;
    }
    
    return 1;
}

/****************************************************************************
* int G__convert_01_flags (char *zero_ones, unsigned char *flags, int n)
*
* PURPOSE: 	To...
* INPUT VARS:	zero_ones   =>	??
*   	    	flags	    =>	??
*   	    	n   	    =>	??
* RETURN VAL:	??
*****************************************************************************/
int G__convert_01_flags (const char *zero_ones, unsigned char *flags, int n)
{
    unsigned char   *v;
    int	    	    count;
    int	    	    size;
    int	    	    i, k;

    /* pad the flags with 0's to make size multiple of 8 */
    v = flags;
    size = G__null_bitstream_size(n);
    count = 0;
   
    for (i = 0; i < size; i++)
    {
      	*v = 0;
      	k = 8;
      	
	while (k-- > 0)
      	{
	    if (count < n)
	    {
	    	*v = *v | ((unsigned char) zero_ones[count] << k);
	    }
            
	    count++;
      	}
      
      	v++;
    }

    return 0;
}

/****************************************************************************
* int G__convert_flags_01 (char *zero_ones, unsigned char *flags, int n)
*
* PURPOSE: 	To...
* INPUT VARS:	zero_ones   =>	??
*   	    	flags	    =>	??
*   	    	n   	    =>	??
* RETURN VAL:	??
*****************************************************************************/
int G__convert_flags_01 (char *zero_ones, const unsigned char *flags, int n)
{
    const unsigned char *v;
    int      	    count;
    int	    	    size;
    int	    	    i, k;

    count = 0;
    v = flags;
    size = G__null_bitstream_size(n);
   
    for (i = 0; i < size; i++)
    {
      	k = 8;
      
      	while (k-- > 0)
      	{
	    if (count < n) 
	    {
	     	zero_ones[count] = ((*v & ( (unsigned char) 1 << k)) != 0);
             	count++;
	    }
      	}
      	
	v++;
    }

    return 0;
}

/****************************************************************************
* int G__init_null_bits (unsigned char *flags, int cols)
*
* PURPOSE: 	To...
* INPUT VARS:	flags	=>  ??
*   	    	cols	=>  ??
* RETURN VAL:	??
*****************************************************************************/
int G__init_null_bits (unsigned char *flags, int cols)
{
    unsigned char   *v;
    int     	    size;
    int     	    i;

    /* pad the flags with 0's to make size multiple of 8 */
    v = flags;
    size = G__null_bitstream_size(cols);
    
    for (i = 0; i < size; i++)
    {
      	if((i+1) * 8 <= cols)
	{
            *v = (unsigned char) 255;
	}
      	else
	{
            *v = (unsigned char) 255 << ((i+1) * 8 - cols);
	}
	
      	v++;
    }

    return 0;
}

