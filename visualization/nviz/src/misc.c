#include "interface.h"

int Nlibinit_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		 int argc,	/* Number of arguments. */
		 char **argv	/* Argument strings. */
    )
{
    GS_libinit();

    return 0;
}

int Nget_cancel_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    char buf[128];

    sprintf(buf, "%d", GS_check_cancel());
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return (TCL_OK);

}

int Nset_SDsurf_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    int id;

    /* Parse Arguments */
    if (argc != 2) {
	Tcl_SetResult(interp, "Error: should be Nset_SDsurf id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Tcl_GetInt(interp, argv[1], &id) != TCL_OK)
	return (TCL_ERROR);

    GS_set_SDsurf(id);

    return (TCL_OK);
}

int Nunset_SDsurf_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    if (argc != 1) {
	Tcl_SetResult(interp, "Error: should be Nunset_SDsurf", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_unset_SDsurf();

    return (TCL_OK);
}

int Nset_SDscale_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    double scale1;
    float scale;

    if (argc != 2) {
	Tcl_SetResult(interp, "Error: should be Nset_SDscale scale", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[1], &scale1) != TCL_OK)
	return (TCL_ERROR);

    scale = (float)scale1;

    GS_set_SDscale(scale);

    return (TCL_OK);
}
