#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"
#include "local_proto.h"


int invert_signatures(void)
{
    int c;			/* class */
    int j, k;
    int stat;
    int *ik, *jk;
    double det;
    struct One_Sig *s;
    int bad;

    ik = (int *)G_malloc(S.nbands * sizeof(int));
    jk = (int *)G_malloc(S.nbands * sizeof(int));

    /* invert each signature */
    bad = 0;
    for (c = 0; c < S.nsigs; c++) {
	stat = invert(s = &S.sig[c], S.nbands, ik, jk, &det);
	if (stat != 1) {
	    if (stat)
		G_warning(_("Signature %d is not valid (ill-conditioned) - ignored"),
			  c + 1);
	    else
		G_warning(_("Signature %d is not valid (singular) - ignored"),
			  c + 1);

	    bad = 1;
	    S.sig[c].status = -1;
	    for (j = 0; j < S.nbands; j++)
		for (k = 0; k < S.nbands; k++)
		    s->var[j][k] = 0.0;

	    for (k = 0; k < S.nbands; k++)
		s->var[k][k] = 1.0;
	    B[c] = -1.0e38;
	}
	else {
	    B[c] = -0.5 * log(det);
	}
    }

    G_free(ik);
    G_free(jk);

    return bad ? 0 : 1;
}


int invert(struct One_Sig *s, int nbands, int *ik, int *jk, double *det)
{
    int i, j, k;
    double max;
    double dx, dy, v;

    /* copy lower half to upper half */
    for (k = 0; k < nbands; k++)
	for (j = 0; j < k; j++)
	    s->var[j][k] = s->var[k][j];

    /* invert */
    *det = 1.0;
    for (k = 0; k < nbands; k++) {	/* 30 */
	max = 0.0;
	for (i = k; i < nbands; i++) {	/* 330 */
	    for (j = k; j < nbands; j++) {	/* 330 */
		if ((dx = v = s->var[i][j]) < 0)
		    dx = -dx;
		if ((dy = max) < 0)
		    dy = -dy;
		if (dy <= dx) {
		    ik[k] = i;
		    jk[k] = j;
		    max = v;
		}
	    }
	    /*330 */ }
	if (max == 0.0)
	    return -1;		/* ill conditioned matrix */

	if (ik[k] != k) {	/* 351 */
	    int kk = ik[k];

	    for (j = 0; j < nbands; j++) {	/* 350 */
		v = s->var[k][j];
		s->var[k][j] = s->var[kk][j];
		s->var[kk][j] = -v;
		/*350 */ }
	    /*351 */ }
	if (jk[k] != k) {	/* 361 */
	    int jj = jk[k];

	    for (i = 0; i < nbands; i++) {	/* 360 */
		v = s->var[i][k];
		s->var[i][k] = s->var[i][jj];
		s->var[i][jj] = -v;
		/*360 */ }
	    /*361 */ }
	for (j = 0; j < nbands; j++)
	    if (j != k)
		s->var[j][k] /= -max;

	for (j = 0; j < nbands; j++)
	    if (j != k) {
		v = s->var[j][k];
		for (i = 0; i < nbands; i++)
		    if (i != k)
			s->var[j][i] += v * s->var[k][i];
	    }

	for (j = 0; j < nbands; j++)
	    if (j != k)
		s->var[k][j] /= max;

	*det *= max;
	s->var[k][k] = 1.0 / max;
	/*30 */
    }

    /* zero means non-invertible */
    if (*det == 0.0)
	return 0;

    /*
     * if negative, then matrix is not positive-definite
     * (But this probably not a sufficient test)
     */
    if (*det < 0.0)
	return -1;

    /* restore ordering of matrix */
    for (k = nbands - 1; k >= 0; k--) {	/* 530 */
	j = ik[k];

	if (j > k)
	    for (i = 0; i < nbands; i++) {	/* 510 */
		v = s->var[i][k];
		s->var[i][k] = -(s->var[i][j]);
		s->var[i][j] = v;
		/*510 */ }

	i = jk[k];

	if (i > k)
	    for (j = 0; j < nbands; j++) {	/* 520 */
		v = s->var[k][j];
		s->var[k][j] = -(s->var[i][j]);
		s->var[i][j] = v;
		/*520 */ }
	/*530 */
    }

    return 1;
}
