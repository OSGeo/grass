/* Function: scale
 **
 ** This function returns the horizontal extent of the map in inches and
 ** modifies the text string.  Much of this code is from p.map's scale.c.
 **
 ** Author: Paul W. Carlson     April 1992
 */
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "distance.h"

#define PWIDTH	(PS.page_width-PS.left_marg-PS.right_marg)
static double do_scale(char *);

#ifdef __GNUC_MINOR__
static int OOPS(void) __attribute__ ((__noreturn__));
#else
static int OOPS();
#endif

double scale(char *text)
{
    double inches;

    inches = do_scale(text);
    return inches;
}

static double do_scale(char *text)
{
    char unit1[30];
    char unit2[30];
    char equals[30];
    char dummy[2];
    long n1, n2;
    double u1, u2;

    /*
     * absolute horizontal width specification
     *   x inches
     *   x panels
     * convert text to 1 : n
     */
    u1 = 0;
    *unit1 = 0;
    *dummy = 0;
    if (sscanf(text, "%lf %s %1s", &u1, unit1, dummy) == 2 && *dummy == 0) {
	u2 = -1;
	if (strncmp(unit1, "panel", 5) == 0 && u1 > 0)
	    u2 = u1 * (PWIDTH);
	else if (strncmp(unit1, "inch", 4) == 0 && u1 > 0)
	    u2 = u1;
	if (u2 > 0) {
	    sprintf(text, "1 : %.0f",
		    METERS_TO_INCHES * distance(PS.w.east, PS.w.west) / u2);
	    return u2;
	}
    }

    /*
     * unitless ratio specification
     *    n : m
     */
    *dummy = 0;
    n1 = n2 = 0;
    if (sscanf(text, "%ld : %ld%1s", &n1, &n2, dummy) == 2) {
	if (n1 <= 0 || n2 <= 0 || *dummy)
	    OOPS();
	sprintf(text, "%ld : %ld", n1, n2);
	return METERS_TO_INCHES * distance(PS.w.east, PS.w.west) * n1 / n2;
    }

    /*
     *
     * ratio specification with unit conversions
     *    x inches equals y miles
     *    x inches equals y meters
     *    x inches equals y kilometers
     */

    *unit1 = 0;
    *unit2 = 0;
    *equals = 0;
    n1 = n2 = 0;
    if (sscanf(text, "%ld %s %s %ld %s", &n1, unit1, equals, &n2, unit2) == 5) {
	if (n1 <= 0 || n2 <= 0)
	    OOPS();
	if (strcmp(equals, "=") != 0 && strncmp(equals, "equal", 5) != 0)
	    OOPS();

	/* unit1: inches */
	if (strncmp(unit1, "inch", 4) == 0)
	    u1 = n1;
	else
	    OOPS();

	/* unit2: meters, miles, kilometers */
	if (strncmp(unit2, "mile", 4) == 0) {
	    u2 = MILES_TO_INCHES;
	    strcpy(unit2, "mile");
	}
	else if (strncmp(unit2, "meter", 5) == 0) {
	    u2 = METERS_TO_INCHES;
	    strcpy(unit2, "meter");
	}
	else if (strncmp(unit2, "kilometer", 9) == 0) {
	    u2 = METERS_TO_INCHES * 1000;
	    strcpy(unit2, "kilometer");
	}
	else
	    OOPS();
	u2 *= n2;

	strcpy(unit1, "inch");
	strcpy(equals, "equal");
	if (n1 == 1)
	    strcat(equals, "s");
	else
	    strcat(unit1, "es");

	if (n2 != 1)
	    strcat(unit2, "s");

	sprintf(text, "%ld %s %s %ld %s", n1, unit1, equals, n2, unit2);

	return METERS_TO_INCHES * distance(PS.w.east, PS.w.west) * u1 / u2;
    }
    OOPS();
}

static int OOPS(void)
{
    G_fatal_error(_("PSmap: do_scale(): shouldn't happen"));
}
