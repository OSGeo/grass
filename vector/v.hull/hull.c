#include <stdlib.h>
#include <grass/glocale.h>

#include "hull.h"

int rightTurn(struct Point *P, int i, int j, int k)
{
    double a, b, c, d;

    a = P[i].x - P[j].x;
    b = P[i].y - P[j].y;
    c = P[k].x - P[j].x;
    d = P[k].y - P[j].y;
    return a * d - b * c < 0;
}

int cmpPoints(const void *v1, const void *v2)
{
    struct Point *p1, *p2;

    p1 = (struct Point *)v1;
    p2 = (struct Point *)v2;
    if (p1->x > p2->x)
	return 1;
    else if (p1->x < p2->x)
	return -1;
    else
	return 0;
}

int convexHull(struct Point *P, int numPoints, int **hull)
{
    int pointIdx, upPoints, loPoints;
    int *upHull, *loHull;

    /* sort points in ascending x order */
    qsort(P, numPoints, sizeof(struct Point), cmpPoints);

    *hull = (int *)G_malloc(numPoints * 2 * sizeof(int));

    /* compute upper hull */
    upHull = *hull;
    upHull[0] = 0;
    upHull[1] = 1;
    upPoints = 1;
    for (pointIdx = 2; pointIdx < numPoints; pointIdx++) {
	upPoints++;
	upHull[upPoints] = pointIdx;
	while (upPoints > 1 &&
	       !rightTurn(P, upHull[upPoints], upHull[upPoints - 1],
			  upHull[upPoints - 2])
	    ) {
	    upHull[upPoints - 1] = upHull[upPoints];
	    upPoints--;
	}
    }

    /* compute lower hull, overwrite last point of upper hull */
    loHull = &(upHull[upPoints]);
    loHull[0] = numPoints - 1;
    loHull[1] = numPoints - 2;
    loPoints = 1;
    for (pointIdx = numPoints - 3; pointIdx >= 0; pointIdx--) {
	loPoints++;
	loHull[loPoints] = pointIdx;
	while (loPoints > 1 &&
	       !rightTurn(P, loHull[loPoints], loHull[loPoints - 1],
			  loHull[loPoints - 2])
	    ) {
	    loHull[loPoints - 1] = loHull[loPoints];
	    loPoints--;
	}
    }

    G_debug(3, "numPoints:%d loPoints:%d upPoints:%d",
	    numPoints, loPoints, upPoints);

    /* reclaim unneeded memory */
    *hull = (int *)G_realloc(*hull, (loPoints + upPoints) * sizeof(int));
    return loPoints + upPoints;
}



void convexHull3d(struct Point *P, const int numPoints, struct Map_info *Map)
{

    int error;
    int i;
    double *px;
    double *py;
    double *pz;

    px = G_malloc(sizeof(double) * numPoints);
    py = G_malloc(sizeof(double) * numPoints);
    pz = G_malloc(sizeof(double) * numPoints);

    for (i = 0; i < numPoints; i++) {
	px[i] = (P)[i].x;
	py[i] = (P)[i].y;
	pz[i] = (P)[i].z;
    }

    /* make 3D hull */
    error = make3DHull(px, py, pz, numPoints, Map);
    if (error < 0) {
	G_fatal_error(_("Simple planar hulls not implemented yet"));
    }

    G_free(px);
    G_free(py);
    G_free(pz);

}
