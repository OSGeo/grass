
/****************************************************************************
*
* MODULE:       r3.out.vtk  
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert at gmx de
* 		27 Feb 2006 Berlin
* PURPOSE:      Converts 3D raster maps (RASTER3D) into the VTK-Ascii format  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#ifndef __R3_OUT_VTK_WRITE_HEAD_H__
#define __R3_OUT_VTK_WRITE_HEAD_H__

/*write the vtk structured point header */
void write_vtk_structured_point_header(FILE * fp, char *vtkFile,
				       RASTER3D_Region region, int dp,
				       double scale);
/*write the vtk structured grid header */
void write_vtk_structured_grid_header(FILE * fp, char *vtkFile,
				      RASTER3D_Region region);
/*write the vtk unstructured grid header */
void write_vtk_unstructured_grid_header(FILE * fp, char *vtkFile,
					RASTER3D_Region region);

#endif
