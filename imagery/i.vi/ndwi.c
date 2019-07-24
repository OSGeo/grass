#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Normalized Difference Water Index
     * after McFeeters (1996), https://doi.org/10.3390/rs5073544 */
double nd_wi(double greenchan, double nirchan)
{
    double result;

    if ((greenchan + nirchan) == 0.0) {
	result = -1.0; /* TODO: -1 or 0 */
    }
    else {
	result = (greenchan - nirchan) / (greenchan + nirchan);
    }
    return result;
}


