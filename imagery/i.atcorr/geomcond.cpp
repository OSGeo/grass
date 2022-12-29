extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "geomcond.h"
#include "common.h"

/* **********************************************************************c */
/*                                                                      c */
/*                                                *     sun             c */
/*                                              \ * /                   c */
/*                                            * * * * *                 c */
/*                                   z          / * \                   c */
/*                                   +           /+                     c */
/*            satellite    /         +          /                       c */
/*                       o/          +         /                        c */
/*                      /.\          +        /.                        c */
/*                     / . \  _avis-_+_-asol_/ .                        c */
/*                       .  \-      -+      /  .    north               c */
/*                       .   \       +     /   .  +                     c */
/*                       .    \      +    /    .+                       c */
/*                       .     \     +   /    +.                        c */
/*                       .      \    +  /   +  .                        c */
/*                       .       \   + /  +    .                        c */
/*                       .        \  +/ +      .                        c */
/*    west + + + + + + + . + + + + +\+ + + + + . + + + + + + + + east   c */
/*                       .          +..        .                        c */
/*                       .        + .   .      .                        c */
/*                       .      +  .      .    .                        c */
/*                       .    +   .       .'.  .                        c */
/*                       .  +    .. . , '     ..                        c */
/*                       .+     .       \       .                       c */
/*                      +.     .         \        .                     c */
/*                    +  .    .           \         .                   c */
/*             south     .   .       (phiv-phi0)                        c */
/*                                                                      c */
/*                                                                      c */
/*                                                                      c */
/* **********************************************************************c */


/*	To take into account the variation of the solar constant as a function 
  of the Julian day. 

  return dsol		
  dsol is a multiplicative factor to apply to the mean value of solar constant 
*/
double GeomCond::varsol ()
{
/* calculation of the variability of the solar constant during the year. 
   jday is the number of the day in the month   */
    long int j;
    if (month <= 2) j = (month - 1) * 31 + jday;
    else if (month > 8) j = (month - 1) * 31 - (month - 2) / 2 - 2 + jday;
    else j = (month - 1) * 31 - (month - 1) / 2 - 2 + jday;

/* Computing 2nd power */
    double tmp = 1.f - cos ((double) (j - 4) * 0.9856f * M_PI / 180.f) * .01673f;
    return 1.f / (double)(tmp * tmp);
}


/* spot, landsat5 and landsat7 is handled the same way */
void GeomCond::landsat(double tu)
{
/*     warning !!! */
/*     xlon and xlat are the coordinates of the scene center. */
    avis = 0.f;
    phiv = 0.f;
    possol(tu);
}

/*
  To compute the solar azimuthal and zenithal angles (in degrees) for a point over
  the globe defined by its longitude and its latitude (in dec. degrees) for a day of the year (fixed by
  number of the month and number of the day in the month) at any Greenwich Meridian Time (GMT
  dec. hour).
*/
void GeomCond::possol(double tu)
{
    long int ia = 0;
    long int nojour;
/*     solar position (zenithal angle asol,azimuthal angle phi0 */
/*                     in degrees) */
/*     jday is the number of the day in the month */
    day_number(ia, nojour);
    pos_fft (nojour, tu);
    if (asol > 90.f)
	G_warning(_("The sun is not raised"));
}

void GeomCond::day_number(long int ia, long int& j)
{
    if (month <= 2)
    {
	j = (month - 1) * 31 + jday;
	return;
    }

    if (month > 8) j = (month - 1) * 31 - (month - 2) / 2 - 2 + jday;
    else j = (month - 1) * 31 - (month - 1) / 2 - 2 + jday;

    if (ia != 0 && ia % 4 == 0) ++j;
}

/* returns the sign of the element */
#define SIGN(X) (((X) >= 0) ? 1. : -1.) 

void GeomCond::pos_fft (long int j, double tu)
{
    /* Local variables */
    double ah, et, az, caz, xla, tet, tsm, tsv, elev, azim, gdelta, amuzero;

    /*     solar position (zenithal angle asol,azimuthal angle phi0 */
    /*                     in degrees) */
    /*     j is the day number in the year */

    /* mean solar time (heure decimale) */
    tsm = tu + xlon / 15.;
    xla = xlat * M_PI / 180.;
    tet = (double)(j) * M_PI2 / 365.;

    /* time equation (in mn.dec) */
    et = 7.5e-5f + 0.001868f * cos (tet) - 0.032077f * sin (tet) - 
	0.014615f * cos (tet * 2.f) - 0.040849f * sin (tet * 2.f);

    et = et * 12.f * 60.f / M_PI;

    /* true solar time */
    tsv = tsm + et / 60.f;
    tsv += -12.f;

    /* hour angle */
    ah = tsv * 15.f * M_PI / 180.f;

    /* solar declination   (in radian) */
    gdelta = 0.006918f - 0.399912f * cos (tet) + 0.070257f * sin (tet) - 
	0.006758f * cos (tet * 2.f) + 9.07e-4f * sin (tet * 2.f) - 
	0.002697f * cos (tet * 3.f) + 0.00148f * sin (tet * 3.f);

    /* elevation,azimuth */
    amuzero = sin (xla) * sin (gdelta) + cos (xla) * cos (gdelta) * cos (ah);
    elev = asin (amuzero);
    az = cos (gdelta) * sin (ah) / cos (elev);
  
    if (fabs (az) - 1.f > 0.f) az = SIGN(az);

    caz = (-cos (xla) * sin (gdelta) + sin (xla) * cos (gdelta) * cos (ah)) / cos (elev);
    azim = asin (az);
    if (caz <= 0.f) azim = M_PI - azim;

    if (caz > 0.f && az <= 0.f) azim += M_PI2;

    azim += M_PI;
    if (azim > M_PI2) azim -= M_PI2;
	
    elev = elev * 180. / M_PI;
	
    /*     conversion in degrees */
    asol = (double)(90. - elev);
    phi0 = (double)(azim * 180. / M_PI);
}

/*
  convert:
  1 = meteosat observation 
  2 = goes east observation
  3 = goes west observation
*/
void GeomCond::posobs(double tu, int nc, int nl)
{
    double yr, xr, alti;

    if(igeom == 1) /* meteosat observation */
    {
	yr = nl - 1250.5;
	xr = nc - 2500.5;
	alti = 42164.0 - 6378.155;
    } 
    else if(igeom == 2) /* goes east observation */
    {
	yr = nl - 8665.5;
	xr = nc - 6498.5;
	alti = 42107.0 - 6378.155;
    }
    else /* goes west observation */
    {
	yr = nl - 8665.5;
	xr = nc - 6498.5;
	alti = 42147.0 - 6378.155;
    }


    const double re = 6378.155;
    const double aaa = 1. / 297.;
    const double rp = re / (1.f + aaa);
    const double cdr = M_PI / 180.;
    const double crd = 180. / M_PI;

    double deltax;
    double deltay;

    if(igeom == 1) 
    {
	deltax = 18.0 / 5000.0;
	deltay = 18.0 / 2500.0;
    }
    else
    {
	deltax = 18.0 / 12997.0;
	deltay = 20.0 / 17331.0;
    }

    double x = xr * deltax * cdr;
    double y = yr * deltay * cdr;
    double rs = re + alti;
    double tanx = tan(x);
    double tany = tan(y);
    double val1 = 1.0 + (tanx * tanx);
    double val2 = 1.0 + (tany * (1.0 + aaa)) * (tany * (1.0 + aaa));
    double yk = rs / re;
    double cosx2 = 1. / (val1 * val2);
      
    double sn, zt, xt, yt, teta, ylat, ylon;
    if((1. / cosx2) > ((yk * yk) / (yk*yk - 1.)))
    {
	G_warning(_("No possibility to compute lat. and long."));
	return;
    }
    else
    {
	sn = (rs - (re * (sqrt((yk * yk) - (yk*yk - 1.) * (1. / cosx2))))) / (1. / cosx2);
	zt = rs - sn;
	xt = -(sn * tanx);
	yt = sn * tany / cos(x);
	teta = asin(yt / rp);
	ylat = (atan(((tan(teta)) * rp) / re));
	ylon = atan(xt / zt);
    }
 
    xlat = (double)(ylat * crd);

    if(igeom == 1) xlon = (double)(ylon * crd);
    else if(igeom == 2) xlon = (double)(ylon * crd - 75.);
    else xlon = (double)(ylon * crd - 135.);
 
    possol(tu);
 
    if(igeom == 1) ylon = xlon * M_PI / 180.;
    else if(igeom == 2) ylon = xlon * M_PI / 180. + 75. * cdr;
    else ylon = xlon * M_PI / 180. + 135. * cdr;

    ylat = xlat * M_PI / 180.;
    double gam = sqrt(((1. / cosx2) - 1.) * cosx2);
    avis = (double)(asin((1. + alti / re) * (gam)) * 180. / M_PI);
    phiv = (double)((atan2(tan(ylon),sin(ylat)) + M_PI) * 180. / M_PI);
}

void GeomCond::posnoa(double tu, int nc, double xlonan, double campm, double hna)
{
/*     noaa 6 definition
       orbite inclination ai in radians
       hor mouv in rad/s  an
       h/r=860/6378
       campm allows the user to switch to pm platforms */
 
    const double r = 860. / 6378.155;
    const double ai = 98.96 * M_PI / 180.;
    const double an = 360. * M_PI / (6119. * 180.);
    double ylonan = xlonan * M_PI / 180.;
    double t = tu * 3600;
    double hnam = hna;
    hnam = hnam * 3600;
    double u = t - hnam;
    u = campm * u * an;
    double delt = ((nc - (2048 + 1) / 2.) * 55.385 / ((2048. - 1) / 2.));
    delt = campm * delt * M_PI / 180.;
    avis = (double)asin((1 + r) * sin(delt));
    double d = avis - delt;
    double y = cos(d) * cos(ai) * sin(u) - sin(ai) * sin(d);
    double z = cos(d) * sin(ai) * sin(u) + cos(ai) * sin(d);
    double ylat = asin(z);
    double cosy = cos(d) * cos(u) / cos(ylat);

    double siny = y / cos(ylat);
    double ylon = asin(siny);
    if(cosy <= 0.)
    {
	if(siny > 0) ylon = M_PI - ylon;
	if(siny <= 0) ylon = -(M_PI + ylon);
    }
    double ylo1 = ylon + ylonan - (t - hnam) * 2. * M_PI / 86400.;
    xlat = (double)(ylat * 180. / M_PI);
    xlon = (double)(ylo1 * 180. / M_PI);
 


    possol(tu);
 
    double zlat = asin(sin(ai) * sin(u));
    double zlon = atan2(cos(ai) * sin(u),cos(u));
    if(nc != 1024)
    {
	double xnum = sin(zlon - ylon) * cos(zlat) / sin(fabs(d));
	double xden = (sin(zlat) - sin(ylat) * cos(d)) / cos(ylat) / sin(fabs(d));
	phiv = (double)atan2(xnum,xden);
    }
    else phiv = 0.;
    phiv = (double)(phiv * 180. / M_PI);
    avis = (double)(fabs(avis) * 180. / M_PI);
}

void GeomCond::parse()
{
    cin >> igeom;
    cin.ignore(numeric_limits<int>::max(),'\n');  /* read the rest of the scraps, like comments */

    double campm = -1.0f;	/* initialize in case igeom == 5 */
    double tu, xlonan, hna;
    int nc, nl;

    switch(igeom)
    {
    case 0: /* internal format */
    {
	cin >> asol;
	cin >> phi0;
	cin >> avis;
	cin >> phiv;
	cin >> month;
	cin >> jday;
	cin.ignore(numeric_limits<int>::max(),'\n');  /* read the rest of the scraps, like comments */
	break;
    }
    case 1: /* meteosat observation */
    case 2: /* goes east observation */
    case 3: /* goes west observation  */
    {
	cin >> month;
	cin >> jday;
	cin >> tu;
	cin >> nc;
	cin >> nl;
	cin.ignore(numeric_limits<int>::max(),'\n');
	posobs(tu, nc, nl);
	break;
    }
    case 4: campm = 1.0f; /* avhrr PM, case 4 must fall through to case 5 */
    case 5:  		  /* avhrr PM and avhrr AM */
    {
	cin >> month;
	cin >> jday;
	cin >> tu;
	cin >> nc;
	cin >> xlonan;
	cin >> hna;
	cin.ignore(numeric_limits<int>::max(),'\n');
	posnoa(tu, nc, xlonan, campm, hna);
	break;
    }
    case 6: /* hrv   ( spot )    * enter month,day,hh.ddd,long.,lat. */
    case 7: /* tm    ( landsat ) * enter month,day,hh.ddd,long.,lat. */
    case 8: /* etm+  ( landsat7) * enter month,day,hh.ddd,long.,lat. */
    case 9: /* liss  ( IRS 1C)   * enter month,day,hh.ddd,long.,lat. */
    case 10: /* aster            * enter month,day,hh.ddd,long.,lat. */
    case 11: /* avnir            * enter month,day,hh.ddd,long.,lat. */
    case 12: /* ikonos           * enter month,day,hh.ddd,long.,lat. */
    case 13: /* rapideye         * enter month,day,hh.ddd,long.,lat. */
    case 14: /* vgt1_spot4       * enter month,day,hh.ddd,long.,lat. */
    case 15: /* vgt2_spot5       * enter month,day,hh.ddd,long.,lat. */
    case 16: /* worldview2       * enter month,day,hh.ddd,long.,lat. */
    case 17: /* quickbird2       * enter month,day,hh.ddd,long.,lat. */
    case 18: /* Landsat 8        * enter month,day,hh.ddd,long.,lat. */
    case 19: /* geoeye1          * enter month,day,hh.ddd,long.,lat. */
    case 20: /* spot6            * enter month,day,hh.ddd,long.,lat. */
    case 21: /* spot7            * enter month,day,hh.ddd,long.,lat. */
    case 22: /* pleiades1a       * enter month,day,hh.ddd,long.,lat. */
    case 23: /* pleiades1b       * enter month,day,hh.ddd,long.,lat. */
    case 24: /* worldview3       * enter month,day,hh.ddd,long.,lat. */
    case 25: /* sentinel2a       * enter month,day,hh.ddd,long.,lat. */
    case 26: /* sentinel2b       * enter month,day,hh.ddd,long.,lat. */
    case 27: /* planetscope0c0d  * enter month,day,hh.ddd,long.,lat. */
    case 28: /* planetscope0e    * enter month,day,hh.ddd,long.,lat. */
    case 29: /* planetscope0f10  * enter month,day,hh.ddd,long.,lat. */
    case 30: /* worldview4       * enter month,day,hh.ddd,long.,lat. */
   {
	cin >> month;
	cin >> jday;
	cin >> tu;
	cin >> xlon;
	cin >> xlat;
	cin.ignore(numeric_limits<int>::max(),'\n');  /* read the rest of the scraps, like comments */
	landsat(tu);
	break;
    }
    default: G_fatal_error(_("Unsupported/unreadable format in control file (found igeom=%ld)"), igeom);
    }


    /* ********************************************************************** */
    /*                                                                        */
    /*                                 / scattered direction                  */
    /*                               /                                        */
    /*                             /                                          */
    /*                           / adif                                       */
    /*    incident   + + + + + + + + + + + + + + +                            */
    /*    direction                                                           */
    /*                                                                        */
    /* ********************************************************************** */
    phi = (double)fabs(phiv - phi0);
    phirad = (phi0 - phiv) * (double)M_PI / 180.f;
    if (phirad < 0.f) phirad += (double)M_PI2;
    if (phirad > M_PI2) phirad -= (double)M_PI2;

    xmus = (double)cos (asol * M_PI / 180.f);
    xmuv = (double)cos (avis * M_PI / 180.f);
    xmup = (double)cos (phirad);
    xmud = -xmus * xmuv - (double)sqrt (1.f - xmus * xmus) * (double)sqrt (1.f - xmuv * xmuv) * xmup;

    /* test vermote bug */
    if (xmud > 1.f)  xmud = 1.f;
    if (xmud < -1.f) xmud = -1.f;
    adif = (double)acos (xmud) * 180.f / (double)M_PI;

    dsol = varsol();
}

/* ---- print geometrical conditions ---- */
void GeomCond::print()
{
    static const string etiq1[] = {
	string(" user defined conditions     "),
	string(" Meteosat observation        "),
	string(" GOES east observation       "),
	string(" GOES west observation       "),
	string(" AVHRR (AM noaa) observation "),
	string(" AVHRR (PM noaa) observation "),
	string(" H.R.V.   observation        "),
	string(" T.M.     observation        "),
	string(" ETM+     observation        "),
	string(" LISS     observation        "),
	string(" ASTER    observation        "),
	string(" AVNIR    observation        "),
	string(" Ikonos   observation        "),
	string(" Rapideye observation        "),
	string(" VGT1-SPOT4 observation      "),
	string(" VGT2-SPOT5 observation      "),
	string(" Worldview2 observation      "),
	string(" Quickbird2 observation      "),
	string(" Landsat 8 observation       "),
	string(" geoeye1  observation        "),
	string(" spot6 observation           "),
	string(" spot7 observation           "),
	string(" pleiades1a observation      "),
	string(" pleiades1b observation      "),
	string(" worldview3 observation      "),
	string(" sentinel2a observation      "),
	string(" sentinel2b observation      "),
	string(" planetscope 0c 0d observation"),
	string(" planetscope 0e observation  "),
	string(" planetscope 0f 10 observation"),
	string(" worldview4 observation      ")
	};

    static const string head(" geometrical conditions identity  ");
    static const string line(" -------------------------------  ");
    Output::Begin(); Output::Repeat(22,' '); Output::Print(head); Output::End();
    Output::Begin(); Output::Repeat(22,' '); Output::Print(line); Output::End();

	
    Output::Begin(); Output::Repeat(22,' '); Output::Print(etiq1[igeom]); Output::End();
    Output::Begin(); Output::End();

	
    Output::Begin(); Output::Repeat(2,' ');
    ostringstream s1;
    s1.setf(ios::fixed, ios::floatfield);
    s1 << " month: " << month << " day: " << jday;
    s1 << ends;
    Output::Print(s1.str());
    Output::End();


    Output::Begin(); Output::Repeat(2,' ');
    ostringstream s2;
    s2.setf(ios::fixed, ios::floatfield);
    s2 << setprecision(2);


    s2 << " solar zenith angle:  " << setw(6) << asol << " deg ";
    s2 << " solar azimuthal angle:      " << setw(6) << phi0 << " deg";
    s2 << ends;
    Output::Print(s2.str());
    Output::End();

	
    Output::Begin(); Output::Repeat(2,' ');
    ostringstream s3;
    s3.setf(ios::fixed, ios::floatfield);
    s3 << setprecision(2);
    s3 << " view zenith angle:   " << setw(6) << avis << " deg ";
    s3 << " view azimuthal angle:       " << setw(6) << phiv << " deg ";
    s3 << ends;
    Output::Print(s3.str());
    Output::End();
    Output::Begin(); Output::Repeat(2,' ');
    ostringstream s4;
    s4.setf(ios::fixed, ios::floatfield);
    s4 << setprecision(2);
    s4 << " scattering angle:    " << setw(6) << adif << " deg ";
    s4 << " azimuthal angle difference: " << setw(6) << phi << " deg ";
    s4 << ends;
    Output::Print(s4.str());
    Output::End();
	
    Output::Begin(); Output::End();
}

GeomCond GeomCond::Parse()
{
    GeomCond geom;
    geom.parse();
    return geom;
}
