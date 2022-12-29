extern float asol, phi0, sun_zenith, sun_azimuth;	/* from nadir, from north */
extern int sunset;

/* proto */
long calc_solar_position(double, double, double, int, int, int, int, int,
			 int);
