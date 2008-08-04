#include <stdlib.h>
#include <string.h>
#include "global.h"

int add_lwpolyline(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char layer[DXF_BUF_SIZE];
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int polyline_flag = 0;	/* indicates the type of polyline */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */
    int arr_size = 0;

    /* variables to create arcs */
    double bulge = 0.0;		/* for arc curves */
    double prev_bulge = 0.0;	/* for arc curves */

    strcpy(layer, UNIDENTIFIED_LAYER);

    zpnts[0] = 0.0;
    /* read in lines and processes information until a 0 is read in */
    while ((code = dxf_get_code(dxf)) != 0) {
	if (code == -2)
	    return -1;

	switch (code) {
	case 8:		/* layer name */
	    if (!layer_flag && *dxf_buf) {
		if (flag_list) {
		    if (!is_layer_in_list(dxf_buf)) {
			add_layer_to_list(dxf_buf);
			print_layer(dxf_buf);
		    }
		    return 0;
		}
		/* skip if layers != NULL && (
		 * (flag_invert == 0 && is_layer_in_list == 0) ||
		 * (flag_invert == 1 && is_layer_in_list == 1)
		 * )
		 */
		if (layers && flag_invert == is_layer_in_list(dxf_buf))
		    return 0;
		strcpy(layer, dxf_buf);
		layer_flag = 1;
	    }
	    break;
	case 10:		/* x coordinate */
	    xpnts[arr_size] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 20:		/* y coordinate */
	    ypnts[arr_size] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 42:		/* bulge */
	    bulge = atof(dxf_buf);
	    break;
	case 70:		/* polyline flag */

	    /*******************************************************************
	         Bit         Meaning
	           1         closed
	         128         plinegen
	     ******************************************************************/
	    polyline_flag = atoi(dxf_buf);
	    break;

	case 40:		/* starting width */
	case 41:		/* ending width */
	    break;
	}

	if (xflag && yflag) {
	    arr_size = make_arc_from_polyline(arr_size, bulge, prev_bulge);
	    prev_bulge = bulge;
	    bulge = 0.0;

	    xflag = 0;
	    yflag = 0;
	}
    }

    if (polyline_flag & 1) {
	if (xpnts[0] != xpnts[arr_size - 1] ||
	    ypnts[0] != ypnts[arr_size - 1]) {
	    /* close polyline */
	    xpnts[arr_size] = xpnts[0];
	    ypnts[arr_size] = ypnts[0];
	    zpnts[arr_size] = zpnts[0];

	    /* arr_size incremented to be consistent with polyline_flag != 1 */
	    if (arr_size >= ARR_MAX - 1) {
		ARR_MAX += ARR_INCR;
		xpnts = (double *)G_realloc(xpnts, ARR_MAX * sizeof(double));
		ypnts = (double *)G_realloc(ypnts, ARR_MAX * sizeof(double));
		zpnts = (double *)G_realloc(zpnts, ARR_MAX * sizeof(double));
	    }
	    arr_size++;
	}
    }

    {
	int i;

	for (i = 0; i < arr_size; i++)
	    zpnts[i] = 0.0;
    }

    write_line(Map, layer, arr_size);

    return 0;
}
