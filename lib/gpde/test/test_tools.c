
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests of math tools 
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/N_pde.h>
#include "test_gpde_lib.h"


/* prototypes */
static int test_mean_calc(void);

/* *************************************************************** */
/* Perfrome the math tool tests ********************************** */
/* *************************************************************** */
int unit_test_tools(void)
{
    int sum = 0;

    G_message("\n++ Running math tool unit tests ++");

    sum += test_mean_calc();

    if (sum > 0)
	G_warning("\n-- math tool unit tests failure --");
    else
	G_message("\n-- math tool unit tests finished successfully --");

    return sum;
}

/* *************************************************************** */
/* Test the mean calculation functions *************************** */
/* *************************************************************** */
int test_mean_calc(void)
{
  double a, b, mean_n, mean, vector, distance, D, weight;
  double v[2];
  double array[10] = {0,0,0,0,0,0,0,0,0,0};
  int i;
  int sum = 0;
  char buff1[10];

  for(i = 0; i < 10; i++)
  array[i] += i; 

  a = 1.0/3.0;
  b = 3.0;
  v[0] = a; v[1] = b;

  /*arith mean*/
  mean = N_calc_arith_mean(a, b);
  G_message("N_calc_arith_mean: calc a %g and b %g = %12.18lf", a, b, mean);
  mean_n = N_calc_arith_mean_n(v, 2);
  G_message("N_calc_arith_mean_n: calc a %g and b %g = %12.18lf", v[0], v[1], mean_n);
  if(mean != mean_n)
  	sum++;

  /*geom mean*/
  mean = N_calc_geom_mean(a, b);
  G_message("N_calc_geom_mean: calc a %g and b %g = %12.18lf", a, b, mean);
  mean_n = N_calc_geom_mean_n(v, 2);
  G_message("N_calc_geom_mean_n: calc a %g and b %g = %12.18lf", v[0], v[1], mean_n);
  if(mean != mean_n)
  	sum++;

  /*harmonic mean*/
  mean = N_calc_harmonic_mean(a, b);
  G_message("N_calc_harmonic_mean: calc a %g and b %g = %12.18lf", a, b, mean);
  mean_n = N_calc_harmonic_mean_n(v, 2);
  G_message("N_calc_harmonic_mean_n: calc a %g and b %g = %12.18lf", v[0], v[1], mean_n);
  if(mean != mean_n)
  	sum++;
  /*null test*/
  a = 2;
  b = 0;
  v[0] = a; v[1] = b;
  mean = N_calc_harmonic_mean(a, b);
  G_message("N_calc_harmonic_mean: calc a %g and b %g = %12.18lf", a, b, mean);
  mean_n = N_calc_harmonic_mean_n(v, 2);
  G_message("N_calc_harmonic_mean_n: calc a %g and b %g = %12.18lf", v[0], v[1], mean_n);
  if(mean != mean_n)
  	sum++;
 
  /*quadratic mean*/
  a = 1.0/3.0;
  b = 3.0;
  v[0] = a; v[1] = b;

  mean = N_calc_quad_mean(a, b);
  G_message("N_calc_quad_mean: calc a %g and b %g = %12.18lf", a, b, mean);
  mean_n = N_calc_quad_mean_n(v, 2);
  G_message("N_calc_quad_mean_n: calc a %g and b %g = %12.18lf", v[0], v[1], mean_n);
  if(mean != mean_n)
  	sum++;

  /*Test the full upwind stabailization*/
  vector= -0.000001;
  distance = 20;
  D = 0.000001;

  weight = N_full_upwinding(vector, distance, D);
  G_message("N_full_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);

  if(weight != 0)
  {
	G_warning("Error detected in N_full_upwinding");
	sum++;
  }

  vector= 0.000001;

  weight = N_full_upwinding(vector, distance, D);
  G_message("N_full_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);
  if(weight != 1)
  {
	G_warning("Error detected in N_full_upwinding");
	sum++;
  }

  D = 0.0;

  weight = N_full_upwinding(vector, distance, D);
  G_message("N_full_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);
  if(weight != 0.5)
  {
	G_warning("Error detected in N_full_upwinding");
	sum++;
  }


  /*Test the exponential upwind stabailization*/
  vector= -0.000001;
  distance = 20;
  D = 0.000001;

  weight = N_exp_upwinding(vector, distance, D);
  G_message("N_exp_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);
  sprintf(buff1, "%1.2lf", weight);
  sscanf(buff1, "%lf", &weight);

  if(weight != 0.05)
  {
	G_warning("Error detected in N_exp_upwinding");
	sum++;
  }

  vector= 0.000001;

  weight = N_exp_upwinding(vector, distance, D);
  G_message("N_exp_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);
  sprintf(buff1, "%1.2lf", weight);
  sscanf(buff1, "%lf", &weight);

  if(weight != 0.95)
  {
	G_warning("Error detected in N_exp_upwinding");
	sum++;
  }

  D = 0.0;

  weight = N_exp_upwinding(vector, distance, D);
  G_message("N_exp_upwinding: vector %g distance %g D %g weight %g\n", vector, distance, D, weight);
  if(weight != 0.5)
  {
	G_warning("Error detected in N_exp_upwinding");
	sum++;
  }

  return sum;
}
