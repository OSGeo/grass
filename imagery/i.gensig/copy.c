#include <grass/imagery.h>
int copy_covariances(double **a, double **b, int n)
{
    int b1, b2;

    for (b1 = 0; b1 < n; b1++)
	for (b2 = 0; b2 < n; b2++)
	    a[b1][b2] = b[b1][b2];

    return 0;
}
