#include <stdlib.h>
#include <grass/raster.h>
#include <stdio.h>
#include <grass/gis.h>

int main(void)
{
    char **pads;
    char **items;
    char **list;
    int npads;
    int nitems;
    int count;
    int p;
    int i;
    int n;
    int stat;

    if (R_open_driver() != 0)
	G_fatal_error("No graphics device selected");

    R_pad_list(&pads, &npads);
    for (p = -1; p < npads; p++) {
	if (p < 0) {
	    fprintf(stdout, "SCREEN STATUS:\n");
	    stat = R_pad_select("");
	}
	else {
	    fprintf(stdout, "FRAME: %s\n", pads[p]);
	    stat = R_pad_select(pads[p]);
	}

	if (stat) {
	    R_pad_perror("    ERROR", stat);
	    continue;
	}

	stat = R_pad_list_items(&items, &nitems);
	if (stat) {
	    R_pad_perror("    ERROR", stat);
	    continue;
	}

	for (i = 0; i < nitems; i++) {
	    fprintf(stdout, "    %8s:", items[i]);
	    stat = R_pad_get_item(items[i], &list, &count);
	    if (stat) {
		R_pad_perror("          ERROR", stat);
		continue;
	    }
	    for (n = 0; n < count; n++) {
		if (n == 0)
		    fprintf(stdout, "%s\n", list[n]);
		else
		    fprintf(stdout, "             %s\n", list[n]);
	    }
	    R_pad_freelist(list, count);
	}
    }

    R_close_driver();
    exit(0);
}
