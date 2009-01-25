
/*****************************************************************************/
/***                                                                       ***/
/***                             close_down()                              ***/
/***   	   Closes all input and output raster maps and frees memory.	   ***/
/***               Jo Wood, Project ASSIST, 7th February 1993              ***/
/***                                                                       ***/
/*****************************************************************************/

#include <string.h>
#include "param.h"


void close_down(void)
{
    struct History history;
    char map_title[RECORD_LEN], map_type[32];

    /* Close connection with existing input raster. */
    G_unopen_cell(fd_in);

    /* Write output raster map and close connection. */
    G_close_cell(fd_out);

    /* write out map metadata */
    G_short_history(rast_out_name, "raster", &history);

    strncpy(history.datsrc_1, rast_in_name, RECORD_LEN-1);
    history.datsrc_1[RECORD_LEN-1] = '\0';

    switch (mparam) {

    case ELEV:
	strcpy(map_type, "Generalised elevation value");
	break;

    case SLOPE:
	strcpy(map_type, "Magnitude of maximum gradient");
	G_write_raster_units(rast_out_name, "degrees");

	strcpy(history.edhist[0],
	    "Slope is given for steepest slope angle and measured in degrees.");
	history.edlinecnt = 1;
	break;

    case ASPECT:
	strcpy(map_type, "Direction of maximum gradient");
	G_write_raster_units(rast_out_name, "degrees");

	strcpy(history.edhist[0],
	    "Flow direction (aspect): W=0, E=180, N=+90, S=-90 degrees");
	history.edlinecnt = 1;
	break;

    case PROFC:
	strcpy(map_type, "Profile curvature");
	strcpy(history.edhist[0],
	    "Curvature intersecting with the plane defined by the Z axis and");
	strcpy(history.edhist[1],
	    "maximum gradient direction. Positive values describe convex profile");
	strcpy(history.edhist[2], "curvature, negative values concave profile.");
	history.edlinecnt = 3;
	break;

    case PLANC:
	strcpy(map_type, "Plan curvature");
	strcpy(history.edhist[0],
	    "Plan curvature is the horizontal curvature, intersecting with");
	strcpy(history.edhist[1],
	    "the XY plane.");
	history.edlinecnt = 2;
	break;

    case LONGC:
	strcpy(map_type, "Longitudinal curvature");
	strcpy(history.edhist[0],
	    "Longitudinal curvature is the profile curvature intersecting with the");
	strcpy(history.edhist[1],
	    "plane defined by the surfacenormal and maximum gradient direction.");
	history.edlinecnt = 2;
	break;

    case CROSC:
	strcpy(map_type, "Cross-sectional curvature");
	strcpy(history.edhist[0],
	    "Cross-sectional curvature is the tangential curvature intersecting");
	strcpy(history.edhist[1],
	    "with the plane defined by the surface normal and a tangent to the");
	strcpy(history.edhist[2],
	    "contour - perpendicular to maximum gradient direction.");
	history.edlinecnt = 3;
	break;

    case MINIC:
	strcpy(map_type, "Minimum curvature");
	strcpy(history.edhist[0],
	    "Measured in direction perpendicular to the direction of of maximum curvature.");
	history.edlinecnt = 1;
	break;

    case MAXIC:
	strcpy(map_type, "Maximum curvature");
	strcpy(history.edhist[0],
	    "The maximum curvature is measured in any direction");
	history.edlinecnt = 1;
	break;

    case FEATURE:
	strcpy(map_type, "Morphometric features");
	strcpy(history.edhist[0],
	    "Morphometric features: peaks, ridges, passes, channels, pits and planes");
	history.edlinecnt = 1;
	break;

    default:
	strcpy(map_type, "?");
	break;
    }

    G_command_history(&history);
    G_write_history(rast_out_name, &history);

    sprintf(map_title, "DEM terrain parameter: %s", map_type);
    G_put_cell_title(rast_out_name, map_title);

    return;
}
