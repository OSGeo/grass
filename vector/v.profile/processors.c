#include <stdio.h>
#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

extern Result *resultset;

/* For batch memory allocation */
static size_t alloc_size = 0;

/* Add point to result data set */
void add_point(const int cat, const double dist,
               const double z, size_t * rescount, const int open3d)
{
    Result *tmp;

    /* Allocate memory in batches */
    if (*rescount + 1 > alloc_size) {
        alloc_size += 100;

        tmp =
            (Result *) G_realloc(resultset,
                                 sizeof(Result) * (*rescount + 100));
        /* Don't leak memory if realloc fails */
        if (!tmp) {
            G_free(resultset);
            G_fatal_error(_("Out of memory"));
        }
        else
            resultset = tmp;
    }

    resultset[*rescount].distance = dist;
    resultset[*rescount].cat = cat;

    /* Vect_cat_get(Cats, field_index, &resultset[*rescount].cat); */
    if (open3d == WITH_Z)
        resultset[*rescount].z = z;
    (*rescount)++;
    G_debug(3, "Distance of point %zu is %f", *rescount,
            resultset[*rescount - 1].distance);
}

/* Check if point is on profile line (inside buffer) and calculate distance to it */
void proc_point(struct line_pnts *Points, struct line_pnts *Profil,
                struct line_pnts *Buffer, const int cat,
                size_t * rescount, const int open3d)
{
    double dist;

    if (Vect_point_in_poly(*Points->x, *Points->y, Buffer) > 0) {
        Vect_line_distance(Profil, *Points->x, *Points->y, *Points->z,
                           open3d, NULL, NULL, NULL, NULL, NULL, &dist);
        add_point(cat, dist, *Points->z, rescount, open3d);
    }
}

/* Process all line intersection points */
void proc_line(struct line_pnts *Ipoints, struct line_pnts *Profil,
               const int cat, size_t * rescount, const int open3d)
{
    int i;
    double dist;

    /* Add all line and profile intersection points to resultset */
    for (i = 0; i < Ipoints->n_points; i++) {
        Vect_line_distance(Profil, Ipoints->x[i], Ipoints->y[i],
                           Ipoints->z[i], open3d, NULL, NULL, NULL, NULL,
                           NULL, &dist);
        add_point(cat, dist, Ipoints->z[i], rescount, open3d);
    }
}
