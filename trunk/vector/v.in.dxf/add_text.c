#include <stdlib.h>
#include <string.h>
#include "global.h"

void add_text(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char handle[DXF_BUF_SIZE];	/* entity handle, 16 hexadecimal digits */
    char layer[DXF_BUF_SIZE];	/* layer name */
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */
    double height = 1.0;	/* read in from dxf file */
    double angle = 0.0;		/* read in from dxf file */
    char label[DXF_BUF_SIZE];	/* read in from dxf file */
    int label_len = 0;

    handle[0] = 0;
    strcpy(layer, UNIDENTIFIED_LAYER);

    zpnts[0] = 0.0;
    /* read in lines and process information until a 0 is read in */
    while ((code = dxf_get_code(dxf)) != 0) {
	if (code == -2)
	    return;

	switch (code) {
	case 1:		/* label value */
	    label_len = strlen(dxf_buf);
	    strcpy(label, dxf_buf);
	    break;
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
	case 40:		/* text height */
	    height = atof(dxf_buf);
	    break;
	case 50:		/* text angle */
	    angle = atof(dxf_buf);
	    break;

	case 7:		/* text style name */
	case 11:		/* alignment point */
	case 21:		/* alignment point */
	case 31:		/* alignment point */
	case 41:		/* relative x scale factor */
	case 51:		/* oblique angle */
	case 71:		/* text generation flag */
	case 72:		/* horizontal text justification type */
	    break;
	}
    }

    if (label_len == 0)
	return;

    if (xflag && yflag) {
	/* TODO */
#if 0
	double theta, length, diag, base1, base2;

	/* now build the points of the box */
	theta = angle * M_PI / 180.;
	length = (label_len - 1) * height;

	/* base angles for polar description of rectangle */
	base1 = M_PI / 2.;
	base2 = atan2(1., (double)(label_len - 1));
	diag = hypot(length, height);

	xpnts[4] = xpnts[0];
	ypnts[4] = ypnts[0];
	zpnts[4] = zpnts[0];

	xpnts[1] = xpnts[0] + (height * cos(theta + base1));
	ypnts[1] = ypnts[0] + (height * sin(theta + base1));
	zpnts[1] = zpnts[0];

	xpnts[2] = xpnts[0] + (diag * cos(theta + base2));
	ypnts[2] = ypnts[0] + (diag * sin(theta + base2));
	zpnts[2] = zpnts[0];

	xpnts[3] = xpnts[0] + (length * cos(theta));
	ypnts[3] = ypnts[0] + (length * sin(theta));
	zpnts[3] = zpnts[0];

	write_vect(Map, layer, "TEXT", handle, "", 5, GV_LINE);
#endif
	write_vect(Map, layer, "TEXT", handle, label, 1, GV_POINT);
    }

    return;
}
