/*!
  \file cluster/c_sig.c
  
  \brief Cluster library - Signatures
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Create signatures

  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_signatures(struct Cluster *C)
{
    int c, p, band1, band2;
    int n;
    double m1, m2;
    double p1, p2;
    double dn;

    /*
       fprintf (stderr, "c_sig: 1\n");
       fprintf (stderr, "  nclasses %d\n", C->nclasses);
       fprintf (stderr, "  npoints  %d\n", C->npoints );
       fprintf (stderr, "  nbands   %d\n", C->nbands  );
     */
    for (n = 0; n < C->nclasses; n++) {
	I_new_signature(&C->S);
    }

    for (p = 0; p < C->npoints; p++) {
	c = C->class[p];
	if (c < 0)
	    continue;
	/*
	   if (c >= C->nclasses)
	   fprintf (stderr, " class[%d]=%d ** illegal **\n", p, c);
	 */
	dn = n = C->count[c];
	if (n < 2)
	    continue;
	for (band1 = 0; band1 < C->nbands; band1++) {
	    m1 = C->sum[band1][c] / dn;
	    p1 = C->points[band1][p];
	    for (band2 = 0; band2 <= band1; band2++) {
		m2 = C->sum[band2][c] / dn;
		p2 = C->points[band2][p];
		C->S.sig[c].var[band1][band2] += (p1 - m1) * (p2 - m2);
	    }
	}
    }

    for (c = 0; c < C->nclasses; c++) {
	dn = n = C->S.sig[c].npoints = C->count[c];
	if (n == 0)
	    dn = 1.0;
	for (band1 = 0; band1 < C->nbands; band1++)
	    C->S.sig[c].mean[band1] = C->sum[band1][c] / dn;
	dn = n = C->count[c] - 1;
	if (n < 1)
	    continue;
	for (band1 = 0; band1 < C->nbands; band1++)
	    for (band2 = 0; band2 <= band1; band2++)
		C->S.sig[c].var[band1][band2] /= dn;
	C->S.sig[c].status = 1;
    }

    return 0;
}
