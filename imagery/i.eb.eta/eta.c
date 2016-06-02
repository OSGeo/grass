#include<stdio.h>
#include<math.h>

double et_a(double r_net_day, double evap_fr, double tempk)
{
    double latent, t_celsius, result;

    t_celsius = tempk - 273.15;
    latent = (24.0*60.0*60.0) / ((2.501 - 0.002361 * t_celsius) * pow(10, 6));

    result = r_net_day * evap_fr * latent;

    return result;
}
