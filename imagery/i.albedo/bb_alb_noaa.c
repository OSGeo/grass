/* 
 * Broadband albedo NOAA AVHRR 14 
 * (maybe others too but not sure)
 */
double bb_alb_noaa(double redchan, double nirchan)
{
    double result;

    result = (0.035 + 0.545 * nirchan - 0.32 * redchan);
    return result;
}
