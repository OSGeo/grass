
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

/* cursor */
typedef struct
{
    dbToken token;
    OGRLayerH hLayer;		/* results */
    OGRFeatureH hFeature;	/* current feature */
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *cols;			/* 1 type is known, 0 type is unknown */
    int ncols;			/* num columns */
} cursor;

#ifdef MAIN
OGRDataSourceH hDs;
dbString *errMsg = NULL;	/* error message */
#else
extern OGRDataSourceH hDs;
extern dbString *errMsg;
#endif
