#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"

int natb;

void initialize(void)
{
    int i, j;

    natb = 0;
    for (i = 0; i < window.rows; i++) {
	for (j = 0; j < window.cols; j++) {
	    av(i, j) = window.ns_res * window.ew_res;
	    if (IScvNULL(i, j)) {
		natb++;
		Rast_set_d_null_value(&atbv(i, j), 1);
	    }
	    else {
		atbv(i, j) = -10.0;
	    }
	}
    }
}

void calculate_atanb(void)
{
    int i, j, k, snatb;
    int iter, ncells, nroute, nslp;
    double sum, route[9], tanB[9], dx, dx1, dx2, sumtb, C;
    int nsink;

    dx = window.ew_res;
    dx1 = 1 / dx;
    dx2 = 1 / (1.414 * dx);
    ncells = window.rows * window.cols;
    snatb = natb;

    G_important_message(_("Calculating..."));

    nsink = 0;
    for (iter = 1; natb < ncells; iter++) {
	/*
	   for(i=0;i<80;i++)
	   fprintf(stderr,"\b");
	   fprintf(stderr,"Iteration: %d",iter);
	 */
	G_percent(natb - snatb, ncells - snatb, 1);
	for (i = 0; i < window.rows; i++) {
	    for (j = 0; j < window.cols; j++) {
		/* skip null values */
		if (IScvNULL(i, j))
		    continue;

		/* skip squares already done */
		if (ISatbvNULL(i, j) || atbv(i, j) >= ZERO)
		    continue;

		/* check the 8 possible flow directions for
		 * upslope elements without an atb value
		 */
		if (i > 0) {
		    if (j > 0 &&
			(IScvNULL(i - 1, j - 1) ||
			 cv(i - 1, j - 1) > cv(i, j)) &&
			!ISatbvNULL(i - 1, j - 1) &&
			atbv(i - 1, j - 1) < ZERO)
			continue;

		    if ((IScvNULL(i - 1, j) ||
			 cv(i - 1, j) > cv(i, j)) &&
			!ISatbvNULL(i - 1, j) && atbv(i - 1, j) < ZERO)
			continue;

		    if (j + 1 < window.cols &&
			(IScvNULL(i - 1, j + 1) ||
			 cv(i - 1, j + 1) > cv(i, j)) &&
			!ISatbvNULL(i - 1, j + 1) &&
			atbv(i - 1, j + 1) < ZERO)
			continue;
		}
		if (j > 0 &&
		    (IScvNULL(i, j - 1) ||
		     cv(i, j - 1) > cv(i, j)) &&
		    !ISatbvNULL(i, j - 1) && atbv(i, j - 1) < ZERO)
		    continue;
		if (j + 1 < window.cols &&
		    (IScvNULL(i, j + 1) ||
		     cv(i, j + 1) > cv(i, j)) &&
		    !ISatbvNULL(i, j + 1) && atbv(i, j + 1) < ZERO)
		    continue;
		if (i + 1 < window.rows) {
		    if (j > 0 &&
			(IScvNULL(i + 1, j - 1) ||
			 cv(i + 1, j - 1) > cv(i, j)) &&
			!ISatbvNULL(i + 1, j - 1) &&
			atbv(i + 1, j - 1) < ZERO)
			continue;
		    if ((IScvNULL(i + 1, j) ||
			 cv(i + 1, j) > cv(i, j)) &&
			!ISatbvNULL(i + 1, j) && atbv(i + 1, j) < ZERO)
			continue;
		    if (j + 1 < window.cols &&
			(IScvNULL(i + 1, j + 1) ||
			 cv(i + 1, j + 1) > cv(i, j)) &&
			!ISatbvNULL(i + 1, j + 1) &&
			atbv(i + 1, j + 1) < ZERO)
			continue;
		}
		/* find the outflow directions and calculate 
		 * the sum of weights
		 */
		sum = 0.0;
		for (k = 0; k < 9; k++)
		    route[k] = 0.0;
		nroute = 0;
		if (i > 0) {
		    if (j > 0 &&
			!IScvNULL(i - 1, j - 1) &&
			cv(i, j) - cv(i - 1, j - 1) > ZERO) {
			tanB[0] = (cv(i, j) - cv(i - 1, j - 1)) * dx2;
			route[0] = 0.354 * dx * tanB[0];
			sum += route[0];
			nroute++;
		    }
		    if (!IScvNULL(i - 1, j) && cv(i, j) - cv(i - 1, j) > ZERO) {
			tanB[1] = (cv(i, j) - cv(i - 1, j)) * dx1;
			route[1] = 0.5 * dx * tanB[1];
			sum += route[1];
			nroute++;
		    }
		    if (j + 1 < window.cols &&
			!IScvNULL(i - 1, j + 1) &&
			cv(i, j) - cv(i - 1, j + 1) > ZERO) {
			tanB[2] = (cv(i, j) - cv(i - 1, j + 1)) * dx2;
			route[2] = 0.354 * dx * tanB[2];
			sum += route[2];
			nroute++;
		    }
		}
		if (j > 0 &&
		    !IScvNULL(i, j - 1) && cv(i, j) - cv(i, j - 1) > ZERO) {
		    tanB[3] = (cv(i, j) - cv(i, j - 1)) * dx1;
		    route[3] = 0.5 * dx * tanB[3];
		    sum += route[3];
		    nroute++;
		}
		if (j + 1 < window.cols) {
		    if (!IScvNULL(i, j + 1) && cv(i, j) - cv(i, j + 1) > ZERO) {
			tanB[5] = (cv(i, j) - cv(i, j + 1)) * dx1;
			route[5] = 0.5 * dx * tanB[5];
			sum += route[5];
			nroute++;
		    }
		}
		if (i + 1 < window.rows) {
		    if (j > 0 &&
			!IScvNULL(i + 1, j - 1) &&
			cv(i, j) - cv(i + 1, j - 1) > ZERO) {
			tanB[6] = (cv(i, j) - cv(i + 1, j - 1)) * dx2;
			route[6] = 0.354 * dx * tanB[6];
			sum += route[6];
			nroute++;
		    }
		    if (!IScvNULL(i + 1, j) && cv(i, j) - cv(i + 1, j) > ZERO) {
			tanB[7] = (cv(i, j) - cv(i + 1, j)) * dx1;
			route[7] = 0.5 * dx * tanB[7];
			sum += route[7];
			nroute++;
		    }
		    if (j + 1 < window.cols &&
			!IScvNULL(i + 1, j + 1) &&
			cv(i, j) - cv(i + 1, j + 1) > ZERO) {
			tanB[8] = (cv(i, j) - cv(i + 1, j + 1)) * dx2;
			route[8] = 0.354 * dx * tanB[8];
			sum += route[8];
			nroute++;
		    }
		}

		if (!nroute) {
		    G_debug(1, "Sink or boundary node at %d, %d\n", i, j);
		    nsink++;
		    sumtb = 0.0;
		    nslp = 0;
		    if (i > 0) {
			if (j > 0 && !IScvNULL(i - 1, j - 1)) {
			    sumtb += (cv(i - 1, j - 1)
				      - cv(i, j)) * dx2;
			    nslp++;
			}
			if (!IScvNULL(i - 1, j)) {
			    sumtb += (cv(i - 1, j)
				      - cv(i, j)) * dx1;
			    nslp++;
			}
			if (j + 1 < window.cols && !IScvNULL(i - 1, j + 1)) {
			    sumtb += (cv(i - 1, j + 1)
				      - cv(i, j)) * dx2;
			    nslp++;
			}
		    }

		    if (j > 0 && !IScvNULL(i, j - 1)) {
			sumtb += (cv(i, j - 1)
				  - cv(i, j)) * dx1;
			nslp++;
		    }
		    if (j + 1 < window.cols && !IScvNULL(i, j + 1)) {
			sumtb += (cv(i, j + 1)
				  - cv(i, j)) * dx1;
			nslp++;
		    }
		    if (i + 1 < window.rows) {
			if (j > 0 && !IScvNULL(i + 1, j - 1)) {
			    sumtb += (cv(i + 1, j - 1)
				      - cv(i, j)) * dx2;
			    nslp++;
			}
			if (!IScvNULL(i + 1, j)) {
			    sumtb += (cv(i + 1, j)
				      - cv(i, j)) * dx1;
			    nslp++;
			}
			if (j + 1 < window.cols && !IScvNULL(i + 1, j + 1)) {
			    sumtb += (cv(i + 1, j + 1)
				      - cv(i, j)) * dx2;
			    nslp++;
			}
		    }

		    sumtb /= nslp;
		    if (sumtb > ZERO) {
			atbv(i, j) = log((av(i, j) / (2 * dx * sumtb)));
		    }
		    else {
			Rast_set_d_null_value(&atbv(i, j), 1);
		    }
		    natb++;
		    continue;
		}

		C = av(i, j) / sum;
		atbv(i, j) = log(C);
		natb++;

		if (i > 0) {
		    if (j > 0) {
			av(i - 1, j - 1) += C * route[0];
		    }
		    av(i - 1, j) += C * route[1];
		    if (j + 1 < window.cols) {
			av(i - 1, j + 1) += C * route[2];
		    }
		}
		if (j > 0) {
		    av(i, j - 1) += C * route[3];
		}
		if (j + 1 < window.cols) {
		    av(i, j + 1) += C * route[5];
		}
		if (i + 1 < window.rows) {
		    if (j > 0) {
			av(i + 1, j - 1) += C * route[6];
		    }
		    av(i + 1, j) += C * route[7];
		    if (j + 1 < window.cols) {
			av(i + 1, j + 1) += C * route[8];
		    }
		}
	    }
	}
    }
    G_percent(natb - snatb, ncells - snatb, 1);
    G_important_message(_("Number of sinks or boundaries: %d"), nsink);
}
