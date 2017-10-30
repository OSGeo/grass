/* main.c */


void where_is_point(double *length, struct SunGeometryVarDay *sunVarGeom,
		    struct GridGeometry *gridGeom);
void where_is_point_parallel(double *length, double *sunVarGeom_zp,
                             double *gridGeom_xx0,
                             double *gridGeom_yy0,
                             double *gridGeom_xg0,
                             double *gridGeom_yg0,
                             double *gridGeom_stepx,
                             double *gridGeom_stepy);
int searching(double *length, struct SunGeometryVarDay *sunVarGeom,
	      struct GridGeometry *gridGeom);
int searching_parallel(double *length,
                       double *sunVarGeom_z_orig,
                       double *sunVarGeom_zmax,
                       double *sunVarGeom_zp,
                       double *sunVarGeom_tanSolarAltitude,
                       double *sunVarGeom_stepsinangle,
                       double *sunVarGeom_stepcosangle,
                       double *gridGeom_xx0,
                       double *gridGeom_yy0,
                       double *gridGeom_xg0,
                       double *gridGeom_yg0,
                       double *gridGeom_stepx,
                       double *gridGeom_stepy,
                       double *gridGeom_deltx,
                       double *gridGeom_delty);


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
double brad_parallel(double sh, double *bh,
                     double *sunVarGeom_z_orig,
                     double *sunVarGeom_solarAltitude,
                     double *sunVarGeom_sinSolarAltitude,
                     double *sunSlopeGeom_slope,
                     double *sunSlopeGeom_aspect,
                     double *sunRadVar_cbh,
                     double *sunRadVar_linke,
                     double *sunRadVar_G_norm_extra);

double drad(double, double bh, double *rr,
	    struct SunGeometryVarDay *sunVarGeom,
	    struct SunGeometryVarSlope *sunSlopeGeom,
	    struct SolarRadVar *sunRadVar);
double drad_parallel(double sh, double bh, double *rr,
                     int *sunVarGeom_isShadow,
                     double *sunVarGeom_solarAltitude,
                     double *sunVarGeom_sinSolarAltitude,
                     double *sunVarGeom_solarAzimuth,
                     double *sunSlopeGeom_aspect,
                     double *sunSlopeGeom_slope,
                     double *sunRadVar_cdh,
                     double *sunRadVar_linke,
                     double *sunRadVar_G_norm_extra,
                     double *sunRadVar_alb);

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
void com_par_parallel(double *sunGeom_lum_C11,
                      double *sunGeom_lum_C13,
                      double *sunGeom_lum_C22,
                      double *sunGeom_lum_C31,
                      double *sunGeom_lum_C33,
                      double *sunGeom_sunrise_time,
                      double *sunGeom_sunset_time,
                      double *sunGeom_timeAngle,
                      double *sunVarGeom_solarAltitude,
                      double *sunVarGeom_sinSolarAltitude,
                      double *sunVarGeom_tanSolarAltitude,
                      double *sunVarGeom_solarAzimuth,
                      double *sunVarGeom_sunAzimuthAngle,
                      double *sunVarGeom_stepsinangle,
                      double *sunVarGeom_stepcosangle,
                      double *gridGeom_stepxy,
                      double latitude);

void com_par_const(double longitTime, struct SunGeometryConstDay *sungeom,
		   struct GridGeometry *gridGeom);
void com_par_const_parallel(double longitTime,
                            double *sunGeom_lum_C11,
                            double *sunGeom_lum_C13,
                            double *sunGeom_lum_C22,
                            double *sunGeom_lum_C31,
                            double *sunGeom_lum_C33,
                            double *sunGeom_sunrise_time,
                            double *sunGeom_sunset_time,
                            double *sunGeom_timeAngle,
                            double *sunGeom_sindecl,
                            double *sunGeom_cosdecl,
                            double *gridGeom_sinlat,
                            double *gridGeom_coslat);

double lumcline2(struct SunGeometryConstDay *sungeom,
		 struct SunGeometryVarDay *sunVarGeom,
		 struct SunGeometryVarSlope *sunSlopeGeom,
		 struct GridGeometry *gridGeom,
		 unsigned char *horizonpointer);
double lumcline2_parallel(double *sunGeom_timeAngle,
                          int    *sunVarGeom_isShadow,
                          double *sunVarGeom_solarAltitude,
                          double *sunVarGeom_sunAzimuthAngle,
                          double *sunVarGeom_z_orig,
                          double *sunVarGeom_zmax,
                          double *sunVarGeom_zp,
                          double *sunVarGeom_tanSolarAltitude,
                          double *sunVarGeom_stepsinangle,
                          double *sunVarGeom_stepcosangle,
                          double *sunSlopeGeom_longit_l,
                          double *sunSlopeGeom_lum_C31_l,
                          double *sunSlopeGeom_lum_C33_l,
                          double *gridGeom_xx0,
                          double *gridGeom_yy0,
                          double *gridGeom_xg0,
                          double *gridGeom_yg0,
                          double *gridGeom_stepx,
                          double *gridGeom_stepy,
                          double *gridGeom_deltx,
                          double *gridGeom_delty,
                          unsigned char *horizonpointer);


typedef double (*BeamRadFunc) (double sh, double *bh,
			       struct SunGeometryVarDay * sunVarGeom,
			       struct SunGeometryVarSlope * sunSlopeGeom,
			       struct SolarRadVar * sunRadVar);

typedef double (*DiffRadFunc) (double sh, double bh, double *rr,
			       struct SunGeometryVarDay * sunVarGeom,
			       struct SunGeometryVarSlope * sunSlopeGeom,
			       struct SolarRadVar * sunRadVar);
