#include<stdio.h>
#include<math.h>
#include<stdlib.h>

/*Prestely and Taylor, 1972. */
double pt_daily_et(double alpha_pt, double delta_pt, double ghamma_pt,
		   double rnet, double g0, double tempka)
{
    double result, latentHv, t_celsius;
    double roh_w = 1004.15;	/*mass density of water */
    double vap_slope_ratio;

    /*Latent Heat of vaporization (W/m2/d) */
    t_celsius = tempka - 273.15;
    latentHv = (24.0*60.0*60.0) / ((2.501 - 0.002361 * t_celsius) * pow(10, 6));

    /* Ratio of slope of saturation-vapour pressure Vs Temperature */
    /* ghamma_pt = psychrometric constant */
    vap_slope_ratio = delta_pt / (delta_pt + ghamma_pt);

    /*(Rn-g0)/latentHv returns [-] */
    result = (alpha_pt / roh_w) * vap_slope_ratio * (rnet - g0) / latentHv;
    return result;
}
