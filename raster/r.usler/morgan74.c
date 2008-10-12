#include<stdio.h>
#include<math.h>
#include<stdlib.h>

double morgan_1974(double annual_pmm) 
{
    double result;

    result = (((annual_pmm * 9.28) - 8838.0) * 75.0) / 1000.0;
    return result;
}


