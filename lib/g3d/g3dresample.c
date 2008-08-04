#include <stdio.h>
#include <grass/gis.h>
#include "G3d_intern.h"

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * The default resampling function which uses nearest
 * neighbor resampling.
 *
 *  \param map
 *  \param row
 *  \param col
 *  \param depth
 *  \param value
 *  \param type
 *  \return void
 */

void
G3d_nearestNeighbor(G3D_Map * map, int row, int col, int depth, void *value,
		    int type)
{

     /*AV*/
	/* BEGIN OF ORIGINAL CODE */
	/*
	   G3d_getValueRegion (map, row, col, depth, value, type);
	 */
	/* END OF ORIGINAL CODE */
	 /*AV*/
	/* BEGIN OF MY CODE */
	G3d_getValueRegion(map, col, row, depth, value, type);
    /* END OF MY CODE */


}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the resampling function to be used by
 * G3d_getValue () (cf.{g3d:G3d.getValue}). This function is defined
 * as follows:
 *
 *  \return void
 */

void G3d_setResamplingFun(G3D_Map * map, void (*resampleFun) ())
{
    map->resampleFun = resampleFun;
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * 
 * Returns in <em>resampleFun</em> a pointer to the resampling function used by
 * <em>map</em>.
 *
 *  \return void
 */

void G3d_getResamplingFun(G3D_Map * map, void (**resampleFun) ())
{
    *resampleFun = map->resampleFun;
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns
 * in <em>nnFunPtr</em> a pointer to G3d_nearestNeighbor () (cf.{g3d:G3d.nearestNeighbor}).
 *
 *  \return void
 */

void G3d_getNearestNeighborFunPtr(void (**nnFunPtr) ())
{
    *nnFunPtr = G3d_nearestNeighbor;
}
