#include<stdio.h>
#include<math.h>
#include<stdlib.h>

double foster_1981(double annual_pmm) 
{
    double result;

    result = ((annual_pmm * 0.276 * 75.0) / 100.0);
    return result;
}


