
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

#ifndef __R_OUT_VTK_WRITEASCII_H__
#define __R_OUT_VTK_WRITEASCII_H__

struct Cell_head;		/*Definition needed here */

void write_vtk_normal_header(FILE * fp, struct Cell_head region,
			     double elevation, int type);
void write_vtk_structured_elevation_header(FILE * fp,
					   struct Cell_head region);
void write_vtk_polygonal_elevation_header(FILE * fp, struct Cell_head region);
void write_vtk_celldata_header(FILE * fp, struct Cell_head region);
void write_vtk_pointdata_header(FILE * fp, struct Cell_head region);
void write_vtk_data(int fd, FILE * fp, char *varname, struct Cell_head region,
		    int out_type, char *null_value, int dp);
void write_vtk_rgb_image_data(int redfd, int greenfd, int bluefd, FILE * fp,
			      const char *varname, struct Cell_head region,
			      int out_type, int dp);
void write_vtk_vector_data(int xfd, int yfd, int zfd, FILE * fp,
			   const char *varname, struct Cell_head region,
			   int out_type, int dp);
void write_vtk_structured_coordinates(int fd, FILE * fp, char *varname,
				      struct Cell_head region, int out_type,
				      char *null_value, double scale, int dp);
void write_vtk_polygonal_coordinates(int fd, FILE * fp, char *varname,
				     struct Cell_head region, int out_type,
				     char *null_value, double scale,
				     int polytype, int dp);
#endif
