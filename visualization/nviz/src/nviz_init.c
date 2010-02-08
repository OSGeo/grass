#include <stdio.h>
#include <stdlib.h>
#include "tk.h"
#include "interface.h"
#include <grass/gis.h>
#include "coldefs.h"
#include <grass/bitmap.h>
/* get from gislib: */
#include <grass/version.h>
#include <grass/glocale.h>

char startup_script[] = "toplevel .wait_ok\n\
label .wait_ok.wait -text \"Please wait...\" -fg red -bg black\n\
pack .wait_ok.wait -ipadx 20 -ipady 20 -expand 1 -fill both\n\
wm geometry .wait_ok \"+800+50\"\n\
wm geometry . \"+100+100\"\n\
wm title . \"NVIZ\"\n\
update\n\
grab .wait_ok.wait\n";

extern struct options opts;
extern int script_mode;

static int parse_command(Nv_data * data, Tcl_Interp * interp)
{
    char *arglist[3];
    const char *autoload;
    int i, aload = 1;

    G_debug(1, "nviz_init:parse_command()");

    {
	float defs[MAX_ATTS];

	defs[ATT_TOPO] = 0;
	defs[ATT_COLOR] = DEFAULT_SURF_COLOR;
	defs[ATT_MASK] = 0;
	defs[ATT_TRANSP] = 0;
	defs[ATT_SHINE] = 60;
	defs[ATT_EMIT] = 0;
	GS_set_att_defaults(defs, defs);
    }

    /* Put in the "please wait..." message unless we are in demo mode */
    if (!opts.demo->answer) {
	if (Tcl_Eval(interp, startup_script) != TCL_OK)
	    G_fatal_error("%s", Tcl_GetStringResult(interp));
    }

    G_verbose_message(" ");
    G_verbose_message(" ");
    G_verbose_message("Version: %s", GRASS_VERSION_STRING);
    G_verbose_message(" ");
    G_verbose_message("Authors: Bill Brown, Terry Baker, Mark Astley, David Gerdes");
    G_verbose_message("\tmodifications: Jaro Hofierka, Bob Covill");
    G_verbose_message(" ");
    G_verbose_message(" ");
    G_verbose_message("Please cite one or more of the following references in publications");
    G_verbose_message("where the results of this program were used:");
    G_verbose_message("Brown, W.M., Astley, M., Baker, T., Mitasova, H. (1995).");
    G_verbose_message("GRASS as an Integrated GIS and Visualization System for");
    G_verbose_message("Spatio-Temporal Modeling, Proceedings of Auto Carto 12, Charlotte, N.C.");
    G_verbose_message(" ");
    G_verbose_message("Mitasova, H., W.M. Brown, J. Hofierka, 1994, Multidimensional");
    G_verbose_message("dynamic cartography. Kartograficke listy, 2, p. 37-50.");
    G_verbose_message(" ");
    G_verbose_message("Mitas L., Brown W. M., Mitasova H., 1997, Role of dynamic");
    G_verbose_message("cartography in simulations of landscape processes based on multi-variate");
    G_verbose_message("fields. Computers and Geosciences, Vol. 23, No. 4, pp. 437-446");
    G_verbose_message(" ");
    G_verbose_message("http://www2.gis.uiuc.edu:2280/modviz/viz/nviz.html");
    G_verbose_message(" ");
    G_verbose_message("The papers are available at");
    G_verbose_message("http://www2.gis.uiuc.edu:2280/modviz/");


    /* Look for quickstart flag */
    if (opts.no_args->answer) {
	opts.elev->answers = NULL;
	opts.colr->answers = NULL;
	opts.vct->answers = NULL;
	opts.pnt->answers = NULL;
	opts.vol->answers = NULL;
    }


    /* Look for scriptkill flag */
    if (opts.script_kill->answer) {
	if (Tcl_VarEval(interp, "set NvizScriptKill 1 ", NULL) != TCL_OK)
	    G_fatal_error("%s", Tcl_GetStringResult(interp));
    }

    /* See if an alternative panel path is specified */
    if (opts.panel_path->answer) {
	/* If so then set the variable NvizAltPath to the alternative path
	 */
	if (Tcl_VarEval(interp, "set NvizAltPath ", opts.panel_path->answer,
			NULL) != TCL_OK)
	    G_fatal_error("%s", Tcl_GetStringResult(interp));
    }

    /* Get State file from command line */
    if (opts.state->answer) {
	if (Tcl_VarEval(interp, "set NvizLoadState ", opts.state->answer,
			NULL) != TCL_OK)
	    G_fatal_error("%s", Tcl_GetStringResult(interp));
    }

    /* See if a script file was specified */
    if (opts.script->answer) {
	/* If so then set the variable NvizPlayScript to the file */
	if (Tcl_VarEval(interp, "set NvizPlayScript ", opts.script->answer,
			NULL) != TCL_OK)
	    G_fatal_error("%s", Tcl_GetStringResult(interp));
    }

#ifdef XSCRIPT
    AUTO_FILE = opts.aut->answer;
    /* either file name or NULL */

    Write_script = opts.swrit->answer;
#endif

    /* Consult the user's .grassrc file to see if we should
     * automatically set the colormap of loaded surfaces to be
     * the same as the raster used for topography.  The appropriate
     * resource is:
     *     Nviz_AutoSurfColors
     * If this resource isn't specified, it defaults to true.
     */
    autoload = G__getenv("Nviz_AutoSurfColors");
    if ((autoload != NULL) && (!strcmp(autoload, "false")))
	aload = 0;

    /* Parse answeres from user */
    /* Run check to make sure elev == colr */
    if (opts.elev->answers && opts.colr->answers) {
	int ee = 0, cc = 0;

	for (i = 0; opts.elev->answers[i]; i++) {
	    ee = i;
	}
	for (i = 0; opts.colr->answers[i]; i++) {
	    cc = i;
	}
	if (ee != cc)
	    G_fatal_error
		(_("Number of elevation files does not match number of colors files"));
    }

    if (opts.elev->answers) {
	char tmp[30];

	for (i = 0; opts.elev->answers[i]; i++) {
	    arglist[1] = "surf";
	    arglist[2] = opts.elev->answers[i];

	    if (Nnew_map_obj_cmd(data, interp, 3, arglist) != TCL_OK) {
		G_warning(_("Loading data failed"));
		continue;
	    }

	    /* See if we should autoload the color file */
	    if (aload) {
		strncpy(tmp, Tcl_GetStringResult(interp), 29);
		if (opts.colr->answers) {
		    if (Tcl_VarEval(interp, tmp, " set_att color ",
				    opts.colr->answers[i], NULL) != TCL_OK)
			G_fatal_error("%s", Tcl_GetStringResult(interp));
		}
		else {
		    if (Tcl_VarEval(interp, tmp, " set_att color ",
				    opts.elev->answers[i], NULL) != TCL_OK)
			G_fatal_error("%s", Tcl_GetStringResult(interp));
		}
	    }
	}

	if (i > 1)
	    set_default_wirecolors(data, i);
    }

    if (opts.vct->answers) {
	if (!opts.elev->answers && GS_num_surfs() == 0) {
	    int i;
	    int *surf_list;

	    arglist[1] = "surf";
	    Nnew_map_obj_cmd(data, interp, 2, arglist);

	    surf_list = GS_get_surf_list(&i);
	    GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
	}
	for (i = 0; opts.vct->answers[i]; i++) {
	    arglist[1] = "vect";
	    arglist[2] = opts.vct->answers[i];
	    Nnew_map_obj_cmd(data, interp, 3, arglist);
	}
    }

    if (opts.pnt->answers) {
	if (!opts.elev->answers && GS_num_surfs() == 0) {
	    int i;
	    int *surf_list;

	    arglist[1] = "surf";
	    Nnew_map_obj_cmd(data, interp, 2, arglist);

	    surf_list = GS_get_surf_list(&i);
	    GS_set_att_const(surf_list[0], ATT_TRANSP, 255);
	}
	for (i = 0; opts.pnt->answers[i]; i++) {
	    arglist[1] = "site";
	    arglist[2] = opts.pnt->answers[i];
	    Nnew_map_obj_cmd(data, interp, 3, arglist);
	}
    }

    if (opts.vol->answers) {
	for (i = 0; opts.vol->answers[i]; i++) {
	    arglist[1] = "vol";
	    arglist[2] = opts.vol->answers[i];
	    Nnew_map_obj_cmd(data, interp, 3, arglist);
	}
    }




    return (TCL_OK);
}


int make_red_yellow_ramp(int *ramp, int num, int minval, int maxval)
{
    int g, i, incr;

    incr = (maxval - minval) / (num - 1);
    for (i = 0; i < num; i++) {
	g = minval + incr * i;
	RGB_TO_INT(maxval, g, 0, ramp[i]);
    }

    return 0;
}


/* Sorts surfaces by mid elevation, lowest to highest.
 * Puts ordered id numbers in id_sort, leaving surfs unchanged.
 * Puts ordered indices of surfaces from id_orig in indices.
 */
int sort_surfs_mid(int *id_sort, int *indices, int num)
{
    int i, j;
    float midvals[MAX_SURFS];
    float tmp, max = 0., tmin, tmax, tmid;
    int *surf_list;

    surf_list = GS_get_surf_list(&i);
    for (i = 0; i < num; i++) {
	GS_get_zextents(surf_list[i], &tmin, &tmax, &tmid);
	if (i == 0)
	    max = tmax;
	else
	    max = max < tmax ? tmax : max;
	midvals[i] = tmid;
    }

    for (i = 0; i < num; i++) {
	tmp = midvals[0];
	indices[i] = 0;
	for (j = 0; j < num; j++) {
	    if (midvals[j] < tmp) {
		tmp = midvals[j];
		indices[i] = j;
	    }
	}
	midvals[indices[i]] = max + 1;
	id_sort[i] = surf_list[indices[i]];
    }
    return (TCL_OK);
}

int set_default_wirecolors(Nv_data * dc, int surfs)
{

#ifdef DO_GREYSCALE
    int *surf_list;
    int i, color, greyincr, greyval;

    greyincr = 200 / (surfs + 1);	/* just use upper values */

    surf_list = GS_get_surf_list(&i);
    for (i = 0; i < surfs; i++) {
	greyval = 55 + greyincr * (i + 1);
	RGB_TO_INT(greyval, greyval, greyval, color);
	GS_set_wire_color(surf_list[i], color);
    }
    G_free(surf_list);

#else

    int i, ramp[MAX_SURFS];
    int sortSurfs[MAX_SURFS], sorti[MAX_SURFS];

    make_red_yellow_ramp(ramp, surfs, 30, 255);
    sort_surfs_mid(sortSurfs, sorti, surfs);

    for (i = 0; i < surfs; i++) {
	GS_set_wire_color(sortSurfs[i], ramp[i]);
    }

#endif
    return 0;
}

int Ninit(Tcl_Interp *interp, Tk_Window w)
{
    static Nv_data data;

    /* compile in the home directory */
    Tcl_SetVar(interp, "src_boot", getenv("GISBASE"), TCL_GLOBAL_ONLY);

    init_commands(interp, &data);

    Ninitdata(interp, &data);

    togl_flythrough_init_tcl(interp, &data);

    pick_init_tcl(interp, &data);

    return (TCL_OK);
}

extern void swap_togl(void);

int Ninitdata(Tcl_Interp *interp,	/* Current interpreter. */
	      Nv_data *data)
{
    GS_libinit();

    GVL_libinit();		/* TODO */

    GS_set_swap_func(swap_togl);
    data->NumCplanes = 0;
    data->CurCplane = 0;
    if (!script_mode)
	parse_command(data, interp);

    return (TCL_OK);
}
