#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* soil moisture in the root zone
     * Makin, Molden and Bastiaanssen, 2001
     */ 
double soilmoisture(double evap_fr) 
{
    return ((exp((evap_fr - 1.2836) / 0.4213)) / 0.511);
}


