/* Broadband albedo Landsat OLI 8
 * Simple weighted average from band 2 - 7
 * Temporary until a publication creates an algorithm
-* chan5 is OLI Band 6 (1.57-1.65) 
 * chan7 is OLI band 7 (2.11-2.29)
 */
double bb_alb_landsat8(double bluechan, double greenchan, double redchan,
		      double nirchan, double chan5, double chan7)
{
    double result;

    result =
	(0.06 * bluechan + 0.06 * greenchan + 0.03 * redchan +
	 0.03 * nirchan + 0.08 * chan5 + 0.18 * chan7)/0.44;
    return result;
}
