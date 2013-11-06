
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
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/config.h>
#include "parameters.h"



/* ************************************************************************* */
/* PARAMETERS ************************************************************** */
/* ************************************************************************* */

void set_params()
{
    param.input = G_define_standard_option(G_OPT_R_INPUTS);
    param.input->required = NO;
    param.input->description =
        _("Raster map(s) to be converted to VTK-ASCII data format");

    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->required = NO;
    param.output->description = _("Name for VTK-ASCII output file");

    param.elevationmap = G_define_standard_option(G_OPT_R_ELEV);
    param.elevationmap->required = NO;
    
    param.null_val = G_define_option();
    param.null_val->key = "null";
    param.null_val->type = TYPE_DOUBLE;
    param.null_val->required = NO;
    param.null_val->description = _("Value to represent no data cell");
    param.null_val->answer = "-99999.99";

    param.elev = G_define_option();
    param.elev->key = "elevation2d";
    param.elev->type = TYPE_DOUBLE;
    param.elev->required = NO;
    param.elev->description =
	_("Elevation (if no elevation map is specified)");
    param.elev->answer = "0.0";

    param.point = G_define_flag();
    param.point->key = 'p';
    param.point->description =
	_("Create VTK point data instead of VTK cell data (if no elevation map is given)");

    param.rgbmaps = G_define_option();
    param.rgbmaps->key = "rgbmaps";
    param.rgbmaps->type = TYPE_STRING;
    param.rgbmaps->required = NO;
    param.rgbmaps->gisprompt = "old,cell,raster";
    param.rgbmaps->multiple = YES;
    param.rgbmaps->guisection = "Advanced options";
    param.rgbmaps->description =
	_("Three (r,g,b) raster maps to create rgb values [redmap,greenmap,bluemap]");

    param.vectmaps = G_define_option();
    param.vectmaps->key = "vectormaps";
    param.vectmaps->type = TYPE_STRING;
    param.vectmaps->required = NO;
    param.vectmaps->gisprompt = "old,cell,raster";
    param.vectmaps->multiple = YES;
    param.vectmaps->guisection = "Advanced options";
    param.vectmaps->description =
	_("Three (x,y,z) raster maps to create vector values [xmap,ymap,zmap]");

    param.elevscale = G_define_option();
    param.elevscale->key = "elevscale";
    param.elevscale->type = TYPE_DOUBLE;
    param.elevscale->required = NO;
    param.elevscale->guisection = "Advanced options";
    param.elevscale->description = _("Scale factor for elevation");
    param.elevscale->answer = "1.0";

    param.decimals = G_define_option();
    param.decimals->key = "dp";
    param.decimals->type = TYPE_INTEGER;
    param.decimals->required = NO;
    param.decimals->multiple = NO;
    param.decimals->answer = "12";
    param.decimals->options = "0-20";
    param.decimals->guisection = "Advanced options";
    param.decimals->description =
	_("Number of significant digits (floating point only)");

    param.usestruct = G_define_flag();
    param.usestruct->key = 's';
    param.usestruct->guisection = "Advanced options";
    param.usestruct->description =
	_("Use structured grid for elevation (not recommended)");

    param.usetriangle = G_define_flag();
    param.usetriangle->key = 't';
    param.usetriangle->guisection = "Advanced options";
    param.usetriangle->description =
	_("Use polydata-trianglestrips for elevation grid creation");

    param.usevertices = G_define_flag();
    param.usevertices->key = 'v';
    param.usevertices->guisection = "Advanced options";
    param.usevertices->description =
	_("Use polydata-vertices for elevation grid creation (to use with vtkDelauny2D)");

    param.origin = G_define_flag();
    param.origin->key = 'o';
    param.origin->guisection = "Advanced options";
    param.origin->description =
	_("Scale factor effects the origin (if no elevation map is given)");

    param.coorcorr = G_define_flag();
    param.coorcorr->key = 'c';
    param.coorcorr->guisection = "Advanced options";
    param.coorcorr->description =
	_("Correct the coordinates to fit the VTK-OpenGL precision");


    /* 
     * param.mask = G_define_flag ();
     * param.mask->key = 'm';
     * param.mask->description = _("Use mask (if exists) with input maps");
     * 
     * Maybe needed in the future
     * param.xml = G_define_flag ();
     * param.xml->key = 'x';
     * param.xml->description = "Write XML-VTK-format";
     */
}
