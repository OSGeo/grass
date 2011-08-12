#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_is_null_value_num(const void *n, int type)
{
    if (type == FCELL_TYPE)
	return Rast_is_f_null_value(n);
    else
	return Rast_is_d_null_value(n);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Fills the vector pointed to by <em>c</em> with <em>nofElts</em> NULL-values
 * of <em>type</em>.
 *
 *  \param c
 *  \param nofElts
 *  \param type
 *  \return void
 */

void Rast3d_set_null_value(void *c, int nofElts, int type)
{
    if (type == FCELL_TYPE) {
	Rast_set_f_null_value((float *)c, nofElts);
	return;
    }

    Rast_set_d_null_value((double *)c, nofElts);
}
