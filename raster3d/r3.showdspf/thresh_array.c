#include <stdio.h>
#include "vizual.h"
/*
 **  return an array of struct cmndln_info of resulting
 **   thresholds based on  in_out flag
 */


int build_thresh_arrays(D_spec, headp)
     struct dspec *D_spec;
     file_info *headp;
{
    double min_thresh, max_thresh;
    int a, b, i;

    min_thresh = headp->linefax.tvalue[D_spec->low];
    max_thresh = headp->linefax.tvalue[D_spec->hi];

    /* initializations */
    D_spec->threshes[0].nthres = 0;
    D_spec->threshes[1].nthres = 0;	/* for INSIDE CASE */

    if (D_spec->in_out == INSIDE) {
	b = 0;
	for (a = 0; a < headp->linefax.nthres; a++) {
	    if (min_thresh <= headp->linefax.tvalue[a] &&
		max_thresh >= headp->linefax.tvalue[a]) {
		D_spec->threshes[0].tvalue[b++] = headp->linefax.tvalue[a];
		D_spec->threshes[0].nthres++;
	    }
	}

    }
    else {			/* OUTSIDE */

	for (i = 0; i < 2; i++) {
	    b = 0;
	    for (a = 0; a < headp->linefax.nthres; a++) {
		if (!i) {
		    if (min_thresh >= headp->linefax.tvalue[a]) {
			D_spec->threshes[i].tvalue[b++] =
			    headp->linefax.tvalue[a];
			D_spec->threshes[i].nthres++;
		    }
		    else if (max_thresh <= headp->linefax.tvalue[a]) {
			D_spec->threshes[i].tvalue[b++] =
			    headp->linefax.tvalue[a];
			D_spec->threshes[i].nthres++;
		    }
		}		/* is this brace correct? MN 2001 */
	    }
	}
    }
}
