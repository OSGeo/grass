#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* EVI2: Enhanced Vegetation Index
     * Zhangyan Jiang ; Alfredo R. Huete ; Youngwook Kim and Kamel Didan
     * 2-band enhanced vegetation index without a blue band and its application to AVHRR data
     * Proc. SPIE 6679, Remote Sensing and Modeling of Ecosystems for Sustainability IV, 667905 (October 09, 2007)
     * doi:10.1117/12.734933
     * http://dx.doi.org/10.1117/12.734933
     */ 
double e_vi2(double redchan, double nirchan) 
{
    double tmp, result;

    tmp = nirchan + 2.4 * redchan + 1.0;
    if (tmp == 0.0) {
        result = -1.0;
    }
    else {
        result = 2.5 * (nirchan - redchan) / tmp;
    }
    return result;
}

