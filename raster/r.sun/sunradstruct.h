/******************************************************************************
r.sun: sunradstruct.h. This program was written by Jaro Hofierka in Summer 1993
 and re-engineered in 1996-1999. In cooperation with Marcel Suri and Thomas Huld
 from JRC in Ispra a new version of r.sun was prepared using ESRA solar
 radiation formulas.
See manual pages for details.
SPDX-FileCopyrightText: 2002 Jaro Hofierka
SPDX-FileCopyrightText: 2002 GeoModel, s.r.o.
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
*******************************************************************************/
/*v. 2.0 July 2002, NULL data handling, JH */
/*v. 2.1 January 2003, code optimization by Thomas Huld, JH */

#include <stdbool.h>

#define EPS       1.e-4
#define HOURANGLE M_PI / 12.

struct SunGeometryConstDay {
    double lum_C11;
    double lum_C13;
    double lum_C22;
    double lum_C31;
    double lum_C33;
    double sunrise_time;
    double sunset_time;
    double timeAngle;
    double sindecl;
    double cosdecl;
};

struct SunGeometryVarDay {
    int isShadow;
    double z_orig;
    double zmax;
    double zp;
    double solarAltitude;
    double sinSolarAltitude;
    double tanSolarAltitude;
    double solarAzimuth;
    double sunAzimuthAngle;
    double stepsinangle;
    double stepcosangle;
};

struct SunGeometryVarSlope {
    double longit_l; /* The "longitude" difference between the inclined */
    /* and orientated plane and the instantaneous solar position */
    double lum_C31_l;
    double lum_C33_l;
    double slope;
    double aspect;
    bool shift12hrs;
};

struct SolarRadVar {
    double cbh;
    double cdh;
    double linke;
    double G_norm_extra;
    double alb;
};

struct GridGeometry {
    double xp;
    double yp;
    double xx0;
    double yy0;
    double xg0;
    double yg0;
    double stepx;
    double stepy;
    double deltx;
    double delty;
    double stepxy;
    double sinlat;
    double coslat;
};
