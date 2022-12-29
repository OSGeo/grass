
/******************************************************************************
NAME:                         HIS2RGB
 
PURPOSE    To process hue,intensity,saturation bands to red,green,blue.
 
ALGORITHM:
   Get hue, intensity, saturation from input buffer
   Create the RGB bands
   Write to output buffer
 
ASSUMPTION:
   The input images are read to the input buffer.
 
******************************************************************************/
/* For GRASS one row from each cell map is passed in and each cell in
   each band is processed and written out.   CWU GIS Lab: DBS 8/90 */

#include <grass/gis.h>
#include "globals.h"

void his2rgb(CELL *rowbuffer[3], int columns)
{
    long sample;		/* sample indicator                          */
    double red;			/* the red band output                       */
    double red255;		/* the red band output                       */
    double green;		/* the green band output                     */
    double green255;		/* the green band output                     */
    double blue;		/* the blue band output                      */
    double blue255;		/* the blue band output                      */
    double m1;			/* value used for determining RGB            */
    double m2;			/* value used for determining RGB            */
    double scalei;		/* intensity value                           */
    double scales;		/* saturation value                          */
    double hue;			/* hue                                       */
    double savehue;		/* save the hue for future processing        */


    for (sample = 0; sample < columns; sample++) {
	if (Rast_is_c_null_value(&rowbuffer[0][sample]) ||
	    Rast_is_c_null_value(&rowbuffer[1][sample]) ||
	    Rast_is_c_null_value(&rowbuffer[2][sample])) {
	    Rast_set_c_null_value(&rowbuffer[0][sample], 1);
	    Rast_set_c_null_value(&rowbuffer[1][sample], 1);
	    Rast_set_c_null_value(&rowbuffer[2][sample], 1);
	    continue;
	}

	red = green = blue = 0.0;
	scalei = (double)rowbuffer[1][sample];
	scalei /= 255.;
	scales = (double)rowbuffer[2][sample];
	scales /= 255.;
	m2 = 0.0;
	if (scalei <= 0.50)
	    m2 = scalei * (1.0 + scales);
	else if (scalei > 0.50)
	    m2 = scalei + scales - (scalei * scales);
	m1 = 2.0 * scalei - m2;

	hue = (double)360.0 *rowbuffer[0][sample] / 255.0;

	if (scales == 0.0) {
	    if (hue == -1.0) {
		red = scalei;
		green = scalei;
		blue = scalei;
	    }
	}
	else {
	    /* calculate the red band */
	    savehue = hue + 120.0;
	    if (savehue > 360.0)
		savehue -= 360.0;
	    if (savehue < 0.0)
		savehue += 360.0;
	    if (savehue < 60.0)
		red = m1 + (m2 - m1) * savehue / 60.0;
	    else if (savehue < 180.0)
		red = m2;
	    else if (savehue < 240.0)
		red = m1 + (m2 - m1) * (240.0 - savehue) / 60.0;
	    else
		red = m1;

	    /* calculate the green band */
	    savehue = hue;
	    if (savehue > 360.0)
		savehue -= 360.0;
	    if (savehue < 0.0)
		savehue += 360.0;
	    if (savehue < 60.0)
		green = m1 + (m2 - m1) * savehue / 60.0;
	    else if (savehue < 180.0)
		green = m2;
	    else if (savehue < 240.0)
		green = m1 + (m2 - m1) * (240.0 - savehue) / 60.0;
	    else
		green = m1;

	    /* calculate the blue band */
	    savehue = hue - 120.0;
	    if (savehue > 360.0)
		savehue -= 360.0;
	    if (savehue < 0.0)
		savehue += 360.0;
	    if (savehue < 60.0)
		blue = m1 + (m2 - m1) * savehue / 60.0;
	    else if (savehue < 180.0)
		blue = m2;
	    else if (savehue < 240.0)
		blue = m1 + (m2 - m1) * (240.0 - savehue) / 60.0;
	    else
		blue = m1;
	}

	red255 = red * 255.0;
	green255 = green * 255.0;
	blue255 = blue * 255.0;
	if (red255 > 255.0)
	    red = 255.0;
	else
	    red = red255;
	if (green255 > 255.0)
	    green = 255.0;
	else
	    green = green255;
	if (blue255 > 255.0)
	    blue = 255.0;
	else
	    blue = blue255;

	if (red > 255. - 0.5)
	    red = 255. - 0.5;
	if (red < 0.0)
	    red = 0.0;
	if (green > 255. - 0.5)
	    green = 255. - 0.5;
	if (green < 0.0)
	    green = 0.0;
	if (blue > 255. - 0.5)
	    blue = 255. - 0.5;
	if (blue < 0.0)
	    blue = 0.0;

	rowbuffer[0][sample] = (unsigned char)(red + 0.5);
	rowbuffer[1][sample] = (unsigned char)(green + 0.5);
	rowbuffer[2][sample] = (unsigned char)(blue + 0.5);
    }
}
