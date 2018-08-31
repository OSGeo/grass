
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/config.h>
#include "writeascii.h"

#include "parameters.h"
#include "globaldefs.h"

paramType param;		/*Parameters */

double x_extent;
double y_extent;

/* ************************************************************************* */
/* MAIN ******************************************************************** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct Cell_head region;
    struct Cell_head default_region;
    FILE *fp = NULL;
    struct GModule *module;
    int i = 0, polytype = 0;
    char *null_value;
    int out_type;
    int fd;			/*Normale maps ;) */
    int rgbfd[3];
    int vectfd[3];
    int celltype[3] = { 0, 0, 0 };
    int headertype;
    double scale = 1.0, llscale = 1.0, eleval = 0.0;
    int digits = 12;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword("VTK");
    module->description = _("Converts raster maps into the VTK-ASCII format.");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

   if (param.input->answers == NULL && param.rgbmaps->answers == NULL &&
        param.vectmaps->answers == NULL) {
        G_fatal_error(_("No input maps specified. You need to specify at least one input map or three vector maps or three rgb maps."));
    }


    /*open the output */
    if (param.output->answer) {
	fp = fopen(param.output->answer, "w");
	if (fp == NULL) {
	    perror(param.output->answer);
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    }
    else
	fp = stdout;

    /*Correct the coordinates, so the precision of VTK is not hurt :( */
    if (param.coorcorr->answer) {
	/*Get the default region for coordiante correction */
	G_get_default_window(&default_region);

	/*Use the center of the current region as extent */
	y_extent = (default_region.north + default_region.south) / 2;
	x_extent = (default_region.west + default_region.east) / 2;
    }
    else {
	x_extent = 0;
	y_extent = 0;
    }

    /* Figure out the region from the map */
    G_get_window(&region);

    /*Set the null Value, maybe i have to check this? */
    null_value = param.null_val->answer;

    /*number of significant digits */
    sscanf(param.decimals->answer, "%i", &digits);

    /* read and compute the scale factor */
    sscanf(param.elevscale->answer, "%lf", &scale);
    sscanf(param.elev->answer, "%lf", &eleval);
    /*if LL projection, convert the elevation values to degrees */
    if (region.proj == PROJECTION_LL) {
	llscale = M_PI / (180) * 6378137;
	scale /= llscale;
    }

    /********************* WRITE ELEVATION *************************************/
    if (param.elevationmap->answer) {
	/*If the elevation is set, write the correct Header */
	if (param.usestruct->answer) {
	    write_vtk_structured_elevation_header(fp, region);
	}
	else {
	    write_vtk_polygonal_elevation_header(fp, region);
	}

	G_debug(3, _("Open Raster file %s"), param.elevationmap->answer);

	/* open raster map */
	fd = Rast_open_old(param.elevationmap->answer, "");

	out_type = Rast_get_map_type(fd);

	/*The write the Coordinates */
	if (param.usestruct->answer) {
	    write_vtk_structured_coordinates(fd, fp,
					     param.elevationmap->answer,
					     region, out_type, null_value,
					     scale, digits);
	}
	else {
	    polytype = QUADS;	/*The default */

	    if (param.usetriangle->answer)
		polytype = TRIANGLE_STRIPS;

	    if (param.usevertices->answer)
		polytype = VERTICES;

	    write_vtk_polygonal_coordinates(fd, fp,
					    param.elevationmap->answer,
					    region, out_type, null_value,
					    scale, polytype, digits);
	}
	Rast_close(fd);
    }
    else {
	/*Should pointdata or celldata be written */
	if (param.point->answer)
	    headertype = 1;
	else
	    headertype = 0;

	/*If no elevation is given, write the normal Header */
	if (param.origin->answer)
	    write_vtk_normal_header(fp, region, scale * eleval, headertype);
	else
	    write_vtk_normal_header(fp, region, eleval / llscale, headertype);
    }


  /******************** WRITE THE POINT OR CELL DATA HEADER ******************/
    if (param.input->answers != NULL || param.rgbmaps->answers != NULL) {
	if (param.point->answer || param.elevationmap->answer)
	    write_vtk_pointdata_header(fp, region);
	else
	    write_vtk_celldata_header(fp, region);
    }

  /********************** WRITE NORMAL DATA; CELL OR POINT *******************/
    /*Loop over all input maps! */
    if (param.input->answers != NULL) {

	for (i = 0; param.input->answers[i] != NULL; i++) {


	    G_debug(3, _("Open Raster file %s"), param.input->answers[i]);

	    /* open raster map */
	    fd = Rast_open_old(param.input->answers[i], "");
	    out_type = Rast_get_map_type(fd);
	    /*Now write the data */
	    write_vtk_data(fd, fp, param.input->answers[i], region, out_type,
			   null_value, digits);
	    Rast_close(fd);
	}
    }

  /********************** WRITE RGB IMAGE DATA; CELL OR POINT ****************/
    if (param.rgbmaps->answers != NULL) {
	if (param.rgbmaps->answers[0] != NULL &&
	    param.rgbmaps->answers[1] != NULL &&
	    param.rgbmaps->answers[2] != NULL) {


	    /*Loop over all three rgb input maps! */
	    for (i = 0; i < 3; i++) {
		G_debug(3, _("Open Raster file %s"),
			param.rgbmaps->answers[i]);

		/* open raster map */
		rgbfd[i] = Rast_open_old(param.rgbmaps->answers[i], "");
		celltype[i] = Rast_get_map_type(rgbfd[i]);
	    }

	    /*Maps have to be from the same type */
	    if (celltype[0] == celltype[1] && celltype[0] == celltype[2]) {
		G_debug(3, _("Writing VTK ImageData\n"));

		out_type = celltype[0];

		/*Now write the data */
		write_vtk_rgb_image_data(rgbfd[0], rgbfd[1], rgbfd[2], fp,
					 "RGB_Image", region, out_type,
					 digits);
	    }
	    else {
		G_warning(_("Wrong RGB maps. Maps should have the same type! RGB output not added!"));
		/*do nothing */
	    }

	    /*Close the maps */
	    for (i = 0; i < 3; i++)
		Rast_close(rgbfd[i]);
	}
    }

  /********************** WRITE VECTOR DATA; CELL OR POINT ****************/
    if (param.vectmaps->answers != NULL) {
	if (param.vectmaps->answers[0] != NULL &&
	    param.vectmaps->answers[1] != NULL &&
	    param.vectmaps->answers[2] != NULL) {


	    /*Loop over all three vect input maps! */
	    for (i = 0; i < 3; i++) {
		G_debug(3, _("Open Raster file %s"),
			param.vectmaps->answers[i]);

		/* open raster map */
		vectfd[i] = Rast_open_old(param.vectmaps->answers[i], "");
		celltype[i] = Rast_get_map_type(vectfd[i]);
	    }

	    /*Maps have to be from the same type */
	    if (celltype[0] == celltype[1] && celltype[0] == celltype[2]) {
		G_debug(3, _("Writing VTK Vector Data\n"));

		out_type = celltype[0];

		/*Now write the data */
		write_vtk_vector_data(vectfd[0], vectfd[1], vectfd[2], fp,
				      "Vector_Data", region, out_type,
				      digits);
	    }
	    else {
		G_warning(_("Wrong vector maps. Maps should have the same type! Vector output not added!"));
		/*do nothing */
	    }

	    /*Close the maps */
	    for (i = 0; i < 3; i++)
		Rast_close(vectfd[i]);
	}
    }

    if (param.output->answer && fp != NULL)
	if (fclose(fp)) {
	    G_fatal_error(_("Error closing VTK-ASCII file"));
	}

    return 0;
}
