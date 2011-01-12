#include<stdio.h>
#include<math.h>
#include<stdlib.h>

/* Prestely and Taylor, 1972. 
 * INPUT in Kelvin
 */
double pt_delta(double tempka)
{
    double a, b, result;

    tempka -= 273.15;		/*Celsius */
    b = tempka + 237.3;
    a = (17.27 * tempka) / b;
    result = 2504.0 * exp(a) / pow(b, 2);
    return result;
}
