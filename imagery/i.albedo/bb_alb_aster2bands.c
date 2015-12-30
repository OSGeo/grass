/* Broadband albedo Aster
 * Salleh and Chan, 2014. 
 * Land Surface Albedo Determination: 
 * Remote Sensing and Statistical Validation. 
 * in proceedings of FIG 2014.
 * https://www.fig.net/resources/proceedings/fig_proceedings/fig2014/papers/ts05g/TS05G_salleh_chan_6910.pdf
 * Input: Ref1, Ref3
 */
double bb_alb_aster2bands(double greenchan, double nirchan)
{
    double result =
	(0.697 * greenchan + 0.298 * nirchan - 0.008);
    return result;
}
