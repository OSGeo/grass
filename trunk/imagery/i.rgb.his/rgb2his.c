
/******************************************************************************
NAME:                         RGB2HIS
 
PURPOSE    To process red,green,blue bands to hue,intensity,saturation.
 
ALGORITHM:
   Get red, green, blue from input buffer
   Create the HIS bands
   Write to output buffer
 
ASSUMPTION:
   The input images are read to the input buffer.
 
******************************************************************************/
/* For GRASS one row from each cell map is passed in and each cell in
   each band is processed and written out.   CWU GIS Lab: DBS 8/90 */

#include <grass/gis.h>
#include "globals.h"

void rgb2his(CELL * rowbuffer[3], int columns)
{
    int sample;			/* sample indicator                      */
    double red;			/* the red band output                   */
    double green;		/* the green band output                 */
    double blue;		/* the blue band output                  */
    double scaler;		/* red value                             */
    double scaleg;		/* green value                           */
    double scaleb;		/* blue value                            */
    double high;		/* maximum red, green, blue              */
    double low;			/* minimum red, green, blue              */
    double intens;		/* intensity                             */
    double sat;			/* saturation                            */
    double hue = 0.0L;		/* hue                                   */

    for (sample = 0; sample < columns; sample++) {
	if (Rast_is_c_null_value(&rowbuffer[0][sample]) ||
	    Rast_is_c_null_value(&rowbuffer[1][sample]) ||
	    Rast_is_c_null_value(&rowbuffer[2][sample])) {
	    Rast_set_c_null_value(&rowbuffer[0][sample], 1);
	    Rast_set_c_null_value(&rowbuffer[1][sample], 1);
	    Rast_set_c_null_value(&rowbuffer[2][sample], 1);
	    continue;
	}

	scaler = (double)rowbuffer[0][sample];
	scaler /= 255.0;
	scaleg = (double)rowbuffer[1][sample];
	scaleg /= 255.0;
	scaleb = (double)rowbuffer[2][sample];
	scaleb /= 255.0;

	high = scaler;
	if (scaleg > high)
	    high = scaleg;
	if (scaleb > high)
	    high = scaleb;
	low = scaler;
	if (scaleg < low)
	    low = scaleg;
	if (scaleb < low)
	    low = scaleb;
	/*
	   calculate the lightness (intensity)
	 */
	intens = ((high + low) / 2.0);
	/*
	   if min = max the achromatic case R=G=B
	 */
	if (high == low) {
	    sat = 0.0;
	    /*        hue = -1.0; */
	    hue = 0.0;
	    rowbuffer[0][sample] = (unsigned char)hue;
	    rowbuffer[1][sample] = (unsigned char)(intens * 255.);
	    rowbuffer[2][sample] = (unsigned char)(sat * 255.);
	}
	/*
	   else chromatic case
	 */
	else if (high != low) {
	    if (intens <= 0.5)
		sat = (high - low) / (high + low);
	    else
		/*
		   sat = (high-low)/(2 - (high-low));
		 */
		sat = (high - low) / (2 - high - low);
	    red = (high - scaler) / (high - low);
	    green = (high - scaleg) / (high - low);
	    blue = (high - scaleb) / (high - low);
	    /*
	       resulting color between yellow and magenta
	     */
	    if (scaler == high)
		hue = blue - green;
	    /*
	       resulting color between cyan and yellow
	     */
	    else if (scaleg == high)
		hue = 2 + red - blue;
	    /*
	       resulting color between magenta and cyan
	     */
	    else if (scaleb == high)
		hue = 4 + green - red;
	    /*
	       convert to degrees
	     */
	    hue *= 60.0;
	    /*
	       make nonnegative
	     */
	    if (hue < 0.0)
		hue += 360.0;
	    /*
	       set the HIS output values
	     */
	    rowbuffer[0][sample] = (unsigned char)(255.0 * hue / 360.0 + 0.5);
	    rowbuffer[1][sample] = (unsigned char)(intens * 255. + 0.5);
	    rowbuffer[2][sample] = (unsigned char)(sat * 255. + 0.5);
	}
    }
}
