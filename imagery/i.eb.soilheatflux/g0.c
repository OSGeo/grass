#include<stdio.h>
#include<math.h>
#include<stdlib.h>

double g_0(double bbalb, double ndvi, double tempk, double rnet,
	   double time, int roerink) 
{
    double a, b, result;
    double r0_coef;
    
    if (time <= 9.0 || time > 15.0) 
	r0_coef = 1.1;
    else if (time > 9.0 && time <= 11.0)
	r0_coef = 1.0;
    else if (time > 11.0 && time <= 13.0)
	r0_coef = 0.9;
    else if (time > 13.0 && time <= 15.0) 
	r0_coef = 1.0;
    a = (0.0032 * (bbalb / r0_coef) +
	  0.0062 * (bbalb / r0_coef) * (bbalb / r0_coef));
    b = (1 - 0.978 * pow(ndvi, 4));
    /* Spain (Bastiaanssen, 1995) */ 
    result = (rnet * (tempk - 273.15) / bbalb) * a * b;
    /* HAPEX-Sahel (Roerink, 1995) */ 
    if (roerink)
	result = result * 1.430 - 0.0845;

    return result;
}


