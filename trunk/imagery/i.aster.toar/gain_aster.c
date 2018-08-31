#include<stdio.h>
#include<stdlib.h>


/* Gain values LUT for ASTER */
double gain_aster(int band_number, int gain_code)
{
    /*Gain Code */
    /*0 - High (Not Applicable for band 10-14: TIR) */
    /*1 - Normal */
    /*2 - Low 1(Not Applicable for band 10-14: TIR) */
    /*3 - Low 2(Not Applicable for Band 1-3N/B & 10-14) */
    double result;

    /* HIGH GAIN */
    if (gain_code == 0) {
        /* Band 1 */
        if (band_number == 0)
            result = 0.676;
        /* Band 2 */
        if (band_number == 1)
            result = 0.708;
        /* Band 3N */
        if (band_number == 2)
            result = 0.423;
        /* Band 3B */
        if (band_number == 3)
            result = 0.423;
        /* Band 4 */
        if (band_number == 4)
            result = 0.1087;
        /* Band 5 */
        if (band_number == 5)
            result = 0.0348;
        /* Band 6 */
        if (band_number == 6)
            result = 0.0313;
        /* Band 7 */
        if (band_number == 7)
            result = 0.0299;
        /* Band 8 */
        if (band_number == 8)
            result = 0.0209;
        /* Band 9 */
        if (band_number == 9)
            result = 0.0159;
    }
    /* NORMAL GAIN */
    if (gain_code == 1) {
        /* Band 1 */
        if (band_number == 0)
            result = 1.688;
        /* Band 2 */
        if (band_number == 1)
            result = 1.415;
        /* Band 3N */
        if (band_number == 2)
            result = 0.862;
        /* Band 3B */
        if (band_number == 3)
            result = 0.862;
        /* Band 4 */
        if (band_number == 4)
            result = 0.2174;
        /* Band 5 */
        if (band_number == 5)
            result = 0.0696;
        /* Band 6 */
        if (band_number == 6)
            result = 0.0625;
        /* Band 7 */
        if (band_number == 7)
            result = 0.0597;
        /* Band 8 */
        if (band_number == 8)
            result = 0.0417;
        /* Band 9 */
        if (band_number == 9)
            result = 0.0318;
        /* Band 10 */
        if (band_number == 10)
            result = 0.006822;
        /* Band 11 */
        if (band_number == 11)
            result = 0.006780;
        /* Band 12 */
        if (band_number == 12)
            result = 0.006590;
        /* Band 13 */
        if (band_number == 13)
            result = 0.005693;
        /* Band 14 */
        if (band_number == 14)
            result = 0.005225;
    }

    /* LOW GAIN 1 */
    if (gain_code == 2) {
        /* Band 1 */
        if (band_number == 0)
            result = 2.25;
        /* Band 2 */
        if (band_number == 1)
            result = 1.89;
        /* Band 3N */
        if (band_number == 2)
            result = 1.15;
        /* Band 3B */
        if (band_number == 3)
            result = 1.15;
        /* Band 4 */
        if (band_number == 4)
            result = 0.290;
        /* Band 5 */
        if (band_number == 5)
            result = 0.0925;
        /* Band 6 */
        if (band_number == 6)
            result = 0.0830;
        /* Band 7 */
        if (band_number == 7)
            result = 0.0795;
        /* Band 8 */
        if (band_number == 8)
            result = 0.0556;
        /* Band 9 */
        if (band_number == 9)
            result = 0.0424;
    }
    /* LOW GAIN 2 */
    if (gain_code == 3) {
        /* Band 4 */
        if (band_number == 4)
            result = 0.290;
        /* Band 5 */
        if (band_number == 5)
            result = 0.409;
        /* Band 6 */
        if (band_number == 6)
            result = 0.390;
        /* Band 7 */
        if (band_number == 7)
            result = 0.332;
        /* Band 8 */
        if (band_number == 8)
            result = 0.245;
        /* Band 9 */
        if (band_number == 9)
            result = 0.265;
    }
    return result;
}
