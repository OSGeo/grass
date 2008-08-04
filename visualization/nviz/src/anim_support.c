/*
   anim_support.c --

   This file contains keyframe animation support hooks for tcl/tk.
 */

/* gsf includes */
#include <grass/keyframe.h>

/* Nvision includes */
#include "interface.h"

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>

/* ********************************************************
   Nset_interp_mode_cmd --

   Hook for Nset_interp_mode tcl/tk command.

   Arguments:
   Either "linear" or "spline"

   Returns:
   None

   Side Effects:
   Sets the interpolation mode for the current keyframe animation.
   ******************************************************** */
int Nset_interp_mode_cmd(Nv_data * data,	/* Local data */
			 Tcl_Interp * interp,	/* Current interpreter */
			 int argc,	/* Number of arguments */
			 char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    int mode;

    if (argc != 2) {
	interp->result = "Error: should be Nset_interp_mode linear | spline";
	return (TCL_ERROR);
    }

    if (!strncmp(argv[1], "linear", 6))
	mode = KF_LINEAR;
    else if (!strncmp(argv[1], "spline", 6))
	mode = KF_SPLINE;
    else {
	interp->result =
	    "Error: interpoloation type must be either linear or spline";
	return (TCL_ERROR);
    }

    /* Set the interpolation type in the library */
    GK_set_interpmode(mode);

    return (TCL_OK);
}

/* ********************************************************
   Nset_tension_cmd --

   Hook for Nset_tension tcl/tk command.

   Arguments:
   A float value between 0.0 and 1.0 inclusive.

   Returns:
   None.

   Side Effects:
   Sets tension for interpolating splines
   ******************************************************** */
int Nset_tension_cmd(Nv_data * data,	/* Local data */
		     Tcl_Interp * interp,	/* Current interpreter */
		     int argc,	/* Number of arguments */
		     char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    double tension;

    if (argc != 2) {
	interp->result = "Error: should be Nset_tension float_value";
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[1], &tension) != TCL_OK)
	return (TCL_ERROR);

    if ((tension < 0) || (tension > 1)) {
	interp->result =
	    "Error: float_value should be between 0 and 1 inclusive";
	return (TCL_ERROR);
    }

    /* Now set the tension value */
    GK_set_tension((float)tension);

    return (TCL_OK);
}

/* ********************************************************
   Nshowtension_start
   Nupdate_tension
   Nshowtension_stop --

   Hooks for initializing and stopping multi-view
   display of path when changing tension.  (Currently
   do nothing).

   Arguments:
   None.

   Returns:
   None.

   Side Effects:
   None.
   ******************************************************** */
int Nshowtension_start_cmd(Nv_data * data,	/* Local data */
			   Tcl_Interp * interp,	/* Current interpreter */
			   int argc,	/* Number of arguments */
			   char **argv	/* Argument strings */
    )
{
    /* Parse arguments */

    if (argc != 1) {
	interp->result = "Error: should be Nshowtension_start";
	return (TCL_ERROR);
    }

    /* Call the function */
    GK_showtension_start();

    return (TCL_OK);
}

int Nupdate_tension_cmd(Nv_data * data,	/* Local data */
			Tcl_Interp * interp,	/* Current interpreter */
			int argc,	/* Number of arguments */
			char **argv	/* Argument strings */
    )
{
    /* Parse arguments */

    if (argc != 1) {
	interp->result = "Error: should be Nupdate_tension";
	return (TCL_ERROR);
    }

    /* Call the function */
    GK_update_tension();

    return (TCL_OK);
}

int Nshowtension_stop_cmd(Nv_data * data,	/* Local data */
			  Tcl_Interp * interp,	/* Current interpreter */
			  int argc,	/* Number of arguments */
			  char **argv	/* Argument strings */
    )
{
    /* Parse arguments */

    if (argc != 1) {
	interp->result = "Error: should be Nshowtension_stop";
	return (TCL_ERROR);
    }

    /* Call the function */
    GK_showtension_stop();

    return (TCL_OK);
}

/* ********************************************************
   Nupdate_frames --

   Hook for recalculating the interpolation path using the
   current number of frames requested.  Should be called
   anytime the number of frames changes or a keyframe
   changes (i.e. moved, added or deleted)

   Arguments:
   None.

   Returns:
   None.

   Side Effects:
   None.
   ******************************************************** */
int Nupdate_frames_cmd(Nv_data * data,	/* Local data */
		       Tcl_Interp * interp,	/* Current interpreter */
		       int argc,	/* Number of arguments */
		       char **argv	/* Argument strings */
    )
{
    /* Parse arguments */

    if (argc != 1) {
	interp->result = "Error: should be Nupdate_frames";
	return (TCL_ERROR);
    }

    /* Call the function */
    GK_update_frames();

    return (TCL_OK);

}

/* ********************************************************
   Nset_numsteps --

   Hook for setting the number of frames to be
   interpolated from keyframes.

   Arguments:
   An integer number of frames.

   Returns:
   None.

   Side Effects:
   Sets the current number of frames.
   ******************************************************** */
int Nset_numsteps_cmd(Nv_data * data,	/* Local data */
		      Tcl_Interp * interp,	/* Current interpreter */
		      int argc,	/* Number of arguments */
		      char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    long num_frames;

    if (argc != 2) {
	interp->result = "Error: should be Nset_numsteps #_frames";
	return (TCL_ERROR);
    }

    if (Tcl_GetInt(interp, argv[1], (int *)&num_frames) != TCL_OK)
	return (TCL_ERROR);

    /* Now set the number of frames */
    GK_set_numsteps((int)num_frames);

    return (TCL_OK);

}

/* ********************************************************
   Nclear_keys --

   Hook for clearing all keyframes.

   Arguments:
   None.

   Returns:
   None.

   Side Effects:
   Nukes all keyframes.
   ******************************************************** */
int Nclear_keys_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{
    /* Parse arguments */

    if (argc != 1) {
	interp->result = "Error: should be Nclear_keys";
	return (TCL_ERROR);
    }

    /* Call the function */
    GK_clear_keys();

    return (TCL_OK);

}

/* ********************************************************
   Nadd_key --

   Hook to add a key frame to the internal keyframe list.
   Here's the function def from Bill:
   The pos value is the relative position in the animation for this
   particular keyframe - used to compare relative distance to neighboring
   keyframes, it can be any floating point value.

   The fmask value can be any of the following or'd together:
   KF_FROMX_MASK
   KF_FROMY_MASK
   KF_FROMZ_MASK
   KF_FROM_MASK (KF_FROMX_MASK | KF_FROMY_MASK | KF_FROMZ_MASK)

   KF_DIRX_MASK
   KF_DIRY_MASK
   KF_DIRZ_MASK
   KF_DIR_MASK (KF_DIRX_MASK | KF_DIRY_MASK | KF_DIRZ_MASK)

   KF_FOV_MASK
   KF_TWIST_MASK

   KF_ALL_MASK (KF_FROM_MASK | KF_DIR_MASK | KF_FOV_MASK | KF_TWIST_MASK)

   Other fields will be added later.
   (Mark - I'm still working on this - just use KF_ALL_MASK for now)

   The value precis and the boolean force_replace are used to determine
   if a keyframe should be considered to be at the same position as a
   pre-existing keyframe. e.g., if anykey.pos - newkey.pos <= precis,
   GK_add_key will fail unless force_replace is TRUE.

   Returns 1 if key is added, otherwise -1.
   Calls GK_update_frames() if key is successfully added.


   Arguments:
   pos    - relative position of keyframe
   fmask  - list, see above, for tcl/tk constants have same names
   force_replace - boolean, see above
   precis - float, see above

   Returns:
   None.

   Side Effects:
   Adds or replaces the given key with the given values.

   ******************************************************** */
int Nadd_key_cmd(Nv_data * data,	/* Local data */
		 Tcl_Interp * interp,	/* Current interpreter */
		 int argc,	/* Number of arguments */
		 char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    double pos, precis;
    unsigned long fmask;
    int force_replace;
    const char **listels;
    int numels, i;

    if (argc != 5) {
	interp->result =
	    "Error: should be Nadd_key pos fmask_list force_replace precis";
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[1], &pos) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetDouble(interp, argv[4], &precis) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetBoolean(interp, argv[3], &force_replace) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_SplitList(interp, argv[2], &numels, &listels) != TCL_OK)
	return TCL_ERROR;

    fmask = 0;
    for (i = 0; i < numels; i++) {
	if (!strncmp(listels[i], "KF_FROMX_MASK", 13)) {
	    fmask |= KF_FROMX_MASK;
	}
	else if (!strncmp(listels[i], "KF_FROMY_MASK", 13)) {
	    fmask |= KF_FROMY_MASK;
	}
	else if (!strncmp(listels[i], "KF_FROMZ_MASK", 13)) {
	    fmask |= KF_FROMZ_MASK;
	}
	else if (!strncmp(listels[i], "KF_FROM_MASK", 12)) {
	    fmask |= KF_FROM_MASK;
	}
	else if (!strncmp(listels[i], "KF_DIRX_MASK", 12)) {
	    fmask |= KF_DIRX_MASK;
	}
	else if (!strncmp(listels[i], "KF_DIRY_MASK", 12)) {
	    fmask |= KF_DIRY_MASK;
	}
	else if (!strncmp(listels[i], "KF_DIRZ_MASK", 12)) {
	    fmask |= KF_DIRZ_MASK;
	}
	else if (!strncmp(listels[i], "KF_DIR_MASK", 11)) {
	    fmask |= KF_DIR_MASK;
	}
	else if (!strncmp(listels[i], "KF_FOV_MASK", 11)) {
	    fmask |= KF_FOV_MASK;
	}
	else if (!strncmp(listels[i], "KF_TWIST_MASK", 13)) {
	    fmask |= KF_TWIST_MASK;
	}
	else if (!strncmp(listels[i], "KF_ALL_MASK", 11)) {
	    fmask |= KF_ALL_MASK;
	}
	else {
	    sprintf(interp->result, "Error: mask constant %s not understood",
		    listels[i]);
	    Tcl_Free((char *)listels);
	    return (TCL_ERROR);
	}
    }

    Tcl_Free((char *)listels);

    /* Call the function */
    GK_add_key((float)pos, fmask, force_replace, (float)precis);

    return (TCL_OK);

}

/* ********************************************************
   Ndelete_key --

   Delete a keyframe.

   Arguments:
   Floating point position (i.e. time)
   Floating point precision
   Single delete? (boolean)

   Returns:
   Number of keys deleted.

   Side Effects:
   if single delete is false then removes all keyframes
   within precision of position.  Otherwise removes the
   first (lowest pos) keyframe within precision of position.

   ******************************************************** */
int Ndelete_key_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    double pos, precis;
    int justone;
    int num_deleted;

    if (argc != 4) {
	interp->result = "Error: should be Ndelete_key pos precis justone";
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[1], &pos) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetDouble(interp, argv[2], &precis) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetBoolean(interp, argv[3], &justone) != TCL_OK)
	return TCL_ERROR;

    /* Call the function */
    num_deleted = GK_delete_key((float)pos, (float)precis, justone);

    sprintf(interp->result, "%d", num_deleted);
    return (TCL_OK);

}

/* ********************************************************
   Nmove_key --

   Move a keyframe.

   Arguments:
   Floating point old position (i.e. time)
   Floating point precision
   Floating point new position (i.e. time)

   Returns:
   Number of keys moved (either 1 or 0)

   Side Effects:
   Moves the specified key from old_position +- precision
   to new_position +- precision

   ******************************************************** */
int Nmove_key_cmd(Nv_data * data,	/* Local data */
		  Tcl_Interp * interp,	/* Current interpreter */
		  int argc,	/* Number of arguments */
		  char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    double new_pos, old_pos, precis;
    int num_moved;

    if (argc != 4) {
	interp->result = "Error: should be Nmove_key oldpos precis newpos";
	return (TCL_ERROR);
    }

    if (Tcl_GetDouble(interp, argv[1], &old_pos) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetDouble(interp, argv[2], &precis) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetDouble(interp, argv[3], &new_pos) != TCL_OK)
	return TCL_ERROR;

    /* Call the function */
    num_moved = GK_move_key((float)old_pos, (float)precis, (float)new_pos);


    G_debug(3, "Arguments to move_key %f %f %f\n",
	    (float)old_pos, (float)precis, (float)new_pos);
    G_debug(3, "Frames moved = %d\n", num_moved);

    sprintf(interp->result, "%d", num_moved);
    return (TCL_OK);

}

/* ********************************************************
   Ndo_framestep --

   Hook to move animation to frame number given.
   Should be a value between 1 and the number of frames.

   Arguments:
   Integer frame number.
   Boolean false for fast-mesh, true for full-rendering

   Returns:
   None.

   Side Effects:
   Moves the animation to the given frame.
   ******************************************************** */
int Ndo_framestep_cmd(Nv_data * data,	/* Local data */
		      Tcl_Interp * interp,	/* Current interpreter */
		      int argc,	/* Number of arguments */
		      char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    long step;
    int render_type;

    if (argc != 3) {
	interp->result =
	    "Error: should be Ndo_framestep frame_# [TRUE | FALSE]";
	return (TCL_ERROR);
    }

    if (Tcl_GetInt(interp, argv[1], (int *)&step) != TCL_OK)
	return (TCL_ERROR);

    if (Tcl_GetBoolean(interp, argv[2], &render_type) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_do_framestep((int)step, render_type);

    return (TCL_OK);

}

/* ********************************************************
   Nshow_site, Nshow_vect, Nshow_path --

   Hook to draw the current sites, vectors, or paths.

   Arguments:
   Boolean, if true then draw.

   Returns:
   None.

   Side Effects:
   Draws the current path on the screen.
   ******************************************************** */
int Nshow_site_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    int arg1;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nshow_site [ TRUE | FALSE] ";
	return (TCL_ERROR);
    }

    /* Parse out the boolean value */
    if (Tcl_GetBoolean(interp, argv[1], &arg1) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_show_site(arg1);

    return (TCL_OK);

}

int Nprint_keys_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nprint_keys filename";
	return (TCL_ERROR);
    }

    /* Call function */
    GK_print_keys(argv[1]);

    return (TCL_OK);

}

int Nshow_vect_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    int arg1;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nshow_vect [ TRUE | FALSE] ";
	return (TCL_ERROR);
    }

    /* Parse out the boolean value */
    if (Tcl_GetBoolean(interp, argv[1], &arg1) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_show_vect(arg1);

    return (TCL_OK);

}

int Nshow_vol_cmd(Nv_data * data,	/* Local data */
		  Tcl_Interp * interp,	/* Current interpreter */
		  int argc,	/* Number of arguments */
		  char **argv	/* Argument strings */
    )
{
    int arg1;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nshow_vol [ TRUE | FALSE] ";
	return (TCL_ERROR);
    }

    /* Parse out the boolean value */
    if (Tcl_GetBoolean(interp, argv[1], &arg1) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_show_vol(arg1);

    return (TCL_OK);

}

int Nshow_lab_cmd(Nv_data * data,	/* Local data */
		  Tcl_Interp * interp,	/* Current interpreter */
		  int argc,	/* Number of arguments */
		  char **argv	/* Argument strings */
    )
{
    int arg1;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nshow_lab [ TRUE | FALSE] ";
	return (TCL_ERROR);
    }

    /* Parse out the boolean value */
    if (Tcl_GetBoolean(interp, argv[1], &arg1) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_show_list(arg1);

    return (TCL_OK);

}


int Nshow_path_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    int arg1;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nshow_path [ TRUE | FALSE] ";
	return (TCL_ERROR);
    }

    /* Parse out the boolean value */
    if (Tcl_GetBoolean(interp, argv[1], &arg1) != TCL_OK)
	return (TCL_ERROR);

    /* Call the function */
    GK_show_path(arg1);

    return (TCL_OK);

}

/* ********************************************************
   Nwrite_ppm -

   Save current GL screen to an ppm file.

   Arguments:
   String - name of file to save to.

   Returns:
   None.

   Side Effects:
   Saves the current GL screen to the given file.
   ******************************************************** */
int Nwrite_ppm_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nwrite_ppm file_name";
	return (TCL_ERROR);
    }

    /* Call the function */
    GS_write_ppm(argv[1]);

    return (TCL_OK);

}

/* ********************************************************
   Nwrite_tif -

   Save current GL screen to a TIFF file.

   Arguments:
   String - name of file to save to.

   Returns:
   None.

   Side Effects:
   Saves the current GL screen to the given file.
   ******************************************************** */
int Nwrite_tif_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Nwrite_ppm file_name";
	return (TCL_ERROR);
    }

#ifdef HAVE_TIFFIO_H
    /* Call the function */
    GS_write_tif(argv[1]);
#else
    interp->result = "Error: no TIFF support";
    return (TCL_ERROR);
#endif

    return (TCL_OK);

}

/********************************************************* */
int Noff_screen_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{

    int flag;
    int x, y;
    int width, height, maxx, maxy;

    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Noff_screen flag";
	return (TCL_ERROR);
    }

    flag = atoi(argv[1]);
    GS_zoom_setup(&x, &y, &width, &height, &maxx, &maxy);

    if (flag == 1) {
	if (Create_OS_Ctx(width, height) == -1) {
	    interp->result = "Error: Off screen context returned error";
	    return (TCL_ERROR);
	}

    }
    else {
	if (Destroy_OS_Ctx() == -1) {
	    interp->result = "Error: Destroy context returned error";
	    return (TCL_ERROR);
	}
    }

    return (TCL_OK);

}


/*******************************************************
 *  
*******************************************************/
int Ninit_mpeg_cmd(Nv_data * data,	/* Local data */
		   Tcl_Interp * interp,	/* Current interpreter */
		   int argc,	/* Number of arguments */
		   char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 2) {
	interp->result = "Error: should be Ninit_mpeg file_name";
	return (TCL_ERROR);
    }

    /* Call the function */
    if (gsd_init_mpeg(argv[1])) {
	interp->result =
	    "Error: gsd_init_mpeg failed to initialize MPEG stream";
	return (TCL_ERROR);
    }

    return (TCL_OK);

}

/*******************************************************
 *
*******************************************************/
int Nwrite_mpeg_frame_cmd(Nv_data * data,	/* Local data */
			  Tcl_Interp * interp,	/* Current interpreter */
			  int argc,	/* Number of arguments */
			  char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 1) {
	interp->result = "Error: should be Nwrite_mpeg_frame";
	return (TCL_ERROR);
    }

    /* Call the function */
    gsd_write_mpegframe();

    return (TCL_OK);

}

/*******************************************************
 *
*******************************************************/
int Nclose_mpeg_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{
    /* Parse arguments */
    if (argc != 1) {
	interp->result = "Error: should be Nclose_mpeg";
	return (TCL_ERROR);
    }

    /* Call the function */
    gsd_close_mpeg();

    return (TCL_OK);

}
