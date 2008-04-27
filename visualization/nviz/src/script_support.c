/*
 * script_support.c --
 *
 * This file provides a few commands for supporting scripting.
 * Basically, we just track any X events and flush them to
 * a file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <tk.h>

int Nv_script_state = 0;
FILE *Nv_script_file = NULL;

/*
 * ScriptAddString_Cmd --
 *
 * syntax: Nv_script_add_string string
 * Output a string to the current script file if one exists
 *
 */
int ScriptAddString_Cmd(ClientData clientData,	/* Main window associated with
						 * interpreter. */
			Tcl_Interp * interp,	/* Current interpreter. */
			int argc,	/* Number of arguments. */
			char **argv	/* Argument strings. */
    )
{
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nv_script_add_string string",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Nv_script_file != NULL) {
	fprintf(Nv_script_file, "%s\n", argv[1]);
    }

    return (TCL_OK);
}

/*
 * CloseScripting_Cmd --
 *
 * Closes the current scriptfile if one exists.
 */
int CloseScripting_Cmd(ClientData clientData,	/* Main window associated with
						 * interpreter. */
		       Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    if (argc != 1) {
	Tcl_SetResult(interp, "Usage: Nv_close_scripting", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Nv_script_file != NULL) {
	fprintf(Nv_script_file, "puts \"script complete\"\n");
	fclose(Nv_script_file);
    }

    return (TCL_OK);
}

/*
 * ScriptState_Cmd --
 *
 * Takes one argument to turn scripting on and off. 1=on 0=off
 */
int SetState_Cmd(ClientData clientData,	/* Main window associated with
					 * interpreter. */
		 Tcl_Interp * interp,	/* Current interpreter. */
		 int argc,	/* Number of arguments. */
		 char **argv	/* Argument strings. */
    )
{
    int val;

    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nv_set_script_state [0 | 1]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Nv_script_file == NULL) {
	Tcl_SetResult(interp, "no script file specified", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    Tcl_GetInt(interp, argv[1], &val);

    if (val)
	Nv_script_state = 1;
    else
	Nv_script_state = 0;

    return (TCL_OK);
}


/*
 * SetScriptFile_Cmd --
 *
 * Possibly open a new script file with the given name.
 */
int SetScriptFile_Cmd(ClientData clientData,	/* Main window associated with
						 * interpreter. */
		      Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nv_set_script_file file_name",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Nv_script_file != NULL) {
	fclose(Nv_script_file);
    }

    Nv_script_file = fopen(argv[1], "a");

    /* Do a little initialization for file looping */
    fprintf(Nv_script_file, "global Nv_mapLoopMode Nv_mapLoopFile\n");
    fprintf(Nv_script_file, "set Nv_mapLoopMode 0\n");
    fprintf(Nv_script_file, "set Nv_mapLoopFile \"\"\n");

    return (TCL_OK);
}
