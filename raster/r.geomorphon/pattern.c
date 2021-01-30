#include "local_proto.h"

/*directions
 * 3|2|1
 * 4|0|8
 * 5|6|7 */
static int nextr[NUM_DIRS] = { -1, -1, -1, 0, 1, 1, 1, 0 };
static int nextc[NUM_DIRS] = { 1, 0, -1, -1, -1, 0, 1, 1 };
const char *dirname[NUM_DIRS] = { "NE", "N", "NW", "W", "SW", "S", "SE", "E" };

/*
 * A more thorough comparison using a few factors of different priority
 * similar to the BGP best path selection algorithm. When the distances are
 * equal, it becomes an improved version of the original r.geomorphon
 * comparison (which is different from the original paper), in that it applies
 * each threshold to its respective angle and does not default to 0 when there
 * is a tie. Each angle must be non-negative.
 */
static int compare_multi(const double nadir_angle, const double zenith_angle,
                         const double nadir_threshold,
                         const double zenith_threshold,
                         const double nadir_distance,
                         const double zenith_distance)
{
    const unsigned char
        nadir_over = nadir_angle > nadir_threshold,
        zenith_over = zenith_angle > zenith_threshold;

    /*
     * If neither angle exceeds its threshold, consider the elevation profile
     * flat enough.
     */
    if (!nadir_over && !zenith_over)
        return 0;
    /*
     * If exactly one angle exceeds its threshold, that angle represents the
     * elevation profile.
     */
    if (!nadir_over && zenith_over)
        return 1;
    if (nadir_over && !zenith_over)
        return -1;
    /*
     * If both angles exceed their thresholds, the greater angle (if any)
     * represents the elevation profile better.
     */
    if (nadir_angle < zenith_angle)
        return 1;
    if (nadir_angle > zenith_angle)
        return -1;
    /*
     * Here each angle is above its threshold and the angles are exactly equal
     * (which happens quite often when the elevation values are integer rather
     * than real). Consider the angle computed over the greater distance to
     * represent the elevation profile better since it is based on a greater
     * number of cells.
     */
    if (nadir_distance < zenith_distance)
        return 1;
    if (nadir_distance > zenith_distance)
        return -1;
    /*
     * If there is still a tie, 0 would not be a valid result because both
     * angles exceed their thresholds hence the elevation profile definitely
     * is not flat. Resolve this with a preferred constant value.
     */
    return 1;
}

int calc_pattern(PATTERN * pattern, int row, int cur_row, int col,
                 const int oneoff)
{
    /* calculate parameters of geomorphons and store it in the struct pattern */
    int i, j, pattern_size = 0;
    double zenith_angle, nadir_angle, angle;
    double nadir_threshold, zenith_threshold;
    double zenith_height, nadir_height, zenith_distance, nadir_distance;
    double zenith_easting, zenith_northing, nadir_easting, nadir_northing;
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

    if (oneoff)
        prof_sso("search_rel_elevation_m");
    for (i = 0; i < NUM_DIRS; ++i) {
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
            continue;           /* border: current cell is on the end of DEM */
        if (Rast_is_f_null_value
            (&elevation.elev[cur_row + nextr[i]][col + nextc[i]]))
            continue;           /* border: next value is null, line-of-sight does not exists */
        pattern_size++;         /* line-of-sight exists, continue calculate visibility */

        target_northing =
            Rast_row_to_northing(row + j * nextr[i] + 0.5, &window);
        target_easting =
            Rast_col_to_easting(col + j * nextc[i] + 0.5, &window);
        cur_distance =
            G_distance(cur_easting, cur_northing, target_easting,
                       target_northing);

        if (oneoff) {
            zenith_northing = nadir_northing = target_northing;
            zenith_easting = nadir_easting = target_easting;
            pattern->e[i] = cur_easting;
            pattern->n[i] = cur_northing;
            prof_sso(dirname[i]);
        }
        while (cur_distance < search_distance) {
            if (cur_row + j * nextr[i] < 0 ||
                cur_row + j * nextr[i] > row_buffer_size - 1 ||
                col + j * nextc[i] < 0 || col + j * nextc[i] > ncols - 1)
                break;          /* reached end of DEM (cols) or buffer (rows) */

            height =
                elevation.elev[cur_row + j * nextr[i]][col + j * nextc[i]] -
                center_height;
            angle = atan2(height, cur_distance);

            if (angle > zenith_angle) {
                zenith_angle = angle;
                zenith_height = height;
                zenith_distance = cur_distance;
                if (oneoff) {
                    zenith_easting = target_easting;
                    zenith_northing = target_northing;
                }
            }
            if (angle < nadir_angle) {
                nadir_angle = angle;
                nadir_height = height;
                nadir_distance = cur_distance;
                if (oneoff) {
                    nadir_easting = target_easting;
                    nadir_northing = target_northing;
                }
            }
            if (oneoff) {
                char step_name[32];

                snprintf(step_name, sizeof(step_name), "step_%u",
                         (unsigned)j);
                prof_dbl(step_name, height);
            }
            j += cell_step;
            /*             j++; *//* go to next cell */
            target_northing =
                Rast_row_to_northing(row + j * nextr[i] + 0.5, &window);
            target_easting =
                Rast_col_to_easting(col + j * nextc[i] + 0.5, &window);
            cur_distance =
                G_distance(cur_easting, cur_northing, target_easting,
                           target_northing);
        }                       /* end line of sight */
        if (oneoff)
            prof_eso();

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

        if (compmode != ANGLEV1) {
            /*
             * One of the differences from ANGLEV1 is that even if there is a
             * tie, the second switch block will eventually use one of the
             * distances instead of 0 to set the cardinal point distance.
             */
            switch (compmode) {
            case ANGLEV2:
                pattern->pattern[i] =
                    compare_multi(fabs(nadir_angle), fabs(zenith_angle),
                                  nadir_threshold, zenith_threshold, 0, 0);
                break;
            case ANGLEV2_DISTANCE:
                pattern->pattern[i] =
                    compare_multi(fabs(nadir_angle), fabs(zenith_angle),
                                  nadir_threshold, zenith_threshold,
                                  nadir_distance, zenith_distance);
                break;
            default:
                G_fatal_error(_("Internal error in %s()"), __func__);
            }

            switch (pattern->pattern[i]) {
            case 1:
                pattern->elevation[i] = zenith_height;
                pattern->distance[i] = zenith_distance;
                pattern->num_positives++;
                if (oneoff) {
                    pattern->e[i] = zenith_easting;
                    pattern->n[i] = zenith_northing;
                }
                break;
            case -1:
                pattern->elevation[i] = nadir_height;
                pattern->distance[i] = nadir_distance;
                pattern->num_negatives++;
                if (oneoff) {
                    pattern->e[i] = nadir_easting;
                    pattern->n[i] = nadir_northing;
                }
                break;
            case 0:
                pattern->distance[i] = search_distance;
                if (oneoff) {
                    /*
                     * When cell_step == 1, which is always the case in the
                     * one-off mode, which is the only use case for e[] and
                     * n[], after the while() loop the distance to
                     * (target_easting,target_northing) is cur_distance and
                     * cur_distance == search_distance.
                     */
                    pattern->e[i] = target_easting;
                    pattern->n[i] = target_northing;
                }
                break;
            }

            continue;
        }                       /* if (compmode != ANGLEV1) */

        /* ANGLEV1 */
        if (fabs(zenith_angle) > zenith_threshold ||
            fabs(nadir_angle) > nadir_threshold) {
            if (fabs(nadir_angle) < fabs(zenith_angle)) {
                pattern->pattern[i] = 1;
                pattern->elevation[i] = zenith_height;  /* A CHANGE! */
                pattern->distance[i] = zenith_distance;
                pattern->num_positives++;
                if (oneoff) {
                    pattern->e[i] = zenith_easting;
                    pattern->n[i] = zenith_northing;
                }
            }
            if (fabs(nadir_angle) > fabs(zenith_angle)) {
                pattern->pattern[i] = -1;
                pattern->elevation[i] = nadir_height;   /* A CHANGE! */
                pattern->distance[i] = nadir_distance;
                pattern->num_negatives++;
                if (oneoff) {
                    pattern->e[i] = nadir_easting;
                    pattern->n[i] = nadir_northing;
                }
            }
            /*
             * If the angles are exactly equal, the cardinal direction search
             * results are the values set at the beginning of the for() loop.
             */
        }
        else {
            pattern->distance[i] = search_distance;
            if (oneoff) {
                pattern->e[i] = target_easting;
                pattern->n[i] = target_northing;
            }
        }

    }                           /* end for */
    if (oneoff)
        prof_eso();
    return pattern_size;
}
