
/* update_sliders:
 ** updates the text fields when the sliders have changed
 ** and also updates the sliders if the text fields have changed
 */


#include <stdlib.h>
#include "interface.h"

#ifdef OLD

/* use own system of getting vals from sliders to make it easier to
   change the slider's range dynamically */
int
init_default_slider_vals1(Nv_data * dc, float *min, float *max, float *val)
{
    float longdim;

    GS_get_longdim(&longdim);

    min[MAIN_PSP] = 3;
    max[MAIN_PSP] = 120;
    val[MAIN_PSP] = UNIT_OF(120, 3, 40);

    min[CPL_TILT] = 0.0;
    max[CPL_TILT] = 360.;
    val[CPL_TILT] = 0.5;

    min[CPL_ROT] = 0.0;
    max[CPL_ROT] = 360.;
    val[CPL_ROT] = 0.5;

    val[VECT_ZEX] = 0.1;
    val[LITE_BGT] = 0.8;
    val[LITE_RED] = 1.0;
    val[LITE_GRN] = 1.0;
    val[LITE_BLU] = 1.0;
    val[LITE_AMB] = 0.3;
    val[LITE_HGT] = 0.8;
    val[LITE_SHN] = 0.8;

    max[COL_RED] = max[COL_GRN] = max[COL_BLU] = 255;
    val[COL_RED] = 0.3;
    val[COL_GRN] = 0.3;
    val[COL_BLU] = 0.3;

    max[ATTR_CON] = 255;
    val[ATTR_CON] = 0.0;

    max[SITE_SIZ] = longdim / 20.;
    val[SITE_SIZ] = 0.2;
}
#endif

int Nget_first_exag_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			int argc, char **argv)
{
    float exag, texag;
    int nsurfs, i, *surf_list;
    char buf[128];

    surf_list = GS_get_surf_list(&nsurfs);

    exag = 0.0;
    for (i = 0; i < nsurfs; i++) {
	if (GS_get_exag_guess(surf_list[i], &texag) > -1) {
	    if (texag)
		exag = (texag > exag) ? texag : exag;
	}
    }

    if (exag == 0.0)
	exag = 1.0;
    sprintf(buf, "%f", exag);

    if (nsurfs)
	free(surf_list);

    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;

}

/* after initial data has been loaded, & maybe again later */
int Nget_height_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc, char **argv)
{
    float longdim, exag, texag, hmin, hmax;
    int nsurfs, i, *surf_list;
    char min[128];
    char max[128];
    char val[128];
    float fmin, fmax;
    char *list[4];

    surf_list = GS_get_surf_list(&nsurfs);
    if (nsurfs) {
	GS_get_longdim(&longdim);
	GS_get_zrange_nz(&hmin, &hmax);

	exag = 0.0;
	for (i = 0; i < nsurfs; i++) {
	    if (GS_get_exag_guess(surf_list[i], &texag) > -1)
		if (texag)
		    exag = texag > exag ? texag : exag;
	}
	if (exag == 0.0)
	    exag = 1.0;

	fmin = hmin - (2. * longdim / exag);
	fmax = hmin + (3 * longdim / exag);
    }
    else {
	fmax = 10000.0;
	fmin = 0.0;
    }

    /* The one decimal place of accuracy is necessary to force Tcl to */
    /* parse these values as floating point rather than integers.  This */
    /* avoids problems with integers which are too large to represent. */
    sprintf(min, "%.1f", fmin);
    sprintf(max, "%.1f", fmax);
    sprintf(val, "%.1f", fmin + (fmax - fmin) / 2.0);

    list[0] = val;
    list[1] = min;
    list[2] = max;
    list[3] = NULL;
    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;

    return TCL_OK;

}
