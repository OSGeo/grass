#ifndef _GRASS_CC_H
#define _GRASS_CC_H

/* datum.c */
/* fully reworked by al 05/2000 */
int CC_get_datum_by_name(const char *);
char *CC_get_datum_by_nbr(int);
int CC_datum_shift(const char *, double *, double *, double *); 
int CC_get_datum_parameters(const char *, char *, double *, double *, double *);
char *CC_datum_name(int);
char *CC_datum_description(int);
char *CC_datum_ellipsoid(int);
/* molodensky.c */
/* new datum shift routines (block shift) by al 05/2000 */
int CC_datum_shift_CC(double, double, double, double, double, double *, double *, double *, double, double, double, double, double);
int CC_datum_to_datum_shift_CC(int, double, double, double, int, double *, double *, double *);
/* new molodensky datum transformation by al 05/2000 */
int CC_datum_shift_Molodensky(double, double, double, double, double, double, double *, double *, double *, double, double, double, double, double, double);
int CC_datum_to_datum_shift_M(int, double, double, double, int, double *, double *, double *);
/* new bursa wolf = 3d similarity transformation by al 05/2000 */
int CC_datum_shift_BursaWolf(double, double, double, double, double, double *, double *, double *, double, double, double, double, double, double, double, double, double);
int CC_datum_to_datum_shift_BW(int, double, double, double, int, double *, double *, double *);
/* format_ll.c */
int CC_lat_format(double, char *);
int CC_lon_format(double, char *);
int CC_lat_parts(double, int *, int *, double *, char *);
int CC_lon_parts(double, int *, int *, double *, char *);
/* geocen1.c */
int CC_ll2geo(double, double, double, double, double, double *, double *, double *);
int CC_lld2geo(double, double, double, double, double, double *, double *, double *);
/* geocen2.c */
int CC_geo2ll(double, double, double, double, double, double *, double *, double *, int, double);
int CC_geo2lld(double, double, double, double, double, double *, double *, double *);
/* scan_ll.c */
int CC_lat_scan(char *, double *);
int CC_lon_scan(char *, double *);
/* spheroid.c */
int CC_get_spheroid(const char *, double *, double *);
char *CC_spheroid_name(int);
/* new by al 05/2000 */
int CC_get_spheroid_by_name(const char *, double *, double *, double *);
char *CC_get_spheroid_by_nbr(int);
/* tm.c */
int CC_tm2ll_spheroid(char *);
int CC_tm2ll_spheroid_parameters(double, double);
int CC_tm2ll_zone(int);
int CC_tm2ll_north(double);
int CC_tm2ll(double, double *, double *);
int CC_ll2tm(double, double, double *, double *, int *);
/* utm.c */
int CC_u2ll_spheroid(char *);
int CC_u2ll_spheroid_parameters(double, double);
int CC_u2ll_zone(int);
int CC_u2ll_north(double);
int CC_u2ll(double, double *, double *);
int CC_ll2u(double, double, double *, double *, int *);

#endif
