
/*******************************************************************************
r.sun: rsunglobals.h. This program was written by Jaro Hofierka in Summer 1993 and re-engineered
in 1996-1999. In cooperation with Marcel Suri and Thomas Huld from JRC in Ispra
a new version of r.sun was prepared using ESRA solar radiation formulas.
See manual pages for details.
(C) 2002 Copyright Jaro Hofierka, Gresaka 22, 085 01 Bardejov, Slovakia, 
              and GeoModel, s.r.o., Bratislava, Slovakia
email: hofierka@geomodel.sk,marcel.suri@jrc.it,suri@geomodel.sk
*******************************************************************************/
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*v. 2.0 July 2002, NULL data handling, JH */
/*v. 2.1 January 2003, code optimization by Thomas Huld, JH */

#define EARTHRADIUS 6371000.
/* undefined value for terrain aspect */
#define UNDEF    0.
/* internal undefined value for NULL */
#define UNDEFZ   -9999.

/* Constant for calculating angular loss */
#define a_r 0.155

extern int varCount_global;
extern int bitCount_global;
extern int arrayNumInt;

/*
   extern double xp;
   extern double yp;
 */

extern double angular_loss_denom;

extern const double invScale;
extern const double pihalf;
extern const double pi2;
extern const double deg2rad;
extern const double rad2deg;

extern double solar_constant;

extern struct pj_info iproj;
extern struct pj_info oproj;


extern void (*func) (int, int);
