#include <stdlib.h>
#include <string.h>
#include "global.h"

void add_point(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char handle[DXF_BUF_SIZE];	/* entity handle, 16 hexadecimal digits */
    char layer[DXF_BUF_SIZE];	/* layer name */
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */

    handle[0] = 0;
    strcpy(layer, UNIDENTIFIED_LAYER);

    zpnts[0] = 0.0;
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
	    xpnts[0] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 20:		/* y coordinate */
	    ypnts[0] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 30:		/* z coordinate */
	    zpnts[0] = atof(dxf_buf);
	    break;

	case 50:		/* angle of x axis for the UCS in effect */
	    break;
	}
    }

    if (xflag && yflag) {
	/* xpnts[1] = xpnts[0];
	 * ypnts[1] = ypnts[0];
	 * zpnts[1] = zpnts[0];
	 */
	write_vect(Map, layer, "POINT", handle, "", 1, GV_POINT);
    }

    return;
}
