#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "global.h"
#include "proto.h"

/* Utilities */

/* For given type returns pointer to name */
char *get_line_type_name(int type)
{
    char *name;

    switch (type) {
    case GV_POINT:
	name = G_store("point");
	break;
    case GV_LINE:
	name = G_store("line");
	break;
    case GV_BOUNDARY:
	name = G_store("boundary");
	break;
    case GV_CENTROID:
	name = G_store("centroid");
	break;
    default:
	name = G_store("unknown type");
    }

    return name;
}

static int sxo, syo, mode;

void set_location(int x, int y)
{
    sxo = x;
    syo = y;
}

void set_mode(int m)
{
    mode = m;
}

static tool_func_update *tool_update;
static tool_func_end *tool_end;
static void *tool_closure;

static void end_tool(void)
{
    Tcl_Eval(Toolbox, ".screen.canvas configure -cursor {}");
    Tcl_Eval(Toolbox, ".screen.canvas delete active");

    if (tool_end)
	(*tool_end) (tool_closure);

    tool_update = NULL;
    tool_end = NULL;
    tool_closure = NULL;

    driver_close();
    next_tool();
}

void cancel_tool(void)
{
    end_tool();
}

int c_update_tool(ClientData cdata, Tcl_Interp * interp, int argc,
		  char *argv[])
{
    char buf[100];
    int x, y, b;

    G_debug(3, "c_update_tool()");

    if (argc < 4) {
	Tcl_SetResult(interp, "Usage: c_update_tool x y b", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!tool_update)
	return TCL_OK;

    Tcl_GetInt(interp, argv[1], &x);
    Tcl_GetInt(interp, argv[2], &y);
    Tcl_GetInt(interp, argv[3], &b);

    switch (mode) {
    case MOUSE_POINT:
	break;
    case MOUSE_LINE:
	sprintf(buf, "get_update_line %d %d %d %d", sxo, syo, x, y);
	Tcl_Eval(Toolbox, buf);
	break;
    case MOUSE_BOX:
	sprintf(buf, "get_update_box %d %d %d %d", sxo, syo, x, y);
	Tcl_Eval(Toolbox, buf);
	break;
    }

    if (b < 0) {
	update(x, y);
	return TCL_OK;
    }

    if (b == 0) {
	end_tool();
	return TCL_OK;
    }

    if ((*tool_update) (tool_closure, x, y, b)) {
	end_tool();
	return TCL_OK;
    }

    return TCL_OK;
}

void set_tool(tool_func_begin * begin_fn, tool_func_update * update_fn,
	      tool_func_end * end_fn, void *closure)
{
    int ret;

    if (tool_update)
	end_tool();

    driver_open();
    ret = (*begin_fn) (closure);

    if (ret) {
	driver_close();
	return;
    }

    tool_update = update_fn;
    tool_end = end_fn;
    tool_closure = closure;

    Tcl_Eval(Toolbox, ".screen.canvas configure -cursor crosshair");
}

/* Get snapping/selection threshold from GUI */
double get_thresh() {
    /* If not found, fall back to old calculation method */
    if (!var_geti(VAR_SNAP))
        return fabs(D_d_to_u_col(10) - D_d_to_u_col(0));

    if (var_geti(VAR_SNAP_MODE) == SNAP_MAP) {
        return fabs(var_getd(VAR_SNAP_MAP));
    }
    else {
        return fabs(Scale * var_geti(VAR_SNAP_SCREEN));
    }
}
