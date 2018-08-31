/* Function: text_box_path
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include "ps_info.h"

#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

/* font name, size, and color must be set first, outside text_box_path()
 * because text_box_path() is called repeatedly with identical 
 * font name, size, and color */
int
text_box_path(double x, double y, int xref, int yref, char *text,
	      float rotate)
{
    /* get relative box coordinates */
    fprintf(PS.fp, "ZB (%s) PB\n", text);

    /* set box x coordinate */
    fprintf(PS.fp, "%.2f ", x);

    /* set box y coordinate */
    fprintf(PS.fp, " %.2f ", y);

    fprintf(PS.fp, "translate %.2f rotate ", rotate);

    fprintf(PS.fp, " 0 ");

    switch (xref) {
    case LEFT:
	fprintf(PS.fp, "LTX");
	break;
    case RIGHT:
	fprintf(PS.fp, "RTX");
	break;
    case CENTER:
    default:
	fprintf(PS.fp, "CTX");
	break;
    }

    fprintf(PS.fp, " 0 ");

    switch (yref) {
    case UPPER:
	fprintf(PS.fp, "UTY");
	break;

    case LOWER:
	fprintf(PS.fp, "LTY");
	break;

    case CENTER:
    default:
	fprintf(PS.fp, "CTY");
	break;
    }
    fprintf(PS.fp, " TR TB\n");

    return 0;
}
