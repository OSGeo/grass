#ifndef ABSTRA_H
#define ABSTRA_H

struct AbstraStruct
{
	double dtwava; /* downward absorption water vapor dtwava */
	double dtozon; /* downward absorption ozone       dtozon */
	double dtdica; /* downward absorption carbon diox dtdica */
	double dtoxyg; /* downward absorption oxygen      dtoxyg */
	double dtniox; /* downward absorption nitrous oxi dtniox */
	double dtmeth; /* downward absorption methane     dtmeth */
	double dtmoca; /* downward absorption carbon mono dtmoca */
	double utwava; /* upward absorption water vapor   utwava */
	double utozon; /* upward absorption ozone         utozon */
	double utdica; /* upward absorption carbon diox   utdica */
	double utoxyg; /* upward absorption oxygen        utoxyg */
	double utniox; /* upward   absorption nitrous oxi utniox */
	double utmeth; /* upward   absorption methane     utmeth */
	double utmoca; /* upward   absorption carbon mono utmoca */
	double ttwava; /* total(on the two paths ) absorption water vapor ttwava */
	double ttozon; /* total(on the two paths ) absorption ozone       ttozon */
	double ttdica; /* total(on the two paths ) absorption carbon diox ttdica */
	double ttoxyg; /* total(on the two paths ) absorption oxygen      ttoxyg */
	double ttniox; /* total    absorption nitrous oxi ttniox */
	double ttmeth; /* total    absorption methane     ttmeth */
	double ttmoca; /* total    absorption carbon mono ttmoca */
};

struct AtmosModel;
struct Altitude;

/*
To compute the gaseous transmittance between 0.25 and 4 mm for downward,
upward and total paths. We consider the six gases (O2, CO2, H 2O, O3, N 2O and CH4) separately.
The total transmission is put equal to the simple product of each ones. The spectral resolution is
equal to 10 cm-1.
iinf*/
void abstra (const AtmosModel& atms, const Altitude& alt,
			 const double wl, const double xmus, const double xmuv,
			 const double uw, const double uo3, double& uwus, double& uo3us,
			 const double uwpl, const double uo3pl, const double uwusp,
			 const double uo3usp, AbstraStruct& as );

#endif /* ABSTRA_H */
