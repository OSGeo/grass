#include <stdlib.h>
#include <grass/gis.h>
#include "vizual.h"


int viz_calc_tvals(cmndln_info * linefax, char **a_levels, char *a_min,
		   char *a_max, char *a_step, char *a_tnum, int quiet)
{
    double interval;		/*increment tvalue for intervals */
    float datarange;		/*diff btwn min and max data tvalues */
    float tval;			/*the threshold value being computed */
    float min, max;
    int ithresh;
    int do_interval = 0;
    int i;			/*looping variable */

    /* 
     ** Note that the maximum number of thresholds
     ** that are computed is 127.  This is to set a cap on the size of the
     ** display file that is created */

    /* If levels are specified, only use those.  Otherwise, if step is
       specified use that, otherwise use number of thresholds */

    min = Headfax.min;
    max = Headfax.max;
    if (a_min)
	sscanf(a_min, "%f", &min);
    if (a_max)
	sscanf(a_max, "%f", &max);
    datarange = max - min;
    if (datarange <= 0.0)
	G_fatal_error("Range error: %f", datarange);

    if (a_levels) {
	for (i = 0; a_levels[i]; i++) {
	    if (i == MAXTHRESH) {
		G_warning("Maximum no. of thresholds is %d", MAXTHRESH);
		break;
	    }
	    if (1 != sscanf(a_levels[i], "%f", &(linefax->tvalue[i]))) {
		G_usage();
		exit(0);
	    }
	}
	linefax->nthres = i;
    }
    else if (a_step) {
	sscanf(a_step, "%lf", &interval);
	do_interval = 1;
    }
    else if (a_tnum) {		/* should already be default even if not specified */
	sscanf(a_tnum, "%d", &ithresh);
	if (ithresh < 2) {
	    ithresh = 2;
	    G_warning("Minimum number of thresholds is 2");
	}
	interval = datarange / (ithresh - 1);
	do_interval = 1;
    }
    else {			/* should never get here */
	G_usage();
	exit(0);
    }

    if (do_interval) {
	for (i = 0, tval = min; tval <= max; i++, tval = min + (i * interval)) {
	    if (i == MAXTHRESH) {
		G_warning("Maximum no. of thresholds is %d", MAXTHRESH);
		break;
	    }
	    linefax->tvalue[i] = tval;
	}

	linefax->nthres = i;
    }

    if (!quiet) {
	fprintf(stderr, "threshold values: ");
	for (i = 0; i < linefax->nthres; i++)
	    fprintf(stderr, "%f ", linefax->tvalue[i]);
	G_message("No. of thresholds: %i", linefax->nthres + 1);

    }

    return (0);
}
