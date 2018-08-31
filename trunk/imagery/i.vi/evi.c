#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* EVI: Enhanced Vegetation Index
     * Huete A.R., Liu H.Q., Batchily K., vanLeeuwen W. (1997)
     * A comparison of vegetation indices global set of TM images for EOS-MODIS
     * Remote Sensing of Environment, 59:440-451.
     */ 
double e_vi(double bluechan, double redchan, double nirchan) 
{
    double tmp, result;

    tmp = nirchan + 6.0 * redchan - 7.5 * bluechan + 1.0;
    if (tmp == 0.0) {
	result = -1.0;
    }
    else {
	result = 2.5 * (nirchan - redchan) / tmp;
    }
    return result;
}


