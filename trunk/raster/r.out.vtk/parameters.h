
/****************************************************************************
*
* MODULE:       r.out.vtk  
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert@gmx.de
* 		08 23 2005 Berlin
* PURPOSE:      Converts raster maps into the VTK-Ascii format  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#ifndef __R_OUT_VTK_PARAMETERS_H__
#define __R_OUT_VTK_PARAMETERS_H__
struct Option;
struct Flag;

typedef struct
{
    struct Option *input, *output, *elevationmap, *null_val, *elevscale,
	*elev, *rgbmaps, *vectmaps, *decimals;
    struct Flag *usestruct, *usetriangle, *usevertices, *origin, *point, *coorcorr;	/*struct Flag *mask;          struct Flag *xml; *//*maybe xml support in the future */
} paramType;

/*global structs */

extern paramType param;		/*Parameters */


/*prototype */
void set_params(void);

#endif
