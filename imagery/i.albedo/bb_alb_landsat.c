/* Broadband albedo Landsat 5TM and 7ETM+
 * (maybe others too but not sure)
 */
double bb_alb_landsat(double bluechan, double greenchan, double redchan,
		      double nirchan, double chan5, double chan7)
{
    double result;

    result =
	(0.293 * bluechan + 0.274 * greenchan + 0.233 * redchan +
	 0.156 * nirchan + 0.033 * chan5 + 0.011 * chan7);
    return result;
}
