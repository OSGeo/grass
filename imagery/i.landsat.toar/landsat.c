#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>

#include "landsat.h"

#define PI  M_PI
#define R2D 180./M_PI
#define D2R M_PI/180.

/****************************************************************************
 * PURPOSE: Calibrated Digital Number to at-satellite Radiance
 *****************************************************************************/
double lsat_qcal2rad(double qcal, band_data * band)
{
    return (double)(qcal * band->gain + band->bias);
}

/****************************************************************************
 * PURPOSE: Radiance of non-thermal band to at-satellite Reflectance
 *****************************************************************************/
double lsat_rad2ref(double rad, band_data * band)
{
    return (double)(rad / band->K1);
}

/****************************************************************************
 * PURPOSE: Radiance of thermal band to at-satellite Temperature
 *****************************************************************************/
double lsat_rad2temp(double rad, band_data * band)
{
    return (double)(band->K2 / log((band->K1 / rad) + 1.));
}

/****************************************************************************
 * PURPOSE: Some band constants
 *
 *      zenith = 90 - sun_elevation
 *      sin( sun_elevation ) = cos( sun_zenith )
 *
 *      lsat : satellite data
 *         i : band number
 *    method : level of atmospheric correction
 *   percent : percent of solar irradiance in path radiance
 *      dark : digital number of  object for DOS
  *****************************************************************************/

#define abs(x)	(((x)>0)?(x):(-x))

void lsat_bandctes(lsat_data * lsat, int i, char method,
		   double percent, int dark, double rayleigh)
{
    double pi_d2, sin_e, cos_v;

    /* TAUv  = at. transmittance surface-sensor */
    /* TAUz  = at. transmittance sun-surface    */
    /* Edown = diffuse sky spectral irradiance  */
    double TAUv, TAUz, Edown;

    pi_d2 = (double)(PI * lsat->dist_es * lsat->dist_es);
    sin_e = (double)(sin(D2R * lsat->sun_elev));
    cos_v = (double)(cos(D2R * (lsat->number < 4 ? 9.2 : 8.2)));

    /** Global irradiance on the sensor.
	 * Radiance to reflectance coefficient, only NO thermal bands.
	 * K1 and K2 variables are also utilized as thermal constants
     */
    if (lsat->band[i].thermal == 0) {
	switch (method) {
	case DOS2:
	    {
		TAUv = 1.;
		TAUz = (lsat->band[i].wavemax < 1.) ? sin_e : 1.;
		Edown = 0.;
		break;
	    }
	case DOS2b:
	    {
		TAUv = (lsat->band[i].wavemax < 1.) ? cos_v : 1.;
		TAUz = (lsat->band[i].wavemax < 1.) ? sin_e : 1.;
		Edown = 0.;
		break;
	    }
	case DOS3:
	    {
		double t;

		t = 2. / (lsat->band[i].wavemax + lsat->band[i].wavemin);
		t = 0.008569 * t * t * t * t * (1 + 0.0113 * t * t + 0.000013 * t * t * t * t);
		TAUv = exp(-t / cos_v);
		TAUz = exp(-t / sin_e);
		Edown = rayleigh;
		break;
	    }
	case DOS4:
	    {
		double Ro =
		    (lsat->band[i].lmax - lsat->band[i].lmin) * (dark -
								 lsat->
								 band[i].
								 qcalmin) /
		    (lsat->band[i].qcalmax - lsat->band[i].qcalmin) +
		    lsat->band[i].lmin;
		double Tv = 1.;
		double Tz = 1.;
		double Lp = 0.;

		do {
		    TAUz = Tz;
		    TAUv = Tv;
		    Lp = Ro - percent * TAUv * (lsat->band[i].esun * sin_e * TAUz + PI * Lp) / pi_d2;
		    Tz = 1. - (4. * pi_d2 * Lp) / (lsat->band[i].esun * sin_e);
		    Tv = exp(sin_e * log(Tz) / cos_v);
		} while (TAUv != Tv && TAUz != Tz);
		TAUz = (Tz < 1. ? Tz : 1.);
		TAUv = (Tv < 1. ? Tv : 1.);
		Edown = (Lp < 0. ? 0. : PI * Lp);
		break;
	    }
	default:		/* DOS1 and Without atmospheric-correction */
	    TAUv = 1.;
	    TAUz = 1.;
	    Edown = 0.;
	    break;
	}
	lsat->band[i].K2 = 0.;
	lsat->band[i].K1 = TAUv * (lsat->band[i].esun * sin_e * TAUz + Edown) / pi_d2;
	if (method > DOS)
	    G_verbose_message("... TAUv = %.5f, TAUz = %.5f, Edown = %.5f\n", TAUv, TAUz, Edown);
    }

    /** Digital number to radiance coefficients.
	 * Without atmospheric calibration for thermal bands.
     */
    lsat->band[i].gain = (lsat->band[i].lmax - lsat->band[i].lmin) / (lsat->band[i].qcalmax - lsat->band[i].qcalmin);

    if (method == UNCORRECTED || lsat->band[i].thermal) {
	/* L = G * (DN - Qmin) + Lmin
	 *  -> bias = Lmin - G * Qmin    
	 */
	lsat->band[i].bias = (lsat->band[i].lmin - lsat->band[i].gain * lsat->band[i].qcalmin);
    }
    else if (method > DOS) {
	/* L = Lsat - Lpath = G * DNsat + B - (G *  + B - p * rad_sun) 
	 *   = G * DNsat - G *  + p * rad_sun
	 *  -> bias = p * rad_sun - G 
	 */
	lsat->band[i].bias = percent * lsat->band[i].K1 - lsat->band[i].gain * dark;
    }
}
