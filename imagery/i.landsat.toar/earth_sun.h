/*
 *  Modified from LIBNOVA-0.12: see earth-sun.c
 *
 *  Copyright (C) 2000 - 2005 Liam Girdwood
 *  Modified to GRASS (C) 2006 E. Jorge Tizado
 */

#ifndef _EARTH_SUN_H
#define _EARTH_SUN_H

struct ln_vsop
{
    double A;
    double B;
    double C;
};

/* Local prototypes */

double ln_calc_series(const struct ln_vsop *data, int terms, double t);
double julian_int(int year, int month, int day);
double julian_char(char date[]);
double earth_sun(char date[]);

#endif
