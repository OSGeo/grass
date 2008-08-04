/*
 * Library support for placing labels.
 */

#include <stdlib.h>
#include "interface.h"
#include <grass/gis.h>

extern int get_idnum();

int Nplace_label_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    int pt[2];
    int color;
    int size;
    char text[120];
    char font[100];

    if (argc != 7) {
	Tcl_SetResult(interp,
		      "Error: should be Nplace_label text font font_size color xpos ypos",
		      TCL_STATIC);
	return (TCL_ERROR);
    }

    sprintf(text, "%s", argv[1]);
    sprintf(font, "%s", argv[2]);

    size = (int)atoi(argv[3]);
    color = (int)tcl_color_to_int(argv[4]);
    pt[0] = (int)atoi(argv[5]);
    pt[1] = (int)atoi(argv[6]);

    /* Print the label */
    FontBase = load_font(font);

    if (FontBase) {
	gs_put_label(text, FontBase, size, color, pt);
    }
    else {
	Tcl_SetResult(interp, "Error: Unable to load font", TCL_STATIC);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}


/* Just a stub */
void G_site_destroy_struct(void *foo)
{
}
