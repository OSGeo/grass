#include "local_proto.h"

/*directions
 * 3|2|1
 * 4|0|8
 * 5|6|7 */
static int nextr[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };
static int nextc[8] = { 1, 0, -1, -1, -1, 0, 1, 1 };

int calc_pattern(PATTERN * pattern, int row, int cur_row, int col)
{
    /* calculate parameters of geomorphons and store it in the struct pattern */
    int i, j, pattern_size = 0;
    double zenith_angle, nadir_angle, angle;
    double nadir_threshold, zenith_threshold;
    double zenith_height, nadir_height, zenith_distance, nadir_distance;
    double cur_northing, cur_easting, target_northing, target_easting;
    double cur_distance;
    double center_height, height;

    /* use distance calculation */
    cur_northing = Rast_row_to_northing(row + 0.5, &window);
    cur_easting = Rast_col_to_easting(col + 0.5, &window);
    center_height = elevation.elev[cur_row][col];
    pattern->num_positives = 0;
    pattern->num_negatives = 0;
    pattern->positives = 0;
    pattern->negatives = 0;

    for (i = 0; i < 8; ++i) {
	/* reset patterns */
	pattern->pattern[i] = 0;
	pattern->elevation[i] = 0.;
	pattern->distance[i] = 0.;
	j = skip_cells + 1;
	zenith_angle = -(PI2);
	nadir_angle = PI2;

	if (cur_row + j * nextr[i] < 0 ||
	    cur_row + j * nextr[i] > row_buffer_size - 1 ||
	    col + j * nextc[i] < 0 || col + j * nextc[i] > ncols - 1)
	    continue;		/* border: current cell is on the end of DEM */
	if (Rast_is_f_null_value
	    (&elevation.elev[cur_row + nextr[i]][col + nextc[i]]))
	    continue;		/* border: next value is null, line-of-sight does not exists */
	pattern_size++;		/* line-of-sight exists, continue calculate visibility */

	target_northing =
	    Rast_row_to_northing(row + j * nextr[i] + 0.5, &window);
	target_easting =
	    Rast_col_to_easting(col + j * nextc[i] + 0.5, &window);
	cur_distance =
	    G_distance(cur_easting, cur_northing, target_easting,
		       target_northing);

	while (cur_distance < search_distance) {
	    if (cur_row + j * nextr[i] < 0 ||
		cur_row + j * nextr[i] > row_buffer_size - 1 ||
		col + j * nextc[i] < 0 || col + j * nextc[i] > ncols - 1)
		break;		/* reached end of DEM (cols) or buffer (rows) */

	    height =
		elevation.elev[cur_row + j * nextr[i]][col + j * nextc[i]] -
		center_height;
	    angle = atan2(height, cur_distance);

	    if (angle > zenith_angle) {
		zenith_angle = angle;
		zenith_height = height;
		zenith_distance = cur_distance;
	    }
	    if (angle < nadir_angle) {
		nadir_angle = angle;
		nadir_height = height;
		nadir_distance = cur_distance;
	    }
	    j += cell_step;
	    /*             j++; */ /* go to next cell */
	    target_northing =
		Rast_row_to_northing(row + j * nextr[i] + 0.5, &window);
	    target_easting =
		Rast_col_to_easting(col + j * nextc[i] + 0.5, &window);
	    cur_distance =
		G_distance(cur_easting, cur_northing, target_easting,
			   target_northing);
	}			/* end line of sight */

	/* original paper version */
	/*      zenith_angle=PI2-zenith_angle;
	   nadir_angle=PI2+nadir_angle;
	   if(fabs(zenith_angle-nadir_angle) > flat_threshold) {
	   if((nadir_angle-zenith_angle) > 0) {
	   patterns->pattern[i]=1;
	   patterns->elevation[i]=nadir_height;
	   patterns->distance[i]=nadir_distance;
	   patterns->num_positives++;
	   } else {
	   patterns->pattern[i]=-1;
	   patterns->elevation[i]=zenith_height;
	   patterns->distance[i]=zenith_distance;
	   patterns->num_negatives++;
	   }
	   } else {
	   patterns->distance[i]=search_distance;
	   }
	 */
	/* this is used to lower flat threshold if distance exceed flat_distance parameter */
	zenith_threshold = (flat_distance > 0 &&
			    flat_distance <
			    zenith_distance) ? atan2(flat_threshold_height,
						     zenith_distance) :
	    flat_threshold;
	nadir_threshold = (flat_distance > 0 &&
			   flat_distance <
			   nadir_distance) ? atan2(flat_threshold_height,
						   nadir_distance) :
	    flat_threshold;

	if (zenith_angle > zenith_threshold)
	    pattern->positives += i;
	if (nadir_angle < -nadir_threshold)
	    pattern->negatives += i;

	if (fabs(zenith_angle) > zenith_threshold ||
	    fabs(nadir_angle) > nadir_threshold) {
	    if (fabs(nadir_angle) < fabs(zenith_angle)) {
		pattern->pattern[i] = 1;
		pattern->elevation[i] = zenith_height;	/* ZMIANA! */
		pattern->distance[i] = zenith_distance;
		pattern->num_positives++;
	    }
	    if (fabs(nadir_angle) > fabs(zenith_angle)) {
		pattern->pattern[i] = -1;
		pattern->elevation[i] = nadir_height;	/* ZMIANA! */
		pattern->distance[i] = nadir_distance;
		pattern->num_negatives++;
	    }
	}
	else {
	    pattern->distance[i] = search_distance;
	}

    }				/* end for */
    return pattern_size;
}
