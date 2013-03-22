#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <math.h>
#include "local_proto.h"

extern CELL f_c(CELL);
extern FCELL f_f(FCELL);
extern DCELL f_d(DCELL);

/* constant definition */
/* #define k_sb 4.903    //[MJ/m2*h]             Stefan Bolzman constant  */
#define cp 1.013		/* [kJ/kg*°C]    specific heat of moist air */
#define epsilon 0.622		/* [-]                   ratio of molecular weigth of water to dry air */
#define Po 101.3		/* [kPa]                 atmospheric pressure at sea level */
#define Tko 293.16		/* [K]                   reference temperature at sea level */
#define eta 0.0065		/* [K/m]                 constant lapse rate */
#define Ao 0			/* [m]                   altitude at sea level */
#define g 9.81			/* [m/s]                 gravitational accelleration */
#define R 287			/* [J/kg*K]              specific gas constant */
#define Zw 2			/* [m]                   height of  wind measurements */
#define Zh 2			/* [m]                   height of  humidity measurements */
#define k 0.41			/* [-]                   Von Karman constant */



DCELL calc_ETp(DCELL T, DCELL Z, DCELL u2, DCELL Rn, int night, DCELL Rh,
	       DCELL hc)
{

    DCELL ea, delta, gamma, gstar, lambda;
    DCELL P, ra, d, Zom, Zoh, G, ETrad, u10, rs, ed, Tkv, rho, ETaero, ETp;

    /* calculus: mean saturation vapoure pressure [KPa] */
    ea = 0.61078 * exp((17.27 * T) / (T + 237.3));

    /* calculus: slope of vapoure pressure curve [KPa/°C] */
    delta = (4098 * ea) / pow((237.3 + T), 2);

    /* calculus: latent heat vapourisation [MJ/kg]  */
    lambda = 2.501 - (0.002361 * T);

    /* calculus: atmospheric pressure [KPa] */
    P = Po * pow(((Tko - eta * (Z - Ao)) / Tko), (g / (eta * R)));

    /* calculus: psichiometric constant [kPa/°C] */
    gamma = ((cp * P) / (epsilon * lambda)) * 0.001;

    /* calculus: aerodynamic resistance [s/m] */
    if (hc < 2) {
	d = (2 / 3) * hc;
	Zom = 0.123 * hc;
	Zoh = 0.1 * Zom;
	ra = (log((Zw - d) / Zom) * log((Zh - d) / Zoh)) / (k * k * u2);
    }
    else {
	u10 = u2 * (log((67.8 * 10) - 5.42)) / 4.87;
	ra = 94 / u10;
    }

    /* calculus: surface resistance [s/m]  */
    rs = 100 / (0.5 * 24 * hc);

    /*calculus: modified psichiometric constant [kPa/°C] */
    gstar = gamma * (1 + (rs / ra));

    /*calculus: net radiation [MJ/m2*d] */
    /*Rn derived from r.sun */

    /*calculus: soil heat flux [MJ/m2*d] */
    if (night == FALSE)
	G = 0.1 * Rn;
    else
	G = 0.5 * Rn;

    /* calculus: radiation term [mm/h] */
    /* ETrad = (delta/(delta+gstar))*((Rn-G)/(lambda*1000000)); */
    ETrad = (delta / (delta + gstar)) * ((Rn - G) / lambda);	/* torna da analisi dimensionale */

    /* calculus: actual vapoure pressure [kPa] */
    ed = Rh * ea / 100;

    /* calculus: virtual temperature [°C] */
    Tkv = (T + 273.15) / (1 - (0.378 * ed / P));

    /* calculus: atmospheric density [Kg/m^3] */
    rho = P / (Tkv * R / 100);

    /* calculus: aerodynamic term [mm/h] */
    /* ETaero = (0.001/lambda)*(1/(delta+gstar))*(rho*cp/ra)*(ea-ed); */
    ETaero = (3.6 / lambda) * (1 / (delta + gstar)) * (rho * cp / ra) * (ea - ed);	/* torna da analisi dimensionale */

    /* calculus: potential evapotranspiration [mm/h] */
    ETp = ETrad + ETaero;

    return ETp;

}

DCELL calc_openwaterETp(DCELL T, DCELL Z, DCELL u2, DCELL Rn, int day,
			DCELL Rh, DCELL hc)
{
    DCELL ea, delta, gamma, lambda;
    DCELL P, ed, ETaero, ETp;

    /* calculus: mean saturation vapoure pressure [KPa] */
    ea = 0.61078 * exp((17.27 * T) / (T + 237.3));

    /* calculus: slope of vapoure pressure curve [KPa/°C] */
    delta = (4098 * ea) / pow((237.3 + T), 2);

    /* calculus: latent heat vapourisation [MJ/kg]  */
    lambda = 2.501 - (0.002361 * T);

    /* calculus: atmospheric pressure [KPa] */
    P = Po * pow(((Tko - eta * (Z - Ao)) / Tko), (g / (eta * R)));

    /* calculus: di psichiometric constant [kPa/°C] */
    gamma = ((cp * P) / (epsilon * lambda)) * 0.001;

    /* calculus: net radiation [MJ/m2*h] */
    /* Rn derived from r.sun */

    /*calculus: actual vapoure pressure [kPa] */
    ed = Rh * ea / 100;

    /*calculus: aerodynamic term [mm/h] */
    /*ETaero = 0.35*(0.5+(0.621375*u2/100))*7.500638*(ea-ed); */
    /*to convert mm/d to mm/h it results: */
    ETaero =
	(0.35 / 24) * (0.5 + (0.621375 * u2 / 100)) * 7.500638 * (ea - ed);

    /*calculus: potential evapotranspiration [mm/h] */
    ETp = (((Rn * delta) / lambda) + (gamma * ETaero)) / (delta + gamma);

    return ETp;

}

#if 0
   DCELL calc_ETp_WaSiM(){


   /* calculus of saturation vapour pressure at the temperature T: es[hPa]  */
   /* with T[°C] */
   es   =       6.1078*exp((17.27*T)/(237.3+T);

   /* tangent of the saturated vapour pressure curve: D[hPa/K] with T[°C] */
   D    =       (25029/pow((273.3+T),2))*exp((17.27*T)/(237.3+T);

   /* air pressure at level hM */
   P    =       1013*exp(-hM/(7991+29.33*Tv));

   /* calculus of lambda [KJ/Kg] with T[°C] */
   lambda       =       2500.8 - 2.372*T;

   /* calculus psichiometric constant gamma [hPa/°C] with cp=1.005[KJ/(Kg*K)] */
   gamma        = ((1.005*P)/(0.622*lambda))*0.001; 
#endif
