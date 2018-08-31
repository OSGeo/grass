
#include <grass/gis.h>
#include <grass/raster.h>
#include "his.h"

/****************************************************************************
 * HIS_to_RGB() returns the R/G/B values for the proper HIS color associated
 *   with the following values:
 *    HUE:
 *       R:      red value 0 - 255
 *       G:      grn value 0 - 255
 *       B:      blu value 0 - 255
 *    INTENSITY:
 *       I    intensity value:  0 (black) to 255 (full color)
 *    SATURATION:
 *       S     saturation val:  0 (gray) to 255 (full color)
 *
 * make_gray_scale() generates a gray-scale color lookup table 
 ****************************************************************************/

void HIS_to_RGB(int R,		/* red percent. for hue: value 0 - 255 */
		int G,		/* grn percent. for hue: value 0 - 255 */
		int B,		/* blu percent. for hue: value 0 - 255 */
		int I,		/* intensity value: 0 (black) to 255 (white)     */
		int S,		/* saturation val:  0 (gray) to 255 (full color) */
		CELL * red,	/* resulting red value */
		CELL * grn,	/* resulting green value */
		CELL * blu	/* resulting blue value */
    )
{
    /* modify according to intensity */
    if (I != 255) {
	R = R * I / 255;
	G = G * I / 255;
	B = B * I / 255;
    }

    /* modify according to saturation (actually "haze factor") */
    if (S != 255) {
	R = 127 + (R - 127) * S / 255;
	G = 127 + (G - 127) * S / 255;
	B = 127 + (B - 127) * S / 255;
    }

    /* make sure final values are within range */
    if (R < 0)
	R = 0;
    if (G < 0)
	G = 0;
    if (B < 0)
	B = 0;

    if (R > 255)
	R = 255;
    if (G > 255)
	G = 255;
    if (B > 255)
	B = 255;

    *red = R;
    *grn = G;
    *blu = B;
}

int make_gray_scale(struct Colors *gray)
{
    int i;

    Rast_init_colors(gray);

    for (i = 0; i < 256; i++)
	Rast_set_c_color((CELL) i, i, i, i, gray);

    return 0;
}
