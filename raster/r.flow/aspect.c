/*
 **  Original Algorithm:    H. Mitasova, L. Mitas, J. Hofierka, M. Zlocha 
 **  GRASS Implementation:  J. Caplan, M. Ruesink  1995
 **
 **  US Army Construction Engineering Research Lab, University of Illinois 
 **
 **  Copyright  M. Ruesink, J. Caplan, H. Mitasova, L. Mitas, J. Hofierka, 
 **     M. Zlocha  1995
 **
 **This program is free software; you can redistribute it and/or
 **modify it under the terms of the GNU General Public License
 **as published by the Free Software Foundation; either version 2
 **of the License, or (at your option) any later version.
 **
 **This program is distributed in the hope that it will be useful,
 **but WITHOUT ANY WARRANTY; without even the implied warranty of
 **MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **GNU General Public License for more details.
 **
 **You should have received a copy of the GNU General Public License
 **along with this program; if not, write to the Free Software
 **Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 **
 */


#include <math.h>
#include <grass/gis.h>
#include "r.flow.h"


DCELL aspect_fly(DCELL * n, DCELL * c, DCELL * s, double d)
{
    double xslope = ((n[-1] + c[-1] + c[-1] + s[-1]) -
		     (n[1] + c[1] + c[1] + s[1])) / (8 * d);
    double yslope = ((s[-1] + s[0] + s[0] + s[1]) -
		     (n[-1] + n[0] + n[0] + n[1])) / (8 * region.ns_res);
    double asp;

    if (!yslope)
	if (!xslope)
	    asp = UNDEF;
	else if (xslope > 0)
	    asp = parm.up ? 270. : 90.;
	else
	    asp = parm.up ? 90. : 270.;
    else if ((asp = atan2(xslope, yslope) / DEG2RAD) < 0.)
	asp += 360.;

    return asp;
}
