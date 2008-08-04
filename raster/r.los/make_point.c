
/****************************************************************/
/*                                                              */
/*      make_point.c    in      ~/src/Glos                      */
/*                                                              */
/*      This function allocates memory space for a new point,   */
/*      initializes the various fields using the values of      */
/*      the parameters passed and returns the address of this   */
/*      new point so that it could be attached in the linked    */
/*      list.                                                   */
/*                                                              */

/****************************************************************/

#include <grass/gis.h>
#include "point.h"

/* #define NULL 0  should be (char *0), or just let the compiler fix it. */

#define		NEW_PT_X		NEW_PT->x
#define		NEW_PT_Y		NEW_PT->y
#define		NEW_PT_ORIENTATION	NEW_PT->orientation
#define		NEW_PT_INCLINATION	NEW_PT->inclination
#define		NEXT_NEW_PT		NEW_PT->next

struct point *make_point(double orientation, double inclination, int y, int x)
{
    struct point *NEW_PT;

    NEW_PT = (struct point *)G_malloc(sizeof(struct point));
    NEW_PT_ORIENTATION = orientation;
    NEW_PT_INCLINATION = inclination;
    NEW_PT_Y = y;
    NEW_PT_X = x;
    NEXT_NEW_PT = NULL;

    return (NEW_PT);
}

/********* END OF FUNCTION "MAKE_POINT" *************************/
