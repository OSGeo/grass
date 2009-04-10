#include <stdlib.h>
#include <string.h>
#include "global.h"

void add_3dface(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char handle[DXF_BUF_SIZE];	/* entity handle, 16 hexadecimal digits */
    char layer[DXF_BUF_SIZE];	/* layer name */
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int dface_flag = 0;		/* indicates if a edge is invisible */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */
    int zflag = 0;		/* indicates if a z value has been found */
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
	case 10:		/* first corner x coordinate */
	    xpnts[arr_size] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 11:		/* second corner x coordinate */
	    xpnts[arr_size] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 12:		/* third corner x coordinate */
	    xpnts[arr_size] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 13:		/* fourth corner x coordinate */
	    xpnts[arr_size] = atof(dxf_buf);
	    xflag = 1;
	    break;
	case 20:		/* first corner y coordinate */
	    ypnts[arr_size] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 21:		/* second corner y coordinate */
	    ypnts[arr_size] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 22:		/* third corner y coordinate */
	    ypnts[arr_size] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 23:		/* fourth corner y coordinate */
	    ypnts[arr_size] = atof(dxf_buf);
	    yflag = 1;
	    break;
	case 30:		/* first corner z coordinate */
	    zpnts[arr_size] = atof(dxf_buf);
	    zflag = 1;
	    break;
	case 31:		/* second corner z coordinate */
	    zpnts[arr_size] = atof(dxf_buf);
	    zflag = 1;
	    break;
	case 32:		/* third corner z coordinate */
	    zpnts[arr_size] = atof(dxf_buf);
	    zflag = 1;
	    break;
	case 33:		/* fourth corner z coordinate */
	    zpnts[arr_size] = atof(dxf_buf);
	    zflag = 1;
	    break;
	case 70:		/* 3dface flag */
	    dface_flag = atoi(dxf_buf);
	    /* TODO: what does 'invisible' mean here? */

	    /*******************************************************************
	     Invisible edge flags (optional; default = 0):
	     1 = First edge is invisible
	     2 = Second edge is invisible
	     4 = Third edge is invisible
	     8 = Fourth edge is invisible
	     ******************************************************************/
	    break;
	}

	/* read in first four points */
	if (xflag && yflag && zflag && arr_size < 4) {
	    arr_size++;
	    xflag = 0;
	    yflag = 0;
	    zflag = 0;
	}
    }

    if (arr_size == 4) {
	/* third corner == fourth corner */
	if (xpnts[2] == xpnts[3] && ypnts[2] == ypnts[3] &&
	    zpnts[2] == zpnts[3])
	    arr_size--;
	write_vect(Map, layer, "3DFACE", handle, "", arr_size, GV_FACE);
    }

    return;
}
