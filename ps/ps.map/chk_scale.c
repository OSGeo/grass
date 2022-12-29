#include <stdio.h>
#include <string.h>
#include "local_proto.h"

/****************************************************************
 *
 * check_scale (text)
 *
 *   text of scale request to be checked
 ***************************************************************/

int check_scale(char *text)
{
    char unit1[30];
    char unit2[30];
    char equals[30];
    char dummy[2];
    long n1, n2;
    double u1;

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
	if (strncmp(unit1, "panel", 5) == 0 && u1 > 0)
	    return 1;
	if (strncmp(unit1, "inch", 4) == 0 && u1 > 0)
	    return 1;
    }

    /*
     * unitless ratio specification
     *    n : m
     */
    *dummy = 0;
    n1 = n2 = 0;
    if (sscanf(text, "%ld : %ld%1s", &n1, &n2, dummy) == 2) {
	if (n1 <= 0 || n2 <= 0 || *dummy)
	    return 0;
	return 1;
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
	    return 0;
	if (strcmp(equals, "=") != 0 && strncmp(equals, "equal", 5) != 0)
	    return 0;

	/* unit1: inches */
	if (strncmp(unit1, "inch", 4) == 0)
	    u1 = 1;
	else
	    return 0;

	/* unit2: meters, miles, kilometers */
	if (strncmp(unit2, "mile", 4) == 0)
	    return 1;
	if (strncmp(unit2, "meter", 5) == 0)
	    return 1;
	if (strncmp(unit2, "kilometer", 9) == 0)
	    return 1;

    }
    return 0;
}
