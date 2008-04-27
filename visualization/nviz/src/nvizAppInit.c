/*
 * This file for the most part has been munged from the tk3.6/4.0
 * distribution and modified to work with Nviz.  The main function is
 * defined in tkMain.c
 */

#include "tk.h"
#include "togl.h"
#include "interface.h"



Tk_Window mainWindow;

extern void create_cb();
extern void reshape_cb();
extern void display_cb();

static void RunScripts(ClientData);

/*
 *----------------------------------------------------------------------
 *
 * RunScripts
 *
 *	This idle handler is used to run active scripts in background.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void RunScripts(ClientData clientData)
{
    Tcl_GlobalEval(clientData, "PlayNextLine");
}



/*
 *----------------------------------------------------------------------
 *
 * NVIZ_AppInit -- (based on tclAppInit)
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int NVIZ_AppInit(Tcl_Interp * interp	/* Interpreter for application. */
    )
{

    mainWindow = Tk_MainWindow(interp);

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /* added 3-12-99 to conform with 8.0.4 */
    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);

/*
  if (TkGLX_Init(interp, mainWindow) == TCL_ERROR) {
    return TCL_ERROR;
  }
*/

    if (Togl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Specify the C callback functions for widget creation, display,
     * and reshape.
     */
    Togl_CreateFunc(create_cb);
    Togl_DisplayFunc(display_cb);
    Togl_ReshapeFunc(reshape_cb);

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Added 26-Feb-2000 by Philip Warner so we can remove tkSpecialWait
     * Uncomment the line below if you want scripts to run in background...
     */
/*
  Tk_DoWhenIdle(RunScripts, interp);
*/

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    /* added 3-12-99 to conform with 8.0.4 */
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.nvizrc", TCL_GLOBAL_ONLY);

    Ninit(interp, mainWindow);

    return TCL_OK;
}
