#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* List the maps currently displayed in GRASS monitor MN + Huidae Cho 8/2001 */

int main(void)
{
    char **pads;
    int npads;
    int p;
    int i;
    int stat;
    char **rast, **vect, **list;
    int nrasts, nvects, nlists;


    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    R_pad_list(&pads, &npads);

    stat = R_pad_select("");
    if (stat)
	G_fatal_error("Failed to process the screen pad");

    for (p = npads - 1; p >= 0; p--) {
	rast = vect = list = NULL;
	nrasts = nvects = nlists = 0;

	R_pad_select(pads[p]);
	fprintf(stdout, "frame: %s\n", pads[p]);

	if (D_get_cell_list(&rast, &nrasts) < 0)
	    rast = NULL;

	if (D_get_dig_list(&vect, &nvects) < 0)
	    vect = NULL;

	if (D_get_list(&list, &nlists) < 0)
	    list = NULL;

	/*print a comma separated list: */
	fprintf(stdout, "rast: ");
	for (i = 0; i < nrasts; i++) {
	    if (i > 0)
		fprintf(stdout, ",");
	    fprintf(stdout, "%s", rast[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "vect: ");
	for (i = 0; i < nvects; i++) {
	    if (i > 0)
		fprintf(stdout, ",");
	    fprintf(stdout, "%s", vect[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "commands:\n");
	for (i = 0; i < nlists; i++)
	    fprintf(stdout, "%s\n", list[i]);

	if (rast)
	    R_pad_freelist(rast, nrasts);

	if (vect)
	    R_pad_freelist(vect, nvects);

	if (list)
	    R_pad_freelist(list, nlists);

	fprintf(stdout, "\n");
    }

    if (pads)
	R_pad_freelist(pads, npads);

    R_close_driver();

    exit(0);
}
