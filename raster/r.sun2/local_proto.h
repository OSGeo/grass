/* main.c */


void where_is_point(double *length, struct SunGeometryVarDay *sunVarGeom,
		    struct GridGeometry *gridGeom);
int searching(double *length, struct SunGeometryVarDay *sunVarGeom,
	      struct GridGeometry *gridGeom);

int useCivilTime();
void setUseCivilTime(int val);
int useShadow();
void setUseShadow(int val);
int useShadowData();
void setUseShadowData(int val);
int useHorizonData();
void setUseHorizonData(int val);
double getTimeOffset();
void setTimeOffset(double val);
double getHorizonInterval();
void setHorizonInterval(double val);
void setAngularLossDenominator();


void cube(int, int);

double com_sol_const(int no_of_day);


double brad(double, double *bh, struct SunGeometryVarDay *sunVarGeom,
	    struct SunGeometryVarSlope *sunSlopeGeom,
	    struct SolarRadVar *sunRadVar);
double drad(double, double bh, double *rr,
	    struct SunGeometryVarDay *sunVarGeom,
	    struct SunGeometryVarSlope *sunSlopeGeom,
	    struct SolarRadVar *sunRadVar);


double brad_angle_loss(double, double *bh,
		       struct SunGeometryVarDay *sunVarGeom,
		       struct SunGeometryVarSlope *sunSlopeGeom,
		       struct SolarRadVar *sunRadVar);
double drad_angle_loss(double, double bh, double *rr,
		       struct SunGeometryVarDay *sunVarGeom,
		       struct SunGeometryVarSlope *sunSlopeGeom,
		       struct SolarRadVar *sunRadVar);


void com_par(struct SunGeometryConstDay *sungeom,
	     struct SunGeometryVarDay *sunVarGeom,
	     struct GridGeometry *gridGeom,
	     double latitude, double longitude);
void com_par_const(double longitTime, struct SunGeometryConstDay *sungeom,
		   struct GridGeometry *gridGeom);
double lumcline2(struct SunGeometryConstDay *sungeom,
		 struct SunGeometryVarDay *sunVarGeom,
		 struct SunGeometryVarSlope *sunSlopeGeom,
		 struct GridGeometry *gridGeom,
		 unsigned char *horizonpointer);


typedef double (*BeamRadFunc) (double sh, double *bh,
			       struct SunGeometryVarDay * sunVarGeom,
			       struct SunGeometryVarSlope * sunSlopeGeom,
			       struct SolarRadVar * sunRadVar);

typedef double (*DiffRadFunc) (double sh, double bh, double *rr,
			       struct SunGeometryVarDay * sunVarGeom,
			       struct SunGeometryVarSlope * sunSlopeGeom,
			       struct SolarRadVar * sunRadVar);
