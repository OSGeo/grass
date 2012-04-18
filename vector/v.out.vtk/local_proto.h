
 /***************************************************************************
 *
 * MODULE:     v.out.vtk  
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************/

#ifndef __V_OUT_VTK_LOCAL_PROTO__
#define __V_OUT_VTK_LOCAL_PROTO__

/*global structs */
extern double x_extent;
extern double y_extent;

/*Write the vtk output */
int write_vtk(FILE * ascii, struct Map_info *, int layer, int *types,
	      int typenum, int dp, double scale, int numatts, int labels );
/*Write the VTK header */
int write_vtk_head(FILE * ascii, struct Map_info *Map);

#endif
