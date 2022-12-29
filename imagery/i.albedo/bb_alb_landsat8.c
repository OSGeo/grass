/* Broadband albedo Landsat OLI 8
 * Simple weighted average from band 2 - 7
 * Temporary until a publication creates an algorithm
-* chan5 is OLI Band 6 (1.57-1.65) 
 * chan7 is OLI band 7 (2.11-2.29)
 * 
 * Temporary better fix than weighted average
 * ------------------------------------------
 * r.regression.multi
 * mapx=LC81270512014115LGN00.toar.1,LC81270512014115LGN00.toar.2,
 * LC81270512014115LGN00.toar.3,LC81270512014115LGN00.toar.4,
 * LC81270512014115LGN00.toar.5,LC81270512014115LGN00.toar.6,
 * LC81270512014115LGN00.toar.7
 * mapy=MCD43_2014113 
 * 
 */
double bb_alb_landsat8(double shortbluechan, double bluechan, double greenchan, double redchan,
              double nirchan, double chan5, double chan7)
{
    double result;

    result = 0.058674+shortbluechan*2.153642+bluechan*(-2.242688)+
    greenchan*(-0.520669)+redchan*0.622670+nirchan*0.129979+
    chan5*(-0.047970)+chan7*0.152228;
    return result;
}
