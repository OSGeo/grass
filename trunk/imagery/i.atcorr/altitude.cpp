#include "common.h"
#include "altitude.h"
#include "atmosmodel.h"
#include "aerosolconcentration.h"

/* Update the atmospheric profile (P(z),T(z),H2O(z),O3(z)) in case the target 
   is not at sea level.


   Given the altitude of the target in kilometers as input, we transform the
   original atmospheric profile (Pressure, Temperature, Water Vapor, Ozone) 
   so that first level of the new profile is the one at the target altitude. 
   We also compute the new integrated content in water vapor and ozone, that
   are used as outputs or in computations when the user chooses to enter a
   specific amount of Ozone and Water Vapor.
*/
void Altitude::pressure(AtmosModel& atms, double& uw, double& uo3)
{
    /* log linear interpolation */
    if(xps >= 100) xps = 99.99f;
		
    int i;
    for(i = 0; atms.z[i] <= xps; i++);
		
    int isup = i;
    int iinf = i - 1;

    double xa = (double)((atms.z[isup] - atms.z[iinf]) / log(atms.p[isup] / atms.p[iinf]));
    double xb = (double)(atms.z[isup] - xa * log(atms.p[isup]));
    double ps = (double)exp((xps - xb) / xa);

    /* interpolating temperature wator vapor and ozone profile versus altitude */
    double xalt = xps;
    double xtemp = (atms.t[isup] - atms.t[iinf]) / (atms.z[isup] - atms.z[iinf]);
    xtemp = xtemp * (xalt - atms.z[iinf]) + atms.t[iinf];
    double xwo = (atms.wo[isup] - atms.wo[iinf]) / (atms.z[isup] - atms.z[iinf]);
    xwo = xwo * (xalt - atms.z[iinf]) + atms.wo[iinf];
    double xwh = (atms.wh[isup] - atms.wh[iinf]) / (atms.z[isup] - atms.z[iinf]);
    xwh = xwh * (xalt - atms.z[iinf]) + atms.wh[iinf];

    /* updating atmospheric profile
       1rst level: target     , complete to 34
       with interpolated layers */
    atms.z[0] = xalt;                                                          
    atms.p[0] = ps;
    atms.t[0] = xtemp;
    atms.wh[0] = xwh;
    atms.wo[0] = xwo;

    for (i = 1; i < 33 - iinf; ++i)
    {
	atms.z[i] = atms.z[i + iinf];
	atms.p[i] = atms.p[i + iinf];
	atms.t[i] = atms.t[i + iinf];
	atms.wh[i] = atms.wh[i + iinf];
	atms.wo[i] = atms.wo[i + iinf];
    }
    int l = 33 - iinf - 1;
    for (i = l; i < 34; ++i)
    {
	atms.z[i] = (atms.z[33] - atms.z[l]) * (i - l) / (33 - l) + atms.z[l];
	atms.p[i] = (atms.p[33] - atms.p[l]) * (i - l) / (33 - l) + atms.p[l];
	atms.t[i] = (atms.t[33] - atms.t[l]) * (i - l) / (33 - l) + atms.t[l];
	atms.wh[i] = (atms.wh[33] - atms.wh[l]) * (i - l) / (33 - l) + atms.wh[l];
	atms.wo[i] = (atms.wo[33] - atms.wo[l]) * (i - l) / (33 - l) + atms.wo[l];
    }

    /* compute modified h2o and o3 integrated content */
    uw = 0;
    uo3 = 0;
    const double g = 98.1f;
    const double air = 0.028964f/0.0224f;
    const double ro3 = 0.048f/0.0224f;

    double rmwh[34];
    double rmo3[34];
    int k;
    double roair, ds;

    k = 0;
    roair = air * 273.16f * atms.p[k] / (atms.t[k] * 1013.25f);
    rmwh[k] = atms.wh[k] / (roair * 1e3f);
    rmo3[k] = atms.wo[k] / (roair * 1e3f);

    for (k = 1; k < 33; ++k)
    {
	roair = air * 273.16f * atms.p[k] / (atms.t[k] * 1013.25f);
	rmwh[k] = atms.wh[k] / (roair * 1e3f);
	rmo3[k] = atms.wo[k] / (roair * 1e3f);

	ds = (atms.p[k - 1] - atms.p[k]) / (atms.p[0] * 2);
	uw += (rmwh[k] + rmwh[k - 1]) * ds;
	uo3 += (rmo3[k] + rmo3[k - 1]) * ds;
    }
    uw = uw * atms.p[0] * 100.f / g;
    uo3 = uo3 * atms.p[0] * 100.f / g;
    uo3 = uo3 * 1e3f / ro3;
}

/*
  Function: Update the atmospheric profile (P(z),T(z),H2O(z),O3(z)) in case the observer is on
  board an aircraft.

  Description: Given the altitude or pressure at aircraft level as input, the first task is to
  compute the altitude (in case the pressure has been entered) or the pressure (in case the altitude has
  been entered) at plane level. Then, a new atmospheric profile is created (Pp,Tp,H2Op,O3p) for which
  the last level is located at the plane altitude. This profile is used in the gaseous absorption
  computation (ABSTRA.f) for the path from target to sensor (upward transmission). The ozone and
  water vapor integrated content of the "plane" atmospheric profile are also an output of this
  subroutine. The last output is the proportion of molecules below plane level which is useful in
  scattering computations (OS.f,ISO.f).
*/
void Altitude::presplane(AtmosModel& atms)
{
    /* log linear interpolation */
    xpp += atms.z[0];
    if(xpp >= 100) xpp = 1000;

    int i;
    for(i = 0; atms.z[i] <= xpp; i++);

    int isup = i;
    int iinf = i-1;

    double xa = (double)((atms.z[isup] - atms.z[iinf]) / log(atms.p[isup] / atms.p[iinf]));
    double xb = (double)(atms.z[isup] - xa * log(atms.p[isup]));
    double ps = (double)(exp((xpp - xb) / xa));

    /* interpolating temperature wator vapor and ozone profile versus altitud */
    double xalt = xpp;
    double xtemp  = (atms.t[isup] - atms.t[iinf])/ (atms.z[isup] - atms.z[iinf]);
    xtemp = xtemp * (xalt - atms.z[iinf]) + atms.t[iinf];
    double xwo = (atms.wo[isup] - atms.wo[iinf]) / (atms.z[isup] - atms.z[iinf]);
    xwo =  xwo * (xalt - atms.z[iinf]) + atms.wo[iinf];
    double xwh = (atms.wh[isup] - atms.wh[iinf]) / (atms.z[isup] - atms.z[iinf]);
    xwh =  xwh * (xalt - atms.z[iinf]) + atms.wh[iinf];

    /* updating atmospheric profile
       last level: plane     , complete to 34
       with interpolated layers */
    for(i = 0; i <= iinf; i++)
    {
	plane_sim.zpl[i] = atms.z[i];
	plane_sim.ppl[i] = atms.p[i];
	plane_sim.tpl[i] = atms.t[i];
	plane_sim.whpl[i] = atms.wh[i];
	plane_sim.wopl[i] = atms.wo[i];
    }

    for(i = iinf+1; i < 34; i++)
    {
	plane_sim.zpl[i] = xalt;
	plane_sim.ppl[i] = ps;
	plane_sim.tpl[i] = xtemp;
	plane_sim.whpl[i] = xwh;
	plane_sim.wopl[i] = xwo;
    }

    /* compute modified h2o and o3 integrated content
       compute conversion factor for rayleigh optical thickness computation
       ftray=rp/rt */
    atms.uw = 0;
    atms.uo3 = 0;
    const double g = 98.1f;
    const double air = 0.028964f/0.0224f;
    const double ro3 = 0.048f/0.0224f;
    double rt = 0;
    double rp = 0;

    double rmo3[34];
    double rmwh[34];
    int k;
    for(k = 0; k < 33; k++)
    {
	double roair = (double)(air * 273.16 * plane_sim.ppl[k] / (1013.25 * plane_sim.tpl[k]));
	rmwh[k] = atms.wh[k] / (roair * 1000);
	rmo3[k] = atms.wo[k] / (roair * 1000);
	rt += (atms.p[k+1] / atms.t[k+1] + atms.p[k] / atms.p[k]) * (atms.z[k+1] - atms.z[k]);
	rp += (plane_sim.ppl[k+1] / plane_sim.tpl[k+1] + plane_sim.ppl[k] / plane_sim.tpl[k]) 
	    * (plane_sim.zpl[k+1] - plane_sim.zpl[k]);
    }

    ftray = rp / rt;
    for(k = 1; k < 33; k++)
    {
	double ds = (plane_sim.ppl[k-1] - plane_sim.ppl[k]) / plane_sim.ppl[0];
	atms.uw += (rmwh[k] + rmwh[k-1])*ds/2;
	atms.uo3+= (rmo3[k] + rmo3[k-1])*ds/2;
    }

    atms.uw *= plane_sim.ppl[0] * 100 / g;
    atms.uo3*= plane_sim.ppl[0] * 100 / g;
    atms.uo3*= 1000 / ro3;
}

void Altitude::init(AtmosModel &atms, const AerosolConcentration &aerocon)
{
    xps = original_xps;
    xpp = original_xpp;

    double uwus;
    double uo3us;
    if(xps <= 0)
    {
	xps = 0;
	uwus = 1.424f;
	uo3us = 0.344f;
    }
    else if(atms.idatm != 8) pressure(atms, atms.uw, atms.uo3);
    else pressure(atms, uwus, uo3us);

    if(xpp <= 0)
    {
	/* ground measurement option */
	palt = 0;
	pps = atms.p[0];
	idatmp = 0;
	original_taer55p = taer55p = 0;
	puw = 0;
    }
    else if(xpp >= 100)
    {
	/* satellite case of equivalent */
	palt = 1000;
	pps = 0;
	original_taer55p = taer55p = aerocon.taer55;
	puw = 0;
	ftray = 1;
	idatmp = 4;
    }
    else
    {
	/* "real" plane case */
	cin >> original_puw;
	cin >> original_puo3;
	cin.ignore(numeric_limits < int >::max(), '\n');	/* ignore comments */

	puw = original_puw;
	puo3 = original_puo3;
	if ( puw < 0 )
	{
	    presplane(atms);
	    idatmp = 2;

	    if (atms.idatm == 8)
	    {
		puwus = puw;
		puo3us = puo3;
		puw *= atms.uw / uwus;
		puo3 *= atms.uo3 / uo3us;
		idatmp = 8;
	    }
	}
	else
	{
	    presplane(atms);
	    idatmp = 8;
	}

	palt = plane_sim.zpl[33] - atms.z[0];
	pps = plane_sim.ppl[33];
	cin >> original_taer55p;
	taer55p = original_taer55p;

	if ((taer55p > 0) || ((aerocon.taer55 - taer55p) < 1e-03))
	{
	    /* a scale heigh of 2km is assumed in case no value is given for taer55p */
	    taer55p = (double)(aerocon.taer55 * (1 - exp(-palt / 2)));
	}
	else
	{
	    /* compute effective scale heigh */
	    double sham = exp(-palt / 4);
	    double sha = 1 - (taer55p / aerocon.taer55);

	    if( sha >= sham) taer55p = (double)(aerocon.taer55 * (1 - exp(-palt / 4)));
	    else {
		sha = -palt/log(sha);
		taer55p = (double)(aerocon.taer55 * (1 - exp(-palt/sha)));
	    }
	}
    }
}

void Altitude::update_hv(AtmosModel & atms, const AerosolConcentration & aerocon)
{
    xps = original_xps;
    xpp = original_xpp;

    double uwus;
    double uo3us;

    if (xps <= 0) {
	xps = 0;
	uwus = 1.424f;
	uo3us = 0.344f;
    }
    else if (atms.idatm != 8)
	pressure(atms, atms.uw, atms.uo3);
    else
	pressure(atms, uwus, uo3us);

    if (xpp <= 0) {
	/* ground measurement option */
	palt = 0;
	pps = atms.p[0];
	idatmp = 0;
	taer55p = 0;
	puw = 0;
    }
    else if (xpp >= 100) {
	/* satellite case of equivalent */
	palt = 1000;
	pps = 0;
	taer55p = aerocon.taer55;
	puw = 0;
	ftray = 1;
	idatmp = 4;
    }
    else {
	/* "real" plane case */

	puw = original_puw;
	puo3 = original_puo3;

	if (puw < 0) {
	    presplane(atms);
	    idatmp = 2;

	    if (atms.idatm == 8) {
		puwus = puw;
		puo3us = puo3;
		puw *= atms.uw / uwus;
		puo3 *= atms.uo3 / uo3us;
		idatmp = 8;
	    }
	}
	else {
	    presplane(atms);
	    idatmp = 8;
	}

	palt = plane_sim.zpl[33] - atms.z[0];
	pps = plane_sim.ppl[33];
	taer55p = original_taer55p;

	if ((taer55p > 0) || ((aerocon.taer55 - taer55p) < 1e-03)) {
	    /* a scale heigh of 2km is assumed in case no value is given for taer55p */
	    taer55p = (double)(aerocon.taer55 * (1 - exp(-palt / 2)));
	}
	else {
	    /* compute effective scale heigh */
	    double sham = exp(-palt / 4);
	    double sha = 1 - (taer55p / aerocon.taer55);

	    if (sha >= sham)
		taer55p = (double)(aerocon.taer55 * (1 - exp(-palt / 4)));
	    else {
		sha = -palt / log(sha);
		taer55p = (double)(aerocon.taer55 * (1 - exp(-palt / sha)));
	    }
	}
    }
}

void Altitude::parse()
{
    cin >> original_xps;
    cin.ignore(numeric_limits<int>::max(),'\n');	/* ignore comments */
    original_xps = -original_xps;
    
    cin >> original_xpp;
    cin.ignore(numeric_limits<int>::max(),'\n');	/* ignore comments */
    original_xpp = -original_xpp;
}

/* --- plane simulation output if selected ---- */
void Altitude::print()
{
    if(palt < 1000)
    {
	Output::Ln();
	Output::WriteLn(22," plane simulation description ");
	Output::WriteLn(22," ---------------------------- ");
		
	ostringstream s1;
	s1.setf(ios::fixed, ios::floatfield);
	s1.precision(2);
	s1 << " plane  pressure          [mb] " << setw(9) << pps << ends;
	Output::WriteLn(10,s1.str());

	ostringstream s2;
	s2.setf(ios::fixed, ios::floatfield);
	s2.precision(3);
	s2 << " plane  altitude absolute [km] " << setw(9) << plane_sim.zpl[33] << ends;
	Output::WriteLn(10,s2.str());

		
	Output::WriteLn(15," atmosphere under plane description: ");

	ostringstream s3;
	s3.setf(ios::fixed, ios::floatfield);
	s3.precision(3);
	s3 << " ozone content            " << setw(9) << puo3 << ends;
	Output::WriteLn(15,s3.str());


	ostringstream s4;
	s4.setf(ios::fixed, ios::floatfield);
	s4.precision(3);
	s4 << " h2o   content            " << setw(9) << puw << ends;
	Output::WriteLn(15,s4.str());

	ostringstream s5;
	s5.setf(ios::fixed, ios::floatfield);
	s5.precision(3);
	s5 << "aerosol opt. thick. 550nm " << setw(9) << taer55p << ends;
	Output::WriteLn(15,s5.str());
    }
}

Altitude Altitude::Parse()
{
    Altitude alt;
    alt.parse();
    return alt;
}
