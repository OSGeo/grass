#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include "local_proto.h"

dist_func *line_distance;

int get_line_box(const struct line_pnts *Points, struct bound_box *box)
{
    int i;

    if (Points->n_points == 0) {
        box->E = box->W = box->N = box->S = box->T = box->B = NAN;
        return 0;
    }

    box->E = box->W = Points->x[0];
    box->N = box->S = Points->y[0];
    box->T = box->B = Points->z[0];

    for (i = 1; i < Points->n_points; i++) {
        if (box->E < Points->x[i])
            box->E = Points->x[i];
        if (box->W > Points->x[i])
            box->W = Points->x[i];
        if (box->N < Points->y[i])
            box->N = Points->y[i];
        if (box->S > Points->y[i])
            box->S = Points->y[i];
        if (box->T < Points->z[i])
            box->T = Points->z[i];
        if (box->B > Points->z[i])
            box->B = Points->z[i];
    }

    return 1;
}

/* segment angle */
double sangle(struct line_pnts *Points, int segment)
{
    double dx, dy;

    if (Points->n_points < 2 || segment < 1)
        return -9;
    if (segment >= Points->n_points)
        G_fatal_error(_("Invalid segment number %d for %d points"), segment,
                      Points->n_points);

    dx = Points->x[segment] - Points->x[segment - 1];
    dy = Points->y[segment] - Points->y[segment - 1];

    return atan2(dy, dx);
}

/* calculate distance parameters between two primitives
 * return 1 point to point
 * return 2 point to line
 * return 1 line to line
 */
int line2line(struct line_pnts *FPoints, int ftype, struct line_pnts *TPoints,
              int ttype, double *fx, double *fy, double *fz, double *falong,
              double *fangle, double *tx, double *ty, double *tz,
              double *talong, double *tangle, double *dist, int with_z)
{
    int i, fseg, tseg, tmp_seg;
    double tmp_dist, tmp_x, tmp_y, tmp_z, tmp_along;
    int ret = 1;
    static struct line_pnts *iPoints = NULL;

    if (!iPoints) {
        iPoints = Vect_new_line_struct();
    }

    *dist = PORT_DOUBLE_MAX;

    /* fangle and tangle are angles in radians, counter clockwise from x axis
     * initialize to invalid angle */
    *fangle = *tangle = -9.;
    *falong = *talong = 0.;

    *fx = FPoints->x[0];
    *fy = FPoints->y[0];
    *fz = FPoints->z[0];

    *tx = TPoints->x[0];
    *ty = TPoints->y[0];
    *tz = TPoints->z[0];

    tmp_z = 0;

    /* point -> point */
    if ((ftype & GV_POINTS) && (ttype & GV_POINTS)) {
        line_distance(TPoints, FPoints->x[0], FPoints->y[0], FPoints->z[0],
                      with_z, tx, ty, tz, dist, NULL, talong);
    }

    /* point -> line and line -> line */
    if ((ttype & GV_LINES)) {

        fseg = tseg = 0;
        /* calculate the min distance between each point in fline with tline */
        for (i = 0; i < FPoints->n_points; i++) {

            tmp_seg = line_distance(TPoints, FPoints->x[i], FPoints->y[i],
                                    FPoints->z[i], with_z, &tmp_x, &tmp_y,
                                    &tmp_z, &tmp_dist, NULL, &tmp_along);
            if (*dist > tmp_dist) {
                *dist = tmp_dist;
                *fx = FPoints->x[i];
                *fy = FPoints->y[i];
                *fz = FPoints->z[i];
                *tx = tmp_x;
                *ty = tmp_y;
                *tz = tmp_z;
                *talong = tmp_along;
                tseg = tmp_seg;
                fseg = i + 1;
            }
        }
        *tangle = sangle(TPoints, tseg);

        if (FPoints->n_points > 1 && fseg > 0) {
            int np = FPoints->n_points;

            fseg--;

            if (fseg > 0) {
                FPoints->n_points = fseg + 1;
                *falong = Vect_line_length(FPoints);
                FPoints->n_points = np;
            }
            // np = fseg;
            if (fseg == 0)
                fseg = 1;
            *fangle = sangle(FPoints, fseg);
        }

        ret++;
    }

    /* line -> point and line -> line */
    if (ftype & GV_LINES) {

        fseg = tseg = 0;

        /* calculate the min distance between each point in tline with fline */
        for (i = 0; i < TPoints->n_points; i++) {

            tmp_seg = line_distance(FPoints, TPoints->x[i], TPoints->y[i],
                                    TPoints->z[i], with_z, &tmp_x, &tmp_y,
                                    &tmp_z, &tmp_dist, NULL, &tmp_along);
            if (*dist > tmp_dist) {
                *dist = tmp_dist;
                *fx = tmp_x;
                *fy = tmp_y;
                *fz = tmp_z;
                *falong = tmp_along;
                *tx = TPoints->x[i];
                *ty = TPoints->y[i];
                *tz = TPoints->z[i];
                fseg = tmp_seg;
                tseg = i + 1;
            }
        }
        *fangle = sangle(FPoints, fseg);

        if (TPoints->n_points > 1 && tseg > 0) {
            int np = TPoints->n_points;

            tseg--;

            if (tseg > 0) {
                TPoints->n_points = tseg + 1;
                *talong = Vect_line_length(TPoints);
                TPoints->n_points = np;
            }
            // np = tseg;
            if (tseg == 0)
                tseg = 1;
            *tangle = sangle(TPoints, tseg);
        }

        ret++;

        if ((ttype & GV_LINES) && *dist > 0) {
            /* check for line intersection */
            struct bound_box fbox, tbox;

            get_line_box(FPoints, &fbox);
            get_line_box(TPoints, &tbox);

            if (Vect_box_overlap(&fbox, &tbox)) {
                Vect_reset_line(iPoints);
                Vect_line_get_intersections(FPoints, TPoints, iPoints, with_z);
                if (iPoints->n_points) {
                    *dist = 0;
                    *fx = *tx = iPoints->x[0];
                    *fy = *ty = iPoints->y[0];
                    *fz = *tz = iPoints->z[0];

                    /* falong, talong */
                    fseg = line_distance(FPoints, iPoints->x[0], iPoints->y[0],
                                         iPoints->z[0], with_z, NULL, NULL,
                                         NULL, NULL, NULL, falong);
                    tseg = line_distance(TPoints, iPoints->x[0], iPoints->y[0],
                                         iPoints->z[0], with_z, NULL, NULL,
                                         NULL, NULL, NULL, talong);
                    /* fangle, tangle */
                    *fangle = sangle(FPoints, fseg);
                    *tangle = sangle(TPoints, tseg);
                }
            }
        }
    }

    return ret;
}

/* shortest distance between line and area
 * return 1 inside area
 * return 2 inside isle of area
 * return 3 outside area */
int line2area(struct Map_info *To, struct line_pnts *Points, int type, int area,
              const struct bound_box *abox, double *fx, double *fy, double *fz,
              double *falong, double *fangle, double *tx, double *ty,
              double *tz, double *talong, double *tangle, double *dist,
              int with_z)
{
    int i, j;
    double tmp_dist;
    int isle, nisles;
    int all_inside_outer, all_outside_inner;
    // int all_outside_outer;
    static struct line_pnts *aPoints = NULL;
    static struct line_pnts **iPoints = NULL;
    static struct bound_box *ibox = NULL;
    static int isle_alloc = 0;

    if (!aPoints)
        aPoints = Vect_new_line_struct();

    *dist = PORT_DOUBLE_MAX;

    /* fangle and tangle are angles in radians, counter clockwise from x axis
     * initialize to invalid angle */
    *fangle = *tangle = -9.;
    *falong = *talong = 0.;

    *fx = Points->x[0];
    *fy = Points->y[0];
    *fz = Points->z[0];

    *tx = Points->x[0];
    *ty = Points->y[0];
    *tz = Points->z[0];

    Vect_get_area_points(To, area, aPoints);
    nisles = Vect_get_area_num_isles(To, area);

    if (nisles > isle_alloc) {
        iPoints = G_realloc(iPoints, nisles * sizeof(struct line_pnts *));
        ibox = G_realloc(ibox, nisles * sizeof(struct bound_box));
        for (i = isle_alloc; i < nisles; i++)
            iPoints[i] = Vect_new_line_struct();
        isle_alloc = nisles;
    }
    for (i = 0; i < nisles; i++) {
        isle = Vect_get_area_isle(To, area, i);
        Vect_get_isle_points(To, isle, iPoints[i]);
        Vect_get_isle_box(To, isle, &ibox[i]);
    }

    /* inside area ? */
    all_inside_outer = 1;
    // all_outside_outer = 1;
    all_outside_inner = 1;

    int in_box;

    for (i = 0; i < Points->n_points; i++) {
        if (with_z)
            in_box = Vect_point_in_box(Points->x[i], Points->y[i], Points->z[i],
                                       abox);
        else
            in_box = Vect_point_in_box_2d(Points->x[i], Points->y[i], abox);
        if (in_box) {

            int poly;

            poly = Vect_point_in_poly(Points->x[i], Points->y[i], aPoints);

            if (poly > 0) {
                /* inside outer ring */
                // all_outside_outer = 0;
            }
            else {
                /* outside outer ring */
                all_inside_outer = 0;
            }

            /* exactly on boundary */
            if (poly == 2) {
                line2line(Points, type, aPoints, GV_BOUNDARY, fx, fy, fz,
                          falong, fangle, tx, ty, tz, talong, tangle, dist,
                          with_z);

                *talong = 0;
                *tangle = -9;

                return 1;
            }
            /* inside outer ring */
            else if (poly == 1) {
                int inside_isle = 0;

                for (j = 0; j < nisles; j++) {
                    if (with_z)
                        in_box = Vect_point_in_box(Points->x[i], Points->y[i],
                                                   Points->z[i], &ibox[j]);
                    else
                        in_box = Vect_point_in_box_2d(Points->x[i],
                                                      Points->y[i], &ibox[j]);
                    if (in_box) {

                        poly = Vect_point_in_poly(Points->x[i], Points->y[i],
                                                  iPoints[j]);

                        /* inside or exactly on boundary */
                        if (poly > 0) {
                            double tmp_fx, tmp_fy, tmp_fz, tmp_fangle,
                                tmp_falong;
                            double tmp_tx, tmp_ty, tmp_tz, tmp_tangle,
                                tmp_talong;

                            /* pass all points of the line,
                             * this will catch an intersection */
                            line2line(Points, type, iPoints[j], GV_BOUNDARY,
                                      &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong,
                                      &tmp_fangle, &tmp_tx, &tmp_ty, &tmp_tz,
                                      &tmp_talong, &tmp_tangle, &tmp_dist,
                                      with_z);

                            if (*dist > tmp_dist) {
                                *dist = tmp_dist;

                                *fx = tmp_fx;
                                *fy = tmp_fy;
                                *fz = tmp_fz;
                                *falong = tmp_falong;
                                *fangle = tmp_fangle;

                                *tx = tmp_tx;
                                *ty = tmp_ty;
                                *tz = tmp_tz;
                                *talong = 0;
                                *tangle = tmp_tangle;
                            }

                            if (poly == 1) /* excludes isle boundary */
                                inside_isle = 1;
                        }
                    }
                    if (*dist == 0)
                        break;
                }
                /* inside area (inside outer ring, outside inner rings
                 * or exactly on one of the inner rings) */
                if (!inside_isle) {
                    *fx = Points->x[i];
                    *fy = Points->y[i];
                    *fz = Points->z[i];

                    *tx = Points->x[i];
                    *ty = Points->y[i];
                    *tz = Points->z[i];

                    *fangle = *tangle = -9.;
                    *falong = *talong = 0.;

                    *dist = 0;

                    return 1;
                }
                else {
                    /* inside one of the islands */
                    all_outside_inner = 0;
                    if (*dist == 0) {
                        /* the line intersected with the isle boundary
                         * -> line is partially inside the area */

                        *fangle = *tangle = -9.;
                        *falong = *talong = 0.;

                        return 1;
                    }
                    /* else continue with next point */
                }
            } /* end inside outer ring */
        }
        else {
            /* point not in box of outer ring */
            all_inside_outer = 0;
        }
        /* exactly on boundary */
        if (*dist == 0)
            return 1;
    }

    /* if all points are inside the outer ring and inside inner rings,
     * there could still be an intersection with one of the inner rings */
    if (all_inside_outer) {
        if (all_outside_inner) {
            /* at least one point is really inside the area!
             * that should have been detected above */
            G_fatal_error(_("At least one point is really inside the area!"));
        }
        /* else all points are inside one of the area isles
         * and we already have the minimum distance */
        return 2;
    }

    /* if at least one point was found to be inside the outer ring,
     * but no point really inside the area,
     * and at least one point outside,
     * then there must be an intersection of the line with both
     * the outer ring and one of the isle boundaries */

    /* if all line points are outside of the area,
     * intersection is still possible */

    line2line(Points, type, aPoints, GV_BOUNDARY, fx, fy, fz, falong, fangle,
              tx, ty, tz, talong, tangle, dist, with_z);

    *talong = 0;

    if (*dist == 0)
        return 1;

    return 3;
}
