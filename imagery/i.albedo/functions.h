/* These are the headers for the RS functions */
/* 2004 */

/* BB_albedo functions */
double bb_alb_aster2bands(double greenchan, double nirchan);
double bb_alb_aster(double greenchan, double redchan, double nirchan,
		    double swirchan1, double swirchan2, double swirchan3,
		    double swirchan4, double swirchan5, double swirchan6);
double bb_alb_landsat(double bluechan, double greenchan, double redchan,
		      double nirchan, double chan5, double chan7);
double bb_alb_noaa(double redchan, double nirchan);

double bb_alb_modis(double redchan, double nirchan, double chan3,
		    double chan4, double chan5, double chan6, double chan7);
