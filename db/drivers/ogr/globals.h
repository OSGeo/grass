
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*               Some updates by Martin Landa <landa.martin gmail.com>
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004-2009 by the GRASS Development Team
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
    OGRLayerH hLayer;		/* current OGR layer */
    OGRFeatureH hFeature;	/* current feature */
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *cols;			/* 1 type is known, 0 type is unknown */
    int ncols;			/* num columns (without fid column) */
} cursor;

/* column info (see execute.c) */
typedef struct
{
    char *name;
    char *value;
    int  index;
    int  qindex; /* query column */
    OGRFieldType type;
} column_info;

extern OGRDataSourceH hDs;
extern dbString *errMsg;
