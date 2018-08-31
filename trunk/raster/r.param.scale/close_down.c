
/*****************************************************************************/
/***                                                                       ***/
/***                             close_down()                              ***/
/***   	   Closes all input and output raster maps and frees memory.	   ***/
/***               Jo Wood, Project ASSIST, 7th February 1993              ***/
/***                                                                       ***/
/*****************************************************************************/

#include <string.h>
#include <grass/raster.h>
#include "param.h"


void close_down(void)
{
    struct History history;
    char map_title[80], map_type[32];

    /* Close connection with existing input raster. */
    Rast_unopen(fd_in);

    /* Write output raster map and close connection. */
    Rast_close(fd_out);

    /* write out map metadata */
    Rast_short_history(rast_out_name, "raster", &history);

    Rast_set_history(&history, HIST_DATSRC_1, rast_in_name);

    switch (mparam) {

    case ELEV:
	strcpy(map_type, "Generalised elevation value");
	break;

    case SLOPE:
	strcpy(map_type, "Magnitude of maximum gradient");
	Rast_write_units(rast_out_name, "degrees");

	Rast_append_history(
	    &history,
	    "Slope is given for steepest slope angle and measured in degrees.");
	break;

    case ASPECT:
	strcpy(map_type, "Direction of maximum gradient");
	Rast_write_units(rast_out_name, "degrees");

	Rast_append_history(
	    &history,
	    "Flow direction (aspect): W=0, E=180, N=+90, S=-90 degrees");
	break;

    case PROFC:
	strcpy(map_type, "Profile curvature");
	Rast_append_history(
	    &history,
	    "Curvature intersecting with the plane defined by the Z axis and");
	Rast_append_history(
	    &history,
	    "maximum gradient direction. Positive values describe convex profile");
	Rast_append_history(
	    &history,
	    "curvature, negative values concave profile.");
	break;

    case PLANC:
	strcpy(map_type, "Plan curvature");
	Rast_append_history(
	    &history,
	    "Plan curvature is the horizontal curvature, intersecting with");
	Rast_append_history(
	    &history,
	    "the XY plane.");
	break;

    case LONGC:
	strcpy(map_type, "Longitudinal curvature");
	Rast_append_history(
	    &history,
	    "Longitudinal curvature is the profile curvature intersecting with the");
	Rast_append_history(
	    &history,
	    "plane defined by the surfacenormal and maximum gradient direction.");
	break;

    case CROSC:
	strcpy(map_type, "Cross-sectional curvature");
	Rast_append_history(
	    &history,
	    "Cross-sectional curvature is the tangential curvature intersecting");
	Rast_append_history(
	    &history,
	    "with the plane defined by the surface normal and a tangent to the");
	Rast_append_history(
	    &history,
	    "contour - perpendicular to maximum gradient direction.");
	break;

    case MINIC:
	strcpy(map_type, "Minimum curvature");
	Rast_append_history(
	    &history,
	    "Measured in direction perpendicular to the direction of of maximum curvature.");
	break;

    case MAXIC:
	strcpy(map_type, "Maximum curvature");
	Rast_append_history(
	    &history,
	    "The maximum curvature is measured in any direction");
	break;

    case FEATURE:
	strcpy(map_type, "Morphometric features");
	Rast_append_history(
	    &history,
	    "Morphometric features: peaks, ridges, passes, channels, pits and planes");
	break;

    default:
	strcpy(map_type, "?");
	break;
    }

    Rast_command_history(&history);
    Rast_write_history(rast_out_name, &history);

    sprintf(map_title, "DEM terrain parameter: %s", map_type);
    Rast_put_cell_title(rast_out_name, map_title);

    return;
}
