/* Broadband albedo Aster
 * After Liang, S.L., 2001. 
 * Narrowband to broadband conversion of land surface albedo 1 Algorithms.
 * Remote Sensing of Environment. 2001, 76, 213-238.
 * Input: Ref1, ref3, Ref5, Ref6, Ref8, Ref9
 */
double bb_alb_aster(double greenchan, double nirchan, double swirchan2,
		    double swirchan3, double swirchan5, double swirchan6)
{
    double result;

    result =
	(0.484 * greenchan + 0.335 * nirchan - 0.324 * swirchan2 +
	 0.551 * swirchan3 + 0.305 * swirchan5 - 0.367 * swirchan6 - 0.0015);
    return result;
}
