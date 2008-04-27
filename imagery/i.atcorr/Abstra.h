#ifndef ABSTRA_H
#define ABSTRA_H

struct AbstraStruct
{
	float dtwava; /* downward absorption water vapor dtwava */
	float dtozon; /* downward absorption ozone       dtozon */
	float dtdica; /* downward absorption carbon diox dtdica */
	float dtoxyg; /* downward absorption oxygen      dtoxyg */
	float dtniox; /* downward absorption nitrous oxi dtniox */
	float dtmeth; /* downward absorption methane     dtmeth */
	float dtmoca; /* downward absorption carbon mono dtmoca */
	float utwava; /* upward absorption water vapor   utwava */
	float utozon; /* upward absorption ozone         utozon */
	float utdica; /* upward absorption carbon diox   utdica */
	float utoxyg; /* upward absorption oxygen        utoxyg */
	float utniox; /* upward   absorption nitrous oxi utniox */
	float utmeth; /* upward   absorption methane     utmeth */
	float utmoca; /* upward   absorption carbon mono utmoca */
	float ttwava; /* total(on the two paths ) absorption water vapor ttwava */
	float ttozon; /* total(on the two paths ) absorption ozone       ttozon */
	float ttdica; /* total(on the two paths ) absorption carbon diox ttdica */
	float ttoxyg; /* total(on the two paths ) absorption oxygen      ttoxyg */
	float ttniox; /* total    absorption nitrous oxi ttniox */
	float ttmeth; /* total    absorption methane     ttmeth */
	float ttmoca; /* total    absorption carbon mono ttmoca */
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
			 const float wl, const float xmus, const float xmuv,
			 const float uw, const float uo3, float& uwus, float& uo3us,
			 const float uwpl, const float uo3pl, const float uwusp,
			 const float uo3usp, AbstraStruct& as );

#endif /* ABSTRA_H */
