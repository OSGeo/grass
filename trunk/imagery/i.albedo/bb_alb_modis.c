/* 
 * Broadband albedo MODIS
 */
double bb_alb_modis(double redchan, double nirchan, double chan3,
		    double chan4, double chan5, double chan6, double chan7)
{
    double result;

    result =
	(0.22831 * redchan + 0.15982 * nirchan +
	 0.09132 * (chan3 + chan4 + chan5) + 0.10959 * chan6 +
	 0.22831 * chan7);
    return result;
}
