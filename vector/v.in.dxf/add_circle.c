#include <stdlib.h>
#include <string.h>
#include "global.h"

void add_circle(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char handle[DXF_BUF_SIZE];	/* entity handle, 16 hexadecimal digits */
    char layer[DXF_BUF_SIZE];	/* layer name */
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */
    int rflag = 0;		/* indicates if a radius has been found */
    double centerx = 0;		/* read in from dxf file */
    double centery = 0;		/* read in from dxf file */
    double radius = 0;		/* read in from dxf file */
    double zcoor = 0;		/* read in from dxf file */
    int arr_size = 0;

    handle[0] = 0;
    strcpy(layer, UNIDENTIFIED_LAYER);

    /* read in lines and process information until a 0 is read in */
    while ((code = dxf_get_code(dxf)) != 0) {
	if (code == -2)
	    return;

	switch (code) {
	case 5:		/* entity handle */
	    strcpy(handle, dxf_buf);
	    break;
	case 8:		/* layer name */
	    if (!layer_flag && *dxf_buf) {
		if (flag_list) {
		    if (!is_layer_in_list(dxf_buf))
			add_layer_to_list(dxf_buf, 1);
		    return;
		}
		/* skip if (opt_layers != NULL && (
		 * (flag_invert == 0 && is_layer_in_list == 0) ||
		 * (flag_invert == 1 && is_layer_in_list == 1)
		 * )
		 */
		if (opt_layers && flag_invert == is_layer_in_list(dxf_buf))
		    return;
		strcpy(layer, dxf_buf);
		layer_flag = 1;
	    }
	    break;
	case 10:		/* x coordinate */
	    centerx = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 20:		/* y coordinate */
	    centery = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 30:		/* z coordinate */
	    zcoor = atof(dxf_buf);
	    break;
	case 40:		/* radius */
	    radius = atof(dxf_buf);
	    rflag = 1;
	    break;
	}
    }

    if (xflag && yflag && rflag) {
	arr_size = make_arc(0, centerx, centery, radius, 0.0, 360.0, zcoor);
	write_vect(Map, layer, "CIRCLE", handle, "", arr_size, GV_LINE);
    }

    return;
}
