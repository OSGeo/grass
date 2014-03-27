#include "local_proto.h"

#define SQRT(x) ((x) * (x))   /* ??? */

int create_distance_mask(int radius)
{

    int i, j;
    int window = 2 * radius + 1;

    distance_mask = G_malloc(window * sizeof(float *));

    for (i = 0; i < window; i++)
	distance_mask[i] = G_malloc(window * sizeof(float));

    for (i = 0; i < window; i++)
	for (j = 0; j < window; j++)
	    distance_mask[i][j] =
		(SQR(i - radius) + SQR(j - radius) <= SQR(radius)) ?
		sqrt(SQR(i - radius) + SQR(j - radius)) : 0;

    return 0;
}

int snap_point(OUTLET *point, int radius, SEGMENT *streams, SEGMENT *accum,
	       double accum_treshold)
{

    int i, j, di = -1, dj = -1;
    int status = 3;
    int teststream = 0;
    float cur_distance = radius;
    float distance = 0;
    double absaccum = 0;
    double sumaccum = 0;
    double maxaccum = 0;
    int naccum = -1;

    if (point->stream > 0 && point->accum > accum_treshold)
	return 0;		/* point lies on line(or skipped) and has proper treshold */

    if (streams) {
	/* stream version: assume ve have stream network and points 
	 * are snapped to stream points where accum is greater than treshold
	 * or to nearest stream point in accum is not supplied */

	for (i = -radius; i <= radius; ++i)
	    for (j = -radius; j <= radius; ++j) {

		if (point->r + i < 0 || point->r + i >= nrows ||
		    point->c + j < 0 || point->c + j >= ncols)
		    continue;

		if (!distance_mask[i + radius][j + radius])
		    continue;

		segment_get(streams, &teststream, point->r + i, point->c + j);
		distance = distance_mask[i + radius][j + radius];

		if (teststream) {	/* is stream line */

		    if (accum) {
			segment_get(accum, &absaccum, point->r + i,
				    point->c + j);
			absaccum = fabs(absaccum);
		    }

		    if (absaccum >= accum_treshold)	/* if no accum absaccum always =-1 */
			if (cur_distance > distance) {
			    cur_distance = distance;
			    di = i;
			    dj = j;
			}
		}
	    }
    }				/* end of streams version */

    if (!streams) {
	/* no stream version: problem for MFD. the snap point is found
	 * in different manner. It is not only point where accum exceed the 
	 * treshold (may be far for potential streamline) but must exceed the
	 * mean value of accums in searach area taken in cells where treshold is exceeded */

	for (i = -radius; i <= radius; ++i)
	    for (j = -radius; j <= radius; ++j) {

		if (point->r + i < 0 || point->r + i >= nrows ||
		    point->c + j < 0 || point->c + j >= ncols)
		    continue;

		if (!distance_mask[i + radius][j + radius])
		    continue;

		segment_get(accum, &absaccum, point->r + i, point->c + j);
		absaccum = fabs(absaccum);

		if (absaccum > maxaccum)
		    maxaccum = absaccum;

		if (absaccum > accum_treshold) {
		    sumaccum += absaccum;
		    naccum++;
		}
	    }

        /* TODO: this should be fixed by someone who knows the code */
	if (sumaccum > 0) 
	    /* accum_treshold=(sumaccum/naccum+maxaccum)/2 */ ;
	accum_treshold = sumaccum / naccum;

	for (i = -radius; i <= radius; ++i)
	    for (j = -radius; j <= radius; ++j) {

		if (point->r + i < 0 || point->r + i >= nrows ||
		    point->c + j < 0 || point->c + j >= ncols)
		    continue;

		if (!distance_mask[i + radius][j + radius])
		    continue;

		segment_get(accum, &absaccum, point->r + i, point->c + j);
		absaccum = fabs(absaccum);

		if (accum_treshold > 0 && absaccum > accum_treshold)
		    if (cur_distance > distance) {
			cur_distance = distance;
			di = i;
			dj = j;
		    }
	    }
    }				/* end of non-streams version */
    if (di == -1 && dj == -1) {
	G_warning(_("Unable to snap point with cat %d, in a given radius. "
                    "Increase search radius."), point->cat);
	di = 0;
	dj = 0;
	status = 2;
    }
    point->di = di;
    point->dj = dj;
    point->status = status;
    return 0;
}
