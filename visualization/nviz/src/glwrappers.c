#include <stdio.h>
#include "tk.h"
#include "interface.h"
#include "coldefs.h"
#include <stdlib.h>

/* this file contains example code for interfacing GL to Tcl. The 
   functions here do app-specific initialization, inform the interpreter
   about app commands, and provide "wrapper" functions for calling
   GL from Tcl. */

int Nresize_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		int argc,	/* Number of arguments. */
		char **argv	/* Argument strings. */
    )
{
    int w, h;

    /* one render  */
    if (argc != 3)
	return (TCL_ERROR);
    w = atoi(argv[1]);
    h = atoi(argv[2]);
    gsd_viewport(0, w, 0, h);
    Nquick_draw_cmd(data, interp);
    return (TCL_OK);
}

/* ARGSUSED */
int Nfinish_cmd(ClientData dummy,	/* Not used. */
		Tcl_Interp * interp,	/* Current interpreter. */
		int argc,	/* Number of arguments. */
		char **argv	/* Argument strings. */
    )
{
    gsd_finish();
    return (TCL_OK);
}

int Nset_background_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			int argc,	/* Number of arguments. */
			char **argv	/* Argument strings. */
    )
{
    char buf[128];

    if (argc == 2)
	if (0 > (data->BGcolor = tcl_color_to_int(argv[1]))) {
	    sprintf(buf, "Unable to convert %s to color\n", argv[1]);
	    return (TCL_ERROR);
	}
    return (TCL_OK);

}

int tcl_color_to_int(const char *clr)
{
    int r, g, b;
    int c;

    if (3 != sscanf(clr, "#%02x%02x%02x", &r, &g, &b))
	return (-1);
    else
	RGB_TO_INT(r, g, b, c);
    return (c);

}
char *int_to_tcl_color(int clr)
{
    int r, g, b;
    static char c[128];

    INT_TO_RED(clr, r);
    INT_TO_GRN(clr, g);
    INT_TO_BLU(clr, b);
    sprintf(c, "#%02x%02x%02x", r, g, b);
    return (c);

}
