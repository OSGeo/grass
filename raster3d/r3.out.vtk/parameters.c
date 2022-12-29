
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
#include <grass/gis.h>
#include <grass/glocale.h>
#include "parameters.h"

/* ************************************************************************* */
/* Setg up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.input = G_define_standard_option(G_OPT_R3_INPUTS);
    param.input->required = NO;
    param.input->description =
	_("3D raster map(s) to be converted to VTK-ASCII data format");

    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->required = NO;
    param.output->description = _("Name for VTK-ASCII output file");

    param.null_val = G_define_option();
    param.null_val->key = "null";
    param.null_val->type = TYPE_DOUBLE;
    param.null_val->required = NO;
    param.null_val->description =
	_("Float value to represent no data cell/points");
    param.null_val->answer = "-99999.99";

    param.point = G_define_flag();
    param.point->key = 'p';
    param.point->description =
	_("Create VTK pointdata instead of VTK celldata (celldata is default)");

    param.top = G_define_option();
    param.top->key = "top";
    param.top->type = TYPE_STRING;
    param.top->required = NO;
    param.top->gisprompt = "old,cell,raster";
    param.top->multiple = NO;
    param.top->guisection = "Surface options";
    param.top->description = _("Top surface 2D raster map");

    param.bottom = G_define_option();
    param.bottom->key = "bottom";
    param.bottom->type = TYPE_STRING;
    param.bottom->required = NO;
    param.bottom->gisprompt = "old,cell,raster";
    param.bottom->multiple = NO;
    param.bottom->guisection = "Surface options";
    param.bottom->description = _("Bottom surface 2D raster map");

    param.structgrid = G_define_flag();
    param.structgrid->key = 's';
    param.structgrid->guisection = "Surface options";
    param.structgrid->description =
	_("Create 3D elevation output with a top and a bottom surface, both raster maps are required.");


    param.rgbmaps = G_define_standard_option(G_OPT_R3_INPUT);
    param.rgbmaps->key = "rgbmaps";
    param.rgbmaps->required = NO;
    param.rgbmaps->multiple = YES;
    param.rgbmaps->guisection = "Advanced options";
    param.rgbmaps->description =
	_("Three (R,G,B) 3D raster maps to create RGB values [redmap,greenmap,bluemap]");

    param.vectormaps = G_define_standard_option(G_OPT_V_OUTPUT);
    param.vectormaps->key = "vectormaps";
    param.vectormaps->required = NO;
    param.vectormaps->multiple = YES;
    param.vectormaps->guisection = "Advanced options";
    param.vectormaps->description =
	_("Three (x,y,z) 3D raster maps to create vector values [xmap,ymap,zmap]");

    param.elevscale = G_define_option();
    param.elevscale->key = "zscale";
    param.elevscale->type = TYPE_DOUBLE;
    param.elevscale->required = NO;
    param.elevscale->description = _("Scale factor for elevation");
    param.elevscale->guisection = "Advanced options";
    param.elevscale->answer = "1.0";

    param.decimals = G_define_option();
    param.decimals->key = "precision";
    param.decimals->type = TYPE_INTEGER;
    param.decimals->required = NO;
    param.decimals->multiple = NO;
    param.decimals->answer = "12";
    param.decimals->options = "0-20";
    param.decimals->guisection = "Advanced options";
    param.decimals->description =
	_("Number of significant digits (floating point only)");

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->guisection = "Advanced options";
    param.mask->description = _("Use 3D raster mask (if exists) with input maps");

    param.origin = G_define_flag();
    param.origin->key = 'o';
    param.origin->guisection = "Advanced options";
    param.origin->description = _("Scale factor affects the origin");

    param.coorcorr = G_define_flag();
    param.coorcorr->key = 'c';
    param.coorcorr->guisection = "Advanced options";
    param.coorcorr->description =
	_("Correct the coordinates to match the VTK-OpenGL precision");

    param.scalell = G_define_flag();
    param.scalell->key = 'l';
    param.scalell->guisection = "Advanced options";
    param.scalell->description =
	_("Do not convert the top-bottom resolution in case of lat long projection to meters");


    /* Maybe needed in the future
     * param.xml = G_define_flag ();
     * param.xml->key = 'x';
     * param.xml->description = "Write XML-VTK-format";
     */

    return;
}
