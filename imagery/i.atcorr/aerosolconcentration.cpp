#include "aerosolconcentration.h"
#include "atmosmodel.h"
#include "common.h"

/**********************************************************************c
c              aerosol model (concentration)                           c
c              ----------------------------                            c
c                                                                      c
c                                                                      c
c  v             if you have an estimate of the meteorological         c
c                parameter: the visibility v, enter directly the       c
c                value of v in km (the aerosol optical depth will      c
c                be computed from a standard aerosol profile)          c

c                                                                      c
c  v=0, taer55   if you have an estimate of aerosol optical depth ,    c
c                enter v=0 for the visibility and enter the aerosol    c
c                optical depth at 550                                  c
c                                                                      c
c  v=-1          warning:  if iaer=0, enter v=-1                       c
c                                                                      c
c**********************************************************************/
void AerosolConcentration::parse(const long int _iaer, const AtmosModel& atms)
{
    iaer = _iaer;

    taer55 = 0.f;
    cin >> v;
    cin.ignore(numeric_limits<int>::max(),'\n');	/* ignore comments */

    if(v == 0)
    {
	cin >> taer55;
	cin.ignore(numeric_limits<int>::max(),'\n');	/* ignore comments */
	v = (double)(exp(-log(taer55/2.7628f)/0.79902f));
    }
    else if(v > 0) oda550(v, atms);
}

void AerosolConcentration::oda550(const double vis, const AtmosModel& atms)
{
    /* aerosol optical depth at wl=550nm */
    /* vertical repartition of aerosol density for v=23km */
    /*                ( in nbr of part/cm3 ) */
    static const double an23[34] = {
	2.828e+03,1.244e+03,5.371e+02,2.256e+02,1.192e+02,
	8.987e+01,6.337e+01,5.890e+01,6.069e+01,5.818e+01,
	5.675e+01,5.317e+01,5.585e+01,5.156e+01,5.048e+01,
	4.744e+01,4.511e+01,4.458e+01,4.314e+01,3.634e+01,
	2.667e+01,1.933e+01,1.455e+01,1.113e+01,8.826e+00,
	7.429e+00,2.238e+00,5.890e-01,1.550e-01,4.082e-02,
	1.078e-02,5.550e-05,1.969e-08,0.000e+00
    };


    /* vertical repartition of aerosol density for v=5km */
    /*                ( in nbr of part/cm3 ) */
    static const double an5[34] = {
	1.378e+04,5.030e+03,1.844e+03,6.731e+02,2.453e+02,
	8.987e+01,6.337e+01,5.890e+01,6.069e+01,5.818e+01,
	5.675e+01,5.317e+01,5.585e+01,5.156e+01,5.048e+01,
	4.744e+01,4.511e+01,4.458e+01,4.314e+01,3.634e+01,
	2.667e+01,1.933e+01,1.455e+01,1.113e+01,8.826e+00,
	7.429e+00,2.238e+00,5.890e-01,1.550e-01,4.082e-02,
	1.078e-02,5.550e-05,1.969e-08,0.000e+00
    };

    taer55 = 0;
    if(fabs(vis) <= 0) return;
    if(iaer == 0) return;

    for(int k = 0; k < 32; k++)
    {
	double dz = atms.z[k+1] - atms.z[k];
	double az = (115.f / 18.f) * (an5[k] - an23[k]);
	double az1 = (115.f / 18.f) * (an5[k+1] - an23[k+1]);

	double bz = (5.f * an5[k] / 18.f) - (23.f * an23[k] / 18.f);
	double bz1 = (5.f * an5[k+1] / 18.f) - (23.f * an23[k+1] / 18.f);

	double bnz = az / vis - bz;
	double bnz1 = az1 / vis - bz1;

	double ev = (double)(dz * exp((log(bnz) + log(bnz1)) / 2));
	taer55 += ev * sigma * 1.0e-03f;
    }
}

void AerosolConcentration::print()
{
    /* --- aerosol model (concentration) ---- */
    Output::Begin();
    Output::End();
    if(iaer == 0) return;

    Output::Begin();
    Output::Repeat(10, ' ');
    Output::Print(" optical condition identity :");
    Output::End();
    if(fabs(v) <= xacc)
    {
	Output::Begin();
	Output::Repeat(15, ' ');
	Output::Print(" user def. opt. thick. at 550nm :");
	ostringstream s;
	s.setf(ios::fixed, ios::floatfield);
	s << setprecision(4);
	s << setw(11) << taer55 << ends;
	Output::Print(s.str());
	Output::End();
    }
    else
    {
	Output::Begin();
	Output::Repeat(15, ' ');
	Output::Print(" visibility :");
	ostringstream s1;
	s1.setf(ios::fixed, ios::floatfield);
	s1 << setprecision(2);
	s1 << setw(8) << v << ends;
	Output::Print(s1.str());
	Output::Print(" km  opt. thick. 550nm :");
	ostringstream s2;
	s2.setf(ios::fixed, ios::floatfield);
	s2 << setprecision(4);
	s2 << setw(9) << taer55 << ends;
	Output::Print(s2.str());
	Output::End();
    }

    Output::Begin();
    Output::End();    
}


AerosolConcentration AerosolConcentration::Parse(const long int iaer, const AtmosModel& atms)
{
    AerosolConcentration aerocon;
    aerocon.parse(iaer, atms);
    return aerocon;
}

