#include <stdio.h>
#include <grass/gis.h>

/* From FAOSOIL CD, after USDA 1951, p209 */

double tex2usle_k(int texture, double om_in)
{
    double usle_k = 200.0; /* Initial value */

    G_debug(1,"tex2usle_k: texture=%i, om=%5.3f", texture, om_in);
    if (om_in < 0.05) {
	if (texture == 0) /* G_message("clay"); */
	    usle_k = 0.29;	/*Took max value @0.2 */
	else if (texture == 1) /* G_message("sandy clay"); */
	    usle_k = 0.14;
	else if (texture == 2) /* G_message("silty clay"); */
	    usle_k = 0.25;
	else if (texture == 3) /* G_message("sandy clay loam"); */
	    usle_k = 0.27;
	else if (texture == 4) /* G_message("clay loam"); */
	    usle_k = 0.28;
	else if (texture == 5) /* G_message("silty clay loam"); */
	    usle_k = 0.37;
	else if (texture == 6) /* G_message("sand"); */
	    usle_k = 0.05;
	else if (texture == 7) /* G_message("loamy sand"); */
	    usle_k = 0.12;
	else if (texture == 8) /* G_message("sandy loam"); */
	    usle_k = 0.27;
	else if (texture == 9) /* G_message("loam"); */
	    usle_k = 0.38;
	else if (texture == 10) /* G_message("silt loam"); */
	    usle_k = 0.48;
	else if (texture == 11) /* G_message("silt"); */
	    usle_k = 0.60;
	else /*G_message("Unable to allocate class"); */
	    usle_k = 500.0;/*change value to show it was processed */
    }
    else if (om_in >= 0.05 && om_in < 0.2) {
	if (texture == 0) /* G_message("clay"); */
	    usle_k = 0.29; /*Range=[0.13-0.29]@0.2, took max */
	else if (texture == 1) /* G_message("sandy clay"); */
	    usle_k = 0.135;
	else if (texture == 2) /* G_message("silty clay"); */
	    usle_k = 0.24;
	else if (texture == 3) /* G_message("sandy clay loam"); */
	    usle_k = 0.26;
	else if (texture == 4) /* G_message("clay loam"); */
	    usle_k = 0.265;
	else if (texture == 5) /* G_message("silty clay loam"); */
	    usle_k = 0.345;
	else if (texture == 6) /* G_message("sand"); */
	    usle_k = 0.04;
	else if (texture == 7) /* G_message("loamy sand"); */
	    usle_k = 0.11;
	else if (texture == 8) /* G_message("sandy loam"); */
	    usle_k = 0.255;
	else if (texture == 9) /* G_message("loam"); */
	    usle_k = 0.36;
	else if (texture == 10) /* G_message("silt loam"); */
	    usle_k = 0.45;
	else if (texture == 11) /* G_message("silt"); */
	    usle_k = 0.56;
	else /*G_message("Unable to allocate class"); */
	    usle_k = 500.0;/*change value to show it was processed */
    }
    else if (om_in == 0.2) {
	if (texture == 0) /* G_message("clay"); */
	    usle_k = 0.22; /*Range=[0.13-0.29]@0.2, took average */
	else if (texture == 1) /* G_message("sandy clay"); */
	    usle_k = 0.13;
	else if (texture == 2) /* G_message("silty clay"); */
	    usle_k = 0.23;
	else if (texture == 3) /* G_message("sandy clay loam"); */
	    usle_k = 0.25;
	else if (texture == 4) /* G_message("clay loam"); */
	    usle_k = 0.25;
	else if (texture == 5) /* G_message("silty clay loam"); */
	    usle_k = 0.32;
	else if (texture == 6) /* G_message("sand"); */
	    usle_k = 0.03;
	else if (texture == 7) /* G_message("loamy sand"); */
	    usle_k = 0.10;
	else if (texture == 8) /* G_message("sandy loam"); */
	    usle_k = 0.24;
	else if (texture == 9) /* G_message("loam"); */
	    usle_k = 0.34;
	else if (texture == 10) /* G_message("silt loam"); */
	    usle_k = 0.42;
	else if (texture == 11) /* G_message("silt"); */
	    usle_k = 0.52;
	else /*G_message("Unable to allocate class"); */
	    usle_k = 500.0;/*change value to show it was processed */
    }
    else if (om_in > 0.2 && om_in < 0.4) {
	if (texture == 0) /* G_message("clay"); */
	    usle_k = 0.13;	/*Range=[0.13-0.29]@0.2, took min */
	else if (texture == 1) /* G_message("sandy clay"); */
	    usle_k = 0.125;
	else if (texture == 2) /* G_message("silty clay"); */
	    usle_k = 0.21;
	else if (texture == 3) /* G_message("sandy clay loam"); */
	    usle_k = 0.23;
	else if (texture == 4) /* G_message("clay loam"); */
	    usle_k = 0.23;
	else if (texture == 5) /* G_message("silty clay loam"); */
	    usle_k = 0.29;
	else if (texture == 6) /* G_message("sand"); */
	    usle_k = 0.025;
	else if (texture == 7) /* G_message("loamy sand"); */
	    usle_k = 0.09;
	else if (texture == 8) /* G_message("sandy loam"); */
	    usle_k = 0.215;
	else if (texture == 9) /* G_message("loam"); */
	    usle_k = 0.325;
	else if (texture == 10) /* G_message("silt loam"); */
	    usle_k = 0.375;
	else if (texture == 11) /* G_message("silt"); */
	    usle_k = 0.47;
	else /*G_message("Unable to allocate class"); */
	    usle_k = 500.0;/*change value to show it was processed */
    }
    else if (om_in >= 0.4) {/*May not be right (>4), no other data */
	if (texture == 0) /* G_message("clay\n"); */
	    usle_k = 0.13; /*took from value min @0.2 (table empty)*/
	else if (texture == 1) /* G_message("sandy clay\n"); */
	    usle_k = 0.12;
	else if (texture == 2) /* G_message("silty clay\n"); */
	    usle_k = 0.19;
	else if (texture == 3) /* G_message("sandy clay loam\n"); */
	    usle_k = 0.21;
	else if (texture == 4) /* G_message("clay loam\n"); */
	    usle_k = 0.21;
	else if (texture == 5) /* G_message("silty clay loam\n"); */
	    usle_k = 0.26;
	else if (texture == 6) /* G_message("sand\n"); */
	    usle_k = 0.02;
	else if (texture == 7) /* G_message("loamy sand\n"); */
	    usle_k = 0.08;
	else if (texture == 8) /* G_message("sandy loam\n"); */
	    usle_k = 0.19;
	else if (texture == 9) /* G_message("loam\n"); */
	    usle_k = 0.29;
	else if (texture == 10) /* G_message("silt loam\n"); */
	    usle_k = 0.33;
	else if (texture == 11) /* G_message("silt\n"); */
	    usle_k = 0.42;
	else /*G_message("Unable to allocate class"); */
	    usle_k = 500.0;/*change value to show it was processed */
    }
    return usle_k;
}
