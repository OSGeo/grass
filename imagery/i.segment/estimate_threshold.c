/* Purpose: open input files and suggest a reasonable threshold */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "iseg.h"

int estimate_threshold(char *image_group)
{
    double min, max, est_t;

    /* check if the input image group is valid. */
    check_group(image_group);

    /* read the raster files to find the minimum and maximum values */
    read_range(&min, &max, image_group);

    /* perform the calculations to estimate the threshold */
    est_t = calc_t(min, max);

    /* set the output message and finish */
    G_done_msg(_("Suggested threshold (if using -w flag and radioweight=1) is: <%g>"),
	       est_t);
    return TRUE;
}

/* function to validate that the user input is readable and contains files */
int check_group(char *image_group)
{
    struct Ref Ref;		/* group reference list */

    /* check that the input image group can be found */
    if (!I_get_group_ref(image_group, &Ref))
	G_fatal_error(_("Unable to read REF file for group <%s>"),
		      image_group);

    /* check that the input image group contains rasters */
    if (Ref.nfiles <= 0)
	G_fatal_error(_("Group <%s> contains no raster maps"), image_group);

    return TRUE;
}

/* function to find the min and max values in all rasters of an image group */
int read_range(double *min, double *max, char *image_group)
{
    int n;
    struct FPRange fp_range;	/* range for a raster */
    struct Ref Ref;		/* group reference list */
    double candidate_min, candidate_max;	/* for min/max in each raster */

    /* read the image group */
    I_get_group_ref(image_group, &Ref);

    /* initialize min/max with the first raster min/max */
    if (Rast_read_fp_range(Ref.file[0].name, Ref.file[0].mapset, &fp_range) != 1)	/* returns -1 on error, 2 on empty range, quiting either way. */
	G_fatal_error(_("No min/max found in raster map <%s>"),
		      Ref.file[0].name);
    else
	Rast_get_fp_range_min_max(&fp_range, min, max);

    /* check the min/max for any remaining rasters */
    for (n = 1; n < Ref.nfiles; n++) {
	if (Rast_read_fp_range(Ref.file[n].name, Ref.file[n].mapset, &fp_range) != 1) {	/* returns -1 on error, 2 on empty range, quiting either way. */
	    G_fatal_error(_("No min/max found in raster map <%s>"),
			  Ref.file[n].name);

	    Rast_get_fp_range_min_max(&fp_range, &candidate_min,
				      &candidate_max);

	    if (candidate_min < *min)
		*min = candidate_min;
	    if (candidate_max > *max)
		*max = candidate_max;
	}
    }

    return TRUE;
}

/* function to calculate a suggested threshold based on the min and max values of the rasters */
double calc_t(double min, double max)
{
    double t, fraction;

    /* Empirical testing indicated 1 to 5% of the differences was a good starting point. */
    fraction = 0.03;

    /* TODO: allow the community to test this estimate, the formula can be updated based on their advice. */
    t = fraction * (max - min);

    return t;
}
