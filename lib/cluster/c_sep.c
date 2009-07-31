/*!
  \file cluster/c_sep.c
  
  \brief Cluster library - Separation
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <math.h>
#include <grass/cluster.h>

#define FAR ((double) -1.0)

/*!
  \brief ?

  \param C pointer to Cluster structure
  \param class1 1st class
  \param class2 2nd class
*/
double I_cluster_separation(struct Cluster *C, int class1, int class2)
{
    int band;
    double q;
    double d;
    double var;
    double a1, a2;
    double n1, n2;
    double m1, m2;
    double s1, s2;

    if (C->count[class1] < 2)
	return FAR;
    if (C->count[class2] < 2)
	return FAR;
    n1 = (double)C->count[class1];
    n2 = (double)C->count[class2];

    d = 0.0;
    a1 = a2 = 0.0;
    for (band = 0; band < C->nbands; band++) {
	s1 = C->sum[band][class1];
	s2 = C->sum[band][class2];
	m1 = s1 / n1;
	m2 = s2 / n2;
	q = m1 - m2;
	q = q * q;
	d += q;


	var = C->sum2[band][class1] - (s1 * m1);
	var /= n1 - 1;
	if (var)
	    a1 += q / var;

	var = C->sum2[band][class2] - (s2 * m2);
	var /= n2 - 1;
	if (var)
	    a2 += q / var;
    }
    if (d == 0.0)
	return d;

    if (a1 < 0 || a2 < 0)
	return FAR;
    if (a1)
	a1 = sqrt(6 * d / a1);
    if (a2)
	a2 = sqrt(6 * d / a2);
    q = a1 + a2;
    if (q == 0.0)
	return FAR;

    return (sqrt(d) / q);
}
