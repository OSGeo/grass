#include<stdio.h>
#include<math.h>
#include<stdlib.h>

/*From https://karnieli-rsl.com/image.ashx?i=901711.pdf&fn=1997-Karnieli_CI_IJRS_97.pdf
 * Crust Index CI=1-(RED-BLUE)/(RED+BLUE). (KARNIELI, 1997)
 * Development and implementation of spectral crust index over dune sands
 * The Remote Sensing Laboratory, J. Blaustein Institute for Desert Research, Ben Gurion University, Sede-Boker Campus 84990, Israel. (Received 26 January 1996; in ® nal form 19 July 1996) */
    /* Crust Index */
double c_i(double bluechan, double redchan)
{
    double result;

    if ((redchan + bluechan) == 0.0) {
        result = -1.0;
    }
    else {
        result = 1 - (redchan - bluechan) / (redchan + bluechan);
    }
    return result;
}
