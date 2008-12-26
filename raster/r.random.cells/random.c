/* random.c                                                             */
#include <grass/gis.h>
#include <grass/glocale.h>

#include "ransurf.h"

#define M1 259200
#define IA1 7141
#define IC1 54773
#define RM1 (1.0/M1)
#define M2 134456
#define IA2 8121
#define IC2 28411
#define RM2 (1.0/M2)
#define M3 243000
#define IA3 4561
#define IC3 51349

/* ran1() returns a double with a value between 0.0 and 1.0             */
double ran1(void)
{
    static long ix1, ix2, ix3;
    static double r[98];
    double temp;
    static int iff = 0;
    int j;

    G_debug(2, "ran1()");

    if (Seed < 0 || iff == 0) {
	iff = 1;
	ix1 = (IC1 - Seed) % M1;
	ix1 = (IA1 * ix1 + IC1) % M1;
	ix2 = ix1 % M2;
	ix1 = (IA1 * ix1 + IC1) % M1;
	ix3 = ix1 % M3;
	for (j = 1; j <= 97; j++) {
	    ix1 = (IA1 * ix1 + IC1) % M1;
	    ix2 = (IA2 * ix2 + IC2) % M2;
	    r[j] = (ix1 + ix2 * RM2) * RM1;
	}
	Seed = 1;
    }

    ix1 = (IA1 * ix1 + IC1) % M1;
    ix2 = (IA2 * ix2 + IC2) % M2;
    ix3 = (IA3 * ix3 + IC3) % M3;
    j = 1 + ((97 * ix3) / M3);
    if (j > 97 || j < 1)
	G_fatal_error(_("RAN1: j == %d shouldn't happen"), j);

    temp = r[j];
    r[j] = (ix1 + ix2 * RM2) * RM1;

    return temp;
}
