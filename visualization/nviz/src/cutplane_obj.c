#include "interface.h"
#include <stdlib.h>

/*
   Nset_fence_color_cmd --

   Command to set appropriate fence color

   Arguments:
   one of ABOVE, BELOW, BLEND, GREY, OFF
   Returns:
   None
   Side Effects:
   Sets appropriate fence color
 */
int
Nset_fence_color_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		     char **argv)
{
    /* Verify arguments */
    if (argc != 2) {
	sprintf(interp->result,
		"Usage: Nset_fence_color ABOVE | BELOW | BLEND | GREY | OFF");
	return (TCL_ERROR);
    }

    /* Otherwise parse the second argument and call the appropriate function */
    if (!strncmp(argv[1], "ABOVE", 5)) {
	GS_set_fencecolor(FC_ABOVE);
    }
    else if (!strncmp(argv[1], "BELOW", 5)) {
	GS_set_fencecolor(FC_BELOW);
    }
    else if (!strncmp(argv[1], "BLEND", 5)) {
	GS_set_fencecolor(FC_BLEND);
    }
    else if (!strncmp(argv[1], "GREY", 4)) {
	GS_set_fencecolor(FC_GREY);
    }
    else if (!strncmp(argv[1], "OFF", 3)) {
	GS_set_fencecolor(FC_OFF);
    }
    else {
	sprintf(interp->result,
		"Error in Nset_fence_color, second argument must be one of ABOVE, BELOW, BLEND, GREY or OFF.");
	return (TCL_ERROR);
    }

    return (TCL_OK);

}

/*
   Nget_fence_color_cmd --

   Command to set appropriate fence color

   Arguments:
   None
   Returns:
   one of ABOVE, BELOW, BLEND, GREY, OFF
   Side Effects:
   None
 */
int
Nget_fence_color_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		     char **argv)
{
    int mode;

    /* Verify arguments */
    if (argc != 1) {
	Tcl_SetResult(interp, "Usage: Nget_fence_color", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Get the mode and return an appropriate value */
    mode = GS_get_fencecolor();

    switch (mode) {
    case FC_ABOVE:
	Tcl_SetResult(interp, "ABOVE", TCL_VOLATILE);
	break;
    case FC_BELOW:
	Tcl_SetResult(interp, "BELOW", TCL_VOLATILE);
	break;
    case FC_BLEND:
	Tcl_SetResult(interp, "BLEND", TCL_VOLATILE);
	break;
    case FC_GREY:
	Tcl_SetResult(interp, "GREY", TCL_VOLATILE);
	break;
    case FC_OFF:
	Tcl_SetResult(interp, "OFF", TCL_VOLATILE);
	break;
    default:
	Tcl_SetResult(interp,
		      "Internal Error: Unknown mode returned from GS_get_fencecolor",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/*
   Ncutplane_obj_cmd --

   Command to intercept commands for cutplane objects.

   Arguments:
   one of:
   draw [surf1 surf2]
   on
   off
   set_rot dx dy dz
   set_trans dx dy dz
   get_rot
   get_trans
   Returns:
   get_rot - returns dx dy dz of rotation for current plane
   get_trans - returns dx dy dz of rotation for current plane
   Side Effects:
   draw - draws the given cutplane
   on - sets the cutplane as active
   off - deactivates the given cutplane
   set_rot - sets rotation for the current cutplane
   set_trans - sets the translation for the current cutplane
 */
int Ncutplane_obj_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    int id;

    id = get_cp_idnum(argv[0]);

    if (!strcmp(argv[1], "draw"))
	return (draw_cp_obj(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "on"))
	return (on_cp_obj(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "off"))
	return (off_cp_obj(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "state"))
	return (state_cp_obj(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "set_rot"))
	return (cp_set_rot(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "set_trans"))
	return (cp_set_trans(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "get_rot"))
	return (cp_get_rot(data, interp, id, argv, argc));
    else if (!strcmp(argv[1], "get_trans"))
	return (cp_get_trans(data, interp, id, argv, argc));
    else {
	sprintf(interp->result,
		"Usage: %s \tdraw [surf1 surf2]\n\t\ton\n\t\toff\n\t\tset_rot dx dy dz\n\t\tset_trans dx dy dz\n\t\tget_rot\n\t\tget_trans",
		argv[0]);
	return (TCL_ERROR);
    }
}

/*
   Nnew_cutplane_obj_cmd --

   Creates a cutplane object.

   Arguments:
   id - Id number for the new cutplane, should be unique
   Returns:
   The name of the command for the new cutplane.
   Side Effects:
   Cutplanes are implemented as Tcl commands which communicate
   with the underlying GSF library.  Each time this routine is
   called a new tcl command is created for the new cutplane.
   I.e. basically an oops approach.
 */
int Nnew_cutplane_obj_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			  int argc,	/* Number of arguments. */
			  char **argv	/* Argument strings. */
    )
{
    /*
       Pretty simple.  Since the number of cutplanes is fixed and
       we'll create them all ahead of time anyway we just let
       the user decide on the id for each.
     */
    char id[128];
    int num_id;

    if (argc != 2) {
	interp->result = "Usage: Nnew_cutplane_obj id_num";
	return (TCL_ERROR);
    }

    data->NumCplanes++;
    num_id = atoi(argv[1]);
    sprintf(id, "Ncutplane%d", num_id);

    /* Initialize internal attributes for this cutplane */
    data->Cp_rot[num_id][Y] = data->Cp_rot[num_id][Z] = 0.0;
    data->Cp_rot[num_id][X] = 0.0;
    data->Cp_trans[num_id][X] = data->Cp_trans[num_id][Y] =
	data->Cp_trans[num_id][Z] = 0.0;
    data->Cp_on[num_id] = 0;

    Tcl_CreateCommand(interp, id, (Tcl_CmdProc *) Ncutplane_obj_cmd, data,
		      NULL);
    Tcl_SetResult(interp, id, TCL_VOLATILE);

    return (TCL_OK);
}

/*
   Nnum_cutplane_obj_cmd --

   Return the number of cutplane objects currently allocated.

   Arguments:
   None
   Returns:
   Number of cutplane objects in existence.
   Side Effects:
   None
 */
int Nnum_cutplane_obj_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			  int argc,	/* Number of arguments. */
			  char **argv	/* Argument strings. */
    )
{
    char result[20];

    sprintf(result, "%d", data->NumCplanes);
    Tcl_SetResult(interp, result, TCL_VOLATILE);
    return (TCL_OK);
}

/*
 */
int Nset_current_cutplane_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			      int argc,	/* Number of arguments. */
			      char **argv	/* Argument strings. */
    )
{

    return (TCL_OK);
}

/*
   Nget_current_cutplane_cmd --

   Get the current active cutplane.

   Arguments:
   None
   Returns:
   NAME of current cutplane
   Side Effects:
   None
 */
int Nget_current_cutplane_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			      int argc,	/* Number of arguments. */
			      char **argv	/* Argument strings. */
    )
{
    char curr[128];

    if (data->CurCplane == -1) {
	sprintf(curr, "None");
    }
    else {
	sprintf(curr, "Ncutplane%d", data->CurCplane);
    }

    Tcl_SetResult(interp, curr, TCL_VOLATILE);
    return (TCL_OK);
}

/*
   Nget_map_list type id|name 
   returns list of all map objects of specified type, 
   either by name or id.
 */
int Nget_cutplane_list_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    return (TCL_OK);
}

/*
   cp_draw --

   Routine to draw all the cutplanes.

   Arguments:
   Current active cutplane
   Data for Nviz application
   Boundary surfaces (surf1 and surf2), set to -1 to ignore
   Returns:
   None
   Side Effects:
   Causes all the active cut planes to be drawn
 */
void cp_draw(int current, Nv_data * data, int surf1, int surf2)
{
    int i, nsurfs;
    int surf_min = 0, surf_max = 0, temp;
    int *surf_list;

    GS_set_draw(GSD_BACK);
    GS_clear(data->BGcolor);
    GS_ready_draw();

    /* If surf boundaries present then find them */
    surf_list = GS_get_surf_list(&nsurfs);
    if ((surf1 != -1) && (surf2 != -1)) {
	for (i = 0; i < nsurfs; i++) {
	    if (surf_list[i] == surf1)
		surf_min = i;
	    if (surf_list[i] == surf2)
		surf_max = i;
	}

	if (surf_max < surf_min) {
	    temp = surf_min;
	    surf_min = surf_max;
	    surf_max = temp;
	}

	surf_max++;
    }
    else {
	surf_min = 0;
	surf_max = nsurfs;
    }

    if (nsurfs > 1) {
	for (i = 0; i < MAX_CPLANES; i++) {
	    if (data->Cp_on[i])
		GS_draw_cplane_fence(surf_list[0], surf_list[1], i);
	}
    }

    for (i = surf_min; i < surf_max; i++) {
	GS_draw_wire(surf_list[i]);
    }

    GS_done_draw();
}

/*
   draw_cp_obj --

   Draw the current cutplane.

   Arguments:
   [Optional] surf1, surf2 - Only draws between surf1 and surf2
   Returns:
   None
   Side Effects:
   None
 */
int
draw_cp_obj(Nv_data * data, Tcl_Interp * interp, int id, char **argv,
	    int argc)
{
    int bound1, bound2;

    /* Verify arguments */
    if ((argc != 2) && (argc != 4)) {
	sprintf(interp->result, "Usage: %s draw [surf1 surf2]", argv[0]);
	return (TCL_ERROR);
    }

    /* Parse boundary surfaces if necessary (should be surface id's) */
    if (argc == 2) {
	bound1 = bound2 = -1;
    }
    else {
	if (Tcl_GetInt(interp, argv[2], &bound1) != TCL_OK) {
	    sprintf(interp->result,
		    "Error in %s draw %s %s, expected integer argument in surf1 field",
		    argv[0], argv[2], argv[3]);
	    return (TCL_ERROR);
	}

	if (Tcl_GetInt(interp, argv[3], &bound2) != TCL_OK) {
	    sprintf(interp->result,
		    "Error in %s draw %s %s, expected integer argument in surf2 field",
		    argv[0], argv[2], argv[3]);
	    return (TCL_ERROR);
	}
    }

    /* Finally make the draw call */
    cp_draw(data->CurCplane, data, bound1, bound2);

    return (TCL_OK);
}

/*
   on_cp_obj --

   Turn on (make current) the given cutplane.

   Arguments:
   None
   Returns:
   None
   Side Effects:
   Sets the current cutplane as the currently active
   cutplane in the gsf library.
 */
int
on_cp_obj(Nv_data * data, Tcl_Interp * interp, int id, char **argv, int argc)
{
    /* Verify arguments are correct */
    if (argc != 2) {
	sprintf(interp->result, "Usage: %s on", argv[0]);
	return (TCL_ERROR);
    }

    /* Otherwise make the current cutplane active */
    data->CurCplane = id;
    data->Cp_on[id] = 1;
    GS_set_cplane(id);

    return (TCL_OK);
}

/*
   off_cp_obj --

   Turn off (make inactive) the given cutplane.

   Arguments:
   None
   Returns:
   None
   Side Effects:
   Sets the current cutplane as inactive in the gsf library.
 */
int
off_cp_obj(Nv_data * data, Tcl_Interp * interp, int id, char **argv, int argc)
{
    /* Verify arguments */
    if (argc != 2) {
	sprintf(interp->result, "Usage: %s off", argv[0]);
	return (TCL_ERROR);
    }

    /* Otherwise make the current cutplane inactive */
    data->Cp_on[id] = 0;
    GS_unset_cplane(id);

    return (TCL_OK);
}

/*
   state_cp_obj --

   Return the state (on/off) of the given cutplane.

   Arguments:
   None
   Returns:
   None
   Side Effects:
   None
 */
int
state_cp_obj(Nv_data * data, Tcl_Interp * interp, int id, char **argv,
	     int argc)
{
    /* Verify arguments */
    if (argc != 2) {
	sprintf(interp->result, "Usage: %s state", argv[0]);
	return (TCL_ERROR);
    }

    /* Otherwise figure out the state */
    if (data->Cp_on[id])
	Tcl_SetResult(interp, "on", TCL_VOLATILE);
    else
	Tcl_SetResult(interp, "off", TCL_VOLATILE);

    return (TCL_OK);
}

/*
   cp_set_rot --

   Set the rotation for the current cutplane.

   Arguments:
   dx, dy, dz - components of rotation
   Returns:
   None
   Side Effects:
   Sets the rotation parameters for the given cutplane.
 */
int
cp_set_rot(Nv_data * data, Tcl_Interp * interp, int id, char **argv, int argc)
{
    double dx, dy, dz;

    /* Verify arguments */
    if (argc != 5) {
	sprintf(interp->result, "Usage: %s set_rot dx dy dz", argv[0]);
	return (TCL_ERROR);
    }

    /* Extract nummerical arguments */
    if (Tcl_GetDouble(interp, argv[2], &dx) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_rot %s %s %s, expected numerical argument in dx field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[3], &dy) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_rot %s %s %s, expected numerical argument in dy field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[4], &dz) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_rot %s %s %s, expected numerical argument in dz field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    /* Set internal rotation and call gsf library */
    data->Cp_rot[id][X] = (float)dx;
    data->Cp_rot[id][Y] = (float)dy;
    data->Cp_rot[id][Z] = (float)dz;
    GS_set_cplane_rot(id, data->Cp_rot[id][X], data->Cp_rot[id][Y],
		      data->Cp_rot[id][Z]);

    cp_draw(data->CurCplane, data, -1, -1);

    return (TCL_OK);
}

/*
   cp_set_trans --

   Set the translation for the current cutplane.

   Arguments:
   dx, dy, dz - values for setting translation
   Returns:
   None
   Side Effects:
   Sets translation of current cutplane
 */
int
cp_set_trans(Nv_data * data, Tcl_Interp * interp, int id, char **argv,
	     int argc)
{
    double dx, dy, dz;

    /* Verify arguments */
    if (argc != 5) {
	sprintf(interp->result, "Usage: %s set_trans dx dy dz", argv[0]);
	return (TCL_ERROR);
    }

    /* Extract nummerical arguments */
    if (Tcl_GetDouble(interp, argv[2], &dx) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_trans %s %s %s, expected numerical argument in dx field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[3], &dy) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_trans %s %s %s, expected numerical argument in dy field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[4], &dz) != TCL_OK) {
	sprintf(interp->result,
		"Error in %s set_trans %s %s %s, expected numerical argument in dz field",
		argv[0], argv[2], argv[3], argv[4]);
	return (TCL_ERROR);
    }

    /* Set internal rotation and call gsf library */
    data->Cp_trans[id][X] = (float)dx;
    data->Cp_trans[id][Y] = (float)dy;
    data->Cp_trans[id][Z] = (float)dz;
    GS_set_cplane_trans(id, data->Cp_trans[id][X], data->Cp_trans[id][Y],
			data->Cp_trans[id][Z]);

    cp_draw(data->CurCplane, data, -1, -1);

    return (TCL_OK);
}

/*
   cp_get_rot --

   Get the rotation values for the current cutplane.

 */
int
cp_get_rot(Nv_data * data, Tcl_Interp * interp, int id, char **argv, int argc)
{
    char x[32], y[32], z[32];

    if (argc != 2) {
	sprintf(interp->result, "Usage: %s get_rot", argv[0]);
	return (TCL_ERROR);
    }

    sprintf(x, "%f", data->Cp_rot[id][X]);
    sprintf(y, "%f", data->Cp_rot[id][Y]);
    sprintf(z, "%f", data->Cp_rot[id][Z]);

    Tcl_AppendElement(interp, x);
    Tcl_AppendElement(interp, y);
    Tcl_AppendElement(interp, z);

    return (TCL_OK);
}

/*
   cp_get_trans --

   Get the translation values for the current cutplane.

 */
int
cp_get_trans(Nv_data * data, Tcl_Interp * interp, int id, char **argv,
	     int argc)
{
    char x[32], y[32], z[32];

    if (argc != 2) {
	sprintf(interp->result, "Usage: %s get_trans", argv[0]);
	return (TCL_ERROR);
    }

    sprintf(x, "%f", data->Cp_trans[id][X]);
    sprintf(y, "%f", data->Cp_trans[id][Y]);
    sprintf(z, "%f", data->Cp_trans[id][Z]);

    Tcl_AppendElement(interp, x);
    Tcl_AppendElement(interp, y);
    Tcl_AppendElement(interp, z);

    return (TCL_OK);
}

/*
   get_cp_idnum --

   Strip the id number out of the name of a
   cutplane object.

   Arguments:
   Name of cutplane object
   Returns:
   Integer id of cutplane object
   Side Effects:
   None
 */
int get_cp_idnum(char *name)
{
    static char comp[] = "Ncutplane";

    /* Strip out "Ncutplane" from name */
    return (atoi(&name[strlen(comp)]));
}
