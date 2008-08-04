/* #include "gis.h" */
#include <stdlib.h>
#include <string.h>
#include <grass/vask.h>
#include "global.h"

#define ZERO_DATA  0

int ask_elev_data(void)
{
    char buf1[60];
    char buf2[60];
    char units[10];
    char *tl;			/* Target Location */
    char *elev_data;		/* Tempoary elevation data layer */
    short ok;
    int no_data_value = ZERO_DATA;

    tl = (char *)G_calloc(40, sizeof(char));
    elev_data = (char *)G_calloc(40, sizeof(char));
    tl = G_location();

    sprintf(elev_data, "ELEV_DATA");
    sprintf(units, "METERS");
    *buf1 = '\0';

    ok = 0;

  /** Ask user if modification of elevation data is needed **/
    ok = G_yes("\nModify the data used for elevation ? ", ok);
    if (!ok) {
	select_target_env();
	if ((mapset_elev = G_find_cell(elev_layer, "")) == NULL)
	    exit(0);
	select_current_env();
	return (0);
    }

    ok = 0;
    while (!ok) {
	/* List options on the screen for the user to answer */
	ok = 1;
	V_clear();
	V_line(1, "Please check the G_mapcalc convention:");
	V_line(3,
	       "ELEV_DATA  =  CELL FILE  [MAPSET  in  LOCATION] [MATH EXPERSION][UNITS]");

	V_line(5, "ELEV_DATA :       ");
	V_line(6, "CELL FILE :       ");
	V_line(7, "MAPSET :          ");
	V_line(8, "LOCATION :        ");
	V_line(9, "MATH EXPRESSION : ");
	V_line(10, "UNITS :           ");
	V_line(12, "NO DATA VALUES = :");


	/* V_ques ( variable, type, row, col, length) ; */
	V_ques(elev_data, 's', 5, 20, 40);
	V_const(elev_layer, 's', 6, 20, 40);
	V_const(mapset_elev, 's', 7, 20, 40);
	V_const(tl, 's', 8, 20, 40);
	V_ques(buf1, 's', 9, 20, 40);
	V_const(units, 's', 10, 20, 10);
	V_ques(&no_data_value, 'i', 12, 20, 10);

	V_intrpt_ok();
	if (!V_call())
	    exit(1);

	ok = 1;
	sprintf(buf2, "Gmapcalc %s = 'if(%s, %s %s , %d)'", elev_data,
		elev_layer, elev_layer, buf1, no_data_value);

	fprintf(stderr,
		"\n\n The following G_mapcalc syntax will be used \n for the modified elevation data\n\n");
	fprintf(stderr, "%s = 'if(%s, %s %s , %d)'", elev_data, elev_layer,
		elev_layer, buf1, no_data_value);
	ok = G_yes("\nDo you accept this G_mapcalc convention \n", ok);
	if (ok) {

		 /** Set LOCATION to target location **/
	    G_setenv("LOCATION_NAME", tl);

		 /** system GMAPCALC **/
	    G_system(buf2);
	    /* elev_data becomes the new elevation layer */
	    strcpy(elev_layer, elev_data);
	    /* need mapset if changed */
	    if ((mapset_elev = G_find_cell(elev_layer, "")) == NULL)
		exit(0);
	    select_current_env();

		 /** reset LOCATION to current location **/
	    G_setenv("LOCATION_NAME", G_location());

	}
    }
    return (0);
}
