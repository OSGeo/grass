#include<stdio.h>
#include<math.h>

double evap_fr(double r_net, double g0, double h0)
{
    return((r_net - g0 - h0) / (r_net - g0));
}
