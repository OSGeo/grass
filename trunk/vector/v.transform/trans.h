/*
 *  Written by the GRASS Team,  02/16/90, -mh .
 */

/******************
*  INCLUDES:       *
*******************/

#include  <stdio.h>

/******************
*  DEFINES:       *
*******************/

#define  TRANS_MATRIX 0
#define  TRANS_SHIFT  1

#define  IDX_XSHIFT 0
#define  IDX_YSHIFT 1
#define  IDX_ZSHIFT 2
#define  IDX_XSCALE 3
#define  IDX_YSCALE 4
#define  IDX_ZSCALE 5
#define  IDX_ZROT   6

/******************
*  GLOBALS:       *
*******************/

/**
* The coordinates of the points from the map that is to be converted
* are placed in ax[] and ay[].
* Those cooresponding points in the other coordinate system
* are placed in bx[], by[].
*
* The use[] contains a true if that point is to be used by the transform
* library or a false (0) if it is not to be used.
* The residuals each set of points contributes is placed in residuals[].
*
* Yah, I made them global.  So shoot me.
**/


/******************
*  STRUCTS:       *
*******************/

/*  For GRASS data files  */
struct file_info
{
    FILE *fp;
    char *mapset;
    char name[GPATH_MAX];
};
