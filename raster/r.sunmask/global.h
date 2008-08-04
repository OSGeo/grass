#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL float asol, phi0, sun_zenith, sun_azimuth;	/* from nadir, from north */
GLOBAL int sunset;

/* proto */
long G_calc_solar_position(double, double, double, int, int, int, int, int,
			   int);
