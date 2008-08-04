/* mod_elev.c */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vask.h>
#include "elev.h"

#define ZERO_DATA  0

int mod_elev_data(void)
{
    short ok;

    tl = G_location();
    sprintf(units, "METERS");

    ok = 0;
    while (!ok) {
	/* List options on the screen for the user to answer */
	ok = 1;
	V_clear();
	V_line(1, "Please check the elevation data convention:");
	V_line(3,
	       "ELEV_DATA  =  CELL FILE  [MAPSET  in  LOCATION] [MATH EXPERSION][UNITS]");

	V_line(6, "CELL FILE :       ");
	V_line(7, "MAPSET :          ");
	V_line(8, "LOCATION :        ");
	V_line(9, "MATH EXPRESSION : ");
	V_line(10, "UNITS :           ");
	V_line(12, "NO DATA VALUES  : ");

	V_const(elev_layer, 's', 6, 20, 40);
	V_const(mapset_elev, 's', 7, 20, 40);
	V_const(tl, 's', 8, 20, 40);
	V_ques(math_exp, 's', 9, 20, 40);
	V_const(units, 's', 10, 20, 10);
	V_ques(nd, 's', 12, 20, 10);

	V_intrpt_ok();
	if (!V_call())
	    exit(1);

    }

    return 0;
}
