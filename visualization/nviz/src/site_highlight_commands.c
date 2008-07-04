/***************************************************************
 *
 * MODULE:       site_highlight_commands.c 1.0
 *
 * AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
 *
 * PURPOSE: Tcl commands created:
 *
 *    Sites depending commands
 *
 *    Nsite_highlight "what" "site_id" "cat" ["value"]
 *       - highlights a specific geopoint -
 *      what = color/size/marker/default
 *            default: panel highlight type and values are used instead of "value"
 *      site_id = (string range map_name 5 end)
 *      cat = DB category associated to the specific geopoint
 *      [value] (optional) = #rrggbb for color
 *                           float for size
 *                           int for marker
 *              when omitted default value is used
 *
 *    Nsite_highlight_list "what" "site_id" "catlist" ["value"]
 *       - highlights a list of geopoints -
 *      what = color/size/marker/default
 *            default: panel highlight type and values are used instead of "value"
 *      site_id = (string range map_name 5 end)
 *      catlist = list of DB category associated to the each geopoint
 *      [value] (optional) = #rrggbb for color
 *                           float for size
 *                           int for marker
 *              when omitted default value is used
 *
 *
 *    Nsite_unhighlight "what" "site_id" "cat"
 *       - unhighlights a specific geopoint -
 *      what = color/size/marker
 *      site_id = (string range map_name 5 end)
 *      cat = DB category associated to the specific geopoint
 *
 *    Nsite_unhighlight_list "what" "site_id" "catlist"
 *       - unhighlights a list of geopoints -
 *      what = color/size/marker/default
 *            default: panel highlight type and values are used instead of "value"
 *      site_id = (string range map_name 5 end)
 *      catlist = list of DB category associated to the each geopoint
 *
 *    Nsite_unhighlight_all "what" "site_id"
 *       - unhighlights all site geopoints -
 *      what = color/size/marker/all
 *             all: unhighlights color, size and marker
 *      site_id = (string range map_name 5 end)
 *
 *
 *    Sites independent commands (for panel_highlight.tcl)
 *
 *    Nsite_highlight_set_default "what" "value"
 *       - sets highlight default type -
 *      what = color/size/marker
 *      value = 1/0 (1: turns ON / 0 turns OFF color/size/marker highlight)
 *
 *    Nsite_highlight_get_default "what"
 *       - returns highlight default type -
 *      what = color/size/marker
 *
 *    Nsite_highlight_set_default_value "what" "value"
 *       - sets highlight default value -
 *      what = color/size/marker
 *      value = color/size/marker value to set
 *
 *    Nsite_highlight_get_default_value "what"
 *       - returns highlight default value -
 *      what = color/size/marker
 *
 *
 * REQUIREMENTS: external files with needed modifications:
 *
 *   - visualization/src
 *       visualization/nviz/src/Makefile
 *       visualization/nviz/src/nviz_init.c
 *
 *   - lib/ogsf
 *       lib/ogsf/gstypes.h
 *       lib/ogsf/Gp3.c
 *       lib/ogsf/gpd.c
 *
 *   See following "README" section for details
 *
 *
 * COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#if 0
*******************************************************************************
******************** site_highlight "README" **********************************
*******************************************************************************

Files involved:

- src
(mod) visualization/nviz/src/Makefile
(new) visualization/nviz/src/site_highlight_commands.c
(mod) visualization/nviz/src/nviz_init.c

- ogsf
(mod) lib/ogsf/gstypes.h
(mod) lib/ogsf/Gp3.c
(mod) lib/ogsf/gpd.c

- script
(new) visualization/nviz/scripts/panel_highlight.tcl



- src

(mod) visualization/nviz/src/Makefile
	added compilation for site_highlight_commands.c

(new) visualization/nviz/src/site_highlight_commands.c
	this file: creates Tcl commands

(mod) visualization/nviz/src/nviz_init.c
	added call to function: site_highlight_init_tcl(interp, &data);
	to intialize all the Tcl commands


- ogsf

(mod) lib/ogsf/gstypes.h

	added in struct g_point:
	...
	int highlight_color;
	int highlight_size;
	int highlight_marker;

	TRUE/FALSE flags to enable/disable proper highlight

	int highlight_color_value;
	float highlight_size_value;
	int highlight_marker_value;
	...

	proper highlight values used for each geopoint


(mod) lib/ogsf/Gp3.c
	added line:
		gpt->highlight_color = gpt->highlight_size = gpt->highlight_marker = FALSE;
	in order to get the proper initializations


(mod) lib/ogsf/gpd.c
	added:
		if (gpt->highlight_color) color = gpt->highlight_color_value;
		if (gpt->highlight_size) marker = gpt->highlight_marker_value;
		if (gpt->highlight_marker) size *= gpt->highlight_size_value;

	in function "gpd_obj_site_attr" (that replaces function "gpd_obj")



- script
(new) visualization/nviz/scripts/panel_highlight.tcl

			to panelIndex file add:
				"highlight"
			to tclIndex file add:
				set auto_index(mkhighlightPanel) "source $dir/panel_highlight.tcl"

*******************************************************************************
******************** end site_highlight "README" ******************************
*******************************************************************************
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "interface.h"

#define SITE_HIGHLIGHT_ALL		0
#define SITE_HIGHLIGHT_COLOR	1
#define SITE_HIGHLIGHT_SIZE		2
#define SITE_HIGHLIGHT_MARKER	3

static int default_highlight_color_value = 0xffffff;	/* white */
static float default_highlight_size_value = 1.2f;
static int default_highlight_marker_value = 9;		/* ST_GYRO */

static int default_highlight_color = 1;
static int default_highlight_size = 0;
static int default_highlight_marker = 0;


/* Color conversion function */
int site_highlight_get_int_BBGGRR(char* rrggbb)
{
/* rrggbb is in the form of #RRGGBB (first char is skipped) */
	char strbuf[16];
	memcpy(strbuf + 0, rrggbb + 5, 2);
	memcpy(strbuf + 2, rrggbb + 3, 2);
	memcpy(strbuf + 4, rrggbb + 1, 2);
	memset(strbuf + 6, 0, 1);
	return(strtol(strbuf, NULL, 16));
}



int Nsite_highlight_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_highlight_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);

int Nsite_unhighlight_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_unhighlight_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_unhighlight_all_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);


int Nsite_highlight_set_default_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_highlight_get_default_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_highlight_set_default_value_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_highlight_get_default_value_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);


/* called by nviz_init.c */

void site_highlight_init_tcl(Tcl_Interp * interp, Nv_data * data)
{
	Tcl_CreateCommand(interp, "Nsite_highlight", (Tcl_CmdProc*)Nsite_highlight_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_unhighlight", (Tcl_CmdProc*)Nsite_unhighlight_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_highlight_list", (Tcl_CmdProc*)Nsite_highlight_list_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_unhighlight_list", (Tcl_CmdProc*)Nsite_unhighlight_list_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_unhighlight_all", (Tcl_CmdProc*)Nsite_unhighlight_all_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_highlight_set_default", (Tcl_CmdProc*)Nsite_highlight_set_default_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_highlight_get_default", (Tcl_CmdProc*)Nsite_highlight_get_default_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_highlight_set_default_value", (Tcl_CmdProc*)Nsite_highlight_set_default_value_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_highlight_get_default_value", (Tcl_CmdProc*)Nsite_highlight_get_default_value_cmd, data, NULL);
}


/*not used for now*/
geosite * site_highlight_get_geosite(char* name)
{
	geosite * gp;
	int i, n, *id;

	id = GP_get_site_list(&n);
	for (i=0; i<n; i++) {
		gp = gp_get_site(id[i]);
		if (!strcmp(gp->filename, name)) {
			free(id);
			return gp;
		}
	}
	free(id);
	return NULL;
}


geopoint * site_highlight_get_geopoint(geosite * gp, int cat)
{
	geopoint * gpt;

	for (gpt = gp->points; gpt; gpt = gpt->next) {
		if (gpt->cat == cat) return gpt;
	}
	return NULL;
}


int site_highlight_loop(geosite * gp, const char ** argvPtr, int argcPtr, int what, int flag, float value)
{
	geopoint * gpt;
	int i;

	switch (what) {
		case SITE_HIGHLIGHT_SIZE:
			for(i=0; i<argcPtr; i++) {
				if ((gpt = site_highlight_get_geopoint(gp, atoi(argvPtr[i]))) == NULL) return(0);
				gpt->highlight_size = flag;
				gpt->highlight_size_value = value;
			}
		break;
		case SITE_HIGHLIGHT_COLOR:
			for(i=0; i<argcPtr; i++) {
				if ((gpt = site_highlight_get_geopoint(gp, atoi(argvPtr[i]))) == NULL) return(0);
				gpt->highlight_color = flag;
				gpt->highlight_color_value = (int)value;
			}
		break;
		case SITE_HIGHLIGHT_MARKER:
			for(i=0; i<argcPtr; i++) {
				if ((gpt = site_highlight_get_geopoint(gp, atoi(argvPtr[i]))) == NULL) return(0);
				gpt->highlight_marker = flag;
				gpt->highlight_marker_value = (int)value;
			}
		break;
		case SITE_HIGHLIGHT_ALL:
			for(i=0; i<argcPtr; i++) {
				if ((gpt = site_highlight_get_geopoint(gp, atoi(argvPtr[i]))) == NULL) return(0);
				gpt->highlight_size = flag;
				gpt->highlight_color = flag;
				gpt->highlight_marker = flag;

				gpt->highlight_size_value = value;
				gpt->highlight_color_value = (int)value;
				gpt->highlight_marker_value = (int)value;
			}
		break;
	}
	return(1);
}


/* Tcl Commands */

int Nsite_highlight_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) / argv[2] = id / argv[3] = category / [argv[4] = color] */
	geosite * gp;
	geopoint * gpt;

	if (argc < 4) return(TCL_ERROR);
	if ((gp = gp_get_site(atoi(argv[2]))) == NULL) return(TCL_ERROR);
	if ((gpt = site_highlight_get_geopoint(gp, atoi(argv[3]))) == NULL) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		if (argc == 5) gpt->highlight_size_value = atof(argv[4]);
		else gpt->highlight_size_value = default_highlight_size_value;
		gpt->highlight_size = TRUE;
	}
	else if (!strcmp(argv[1], "color")) {
		if (argc == 5) gpt->highlight_color_value = site_highlight_get_int_BBGGRR(argv[4]);
		else gpt->highlight_color_value = default_highlight_color_value;
		gpt->highlight_color = TRUE;
	}
	else if (!strcmp(argv[1], "marker")) {
		if (argc == 5) gpt->highlight_marker_value = atoi(argv[4]);
		else gpt->highlight_marker_value = default_highlight_marker_value;
		gpt->highlight_marker = TRUE;
	}
	else if (!strcmp(argv[1], "default")) {
		if (default_highlight_size != 0) {
			gpt->highlight_size_value = default_highlight_size_value;
			gpt->highlight_size = TRUE;
		}
		if (default_highlight_color != 0) {
			gpt->highlight_color_value = default_highlight_color_value;
			gpt->highlight_color = TRUE;
		}
		if (default_highlight_marker != 0) {
			gpt->highlight_marker_value = default_highlight_marker_value;
			gpt->highlight_marker = TRUE;
		}
	}
	else
		return(TCL_ERROR);

	return(TCL_OK);
}


int Nsite_highlight_list_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) / argv[2] = id / argv[3] = list of categories / [argv[4] = color] */
	int argcPtr;
	const char **argvPtr;
	float value;
	geosite * gp;

	if (argc < 4) return(TCL_ERROR);
	if ((gp = gp_get_site(atoi(argv[2]))) == NULL) return(TCL_ERROR);

	if (TCL_OK != Tcl_SplitList(interp, argv[3], &argcPtr, &argvPtr)) return (TCL_ERROR);


	if (!strcmp(argv[1], "size")) {
		if (argc == 5) value = atof(argv[4]);
		else value = default_highlight_size_value;

		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_SIZE, TRUE, value) == 0) goto error;
	}
	else if (!strcmp(argv[1], "color")) {
		if (argc == 5) value = (float)site_highlight_get_int_BBGGRR(argv[4]);
		else value = (float)default_highlight_color_value;

		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_COLOR, TRUE, value) == 0) goto error;
	}
	else if (!strcmp(argv[1], "marker")) {
		if (argc == 5) value = (float)atoi(argv[4]);
		else value = (float)default_highlight_marker_value;

		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_MARKER, TRUE, value) == 0) goto error;
	}
	else if (!strcmp(argv[1], "default")) {
		if (default_highlight_size != 0) {
			value = default_highlight_size_value;
			if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_SIZE, TRUE, value) == 0) goto error;
		}
		if (default_highlight_color != 0) {
			value = (float)default_highlight_color_value;
			if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_COLOR, TRUE, value) == 0) goto error;
		}
		if (default_highlight_marker != 0) {
			value = (float)default_highlight_marker_value;
			if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_MARKER, TRUE, value) == 0) goto error;
		}
	}
	else
		goto error;

	Tcl_Free((char *) argvPtr);
	return(TCL_OK);

error:
	if (argvPtr) Tcl_Free((char *) argvPtr);
	return(TCL_ERROR);
}

int Nsite_unhighlight_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker/all) / argv[2] = id / argv[3] = category */
	geosite * gp;
	geopoint * gpt;

	if (argc < 4) return(TCL_ERROR);
	if ((gp = gp_get_site(atoi(argv[2]))) == NULL) return(TCL_ERROR);
	if ((gpt = site_highlight_get_geopoint(gp, atoi(argv[3]))) == NULL) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) 		gpt->highlight_size = FALSE;
	else if (!strcmp(argv[1], "color")) gpt->highlight_color = FALSE;
	else if (!strcmp(argv[1], "marker"))gpt->highlight_marker = FALSE;
	else if (!strcmp(argv[1], "all")) {
		gpt->highlight_color=FALSE;
		gpt->highlight_size=FALSE;
		gpt->highlight_marker=FALSE;
	}
	else return(TCL_ERROR);

	return(TCL_OK);
}

int Nsite_unhighlight_list_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) / argv[2] = id / argv[3] = list of categories / [argv[4] = color] */
	int argcPtr;
	const char **argvPtr;
	geosite * gp;

	if (argc < 4) return(TCL_ERROR);
	if ((gp = gp_get_site(atoi(argv[2]))) == NULL) return(TCL_ERROR);

	if (TCL_OK != Tcl_SplitList(interp, argv[3], &argcPtr, &argvPtr)) return (TCL_ERROR);


	if (!strcmp(argv[1], "size")) {
		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_SIZE, FALSE, 0.0) == 0) goto error;
	}
	else if (!strcmp(argv[1], "color")) {
		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_COLOR, FALSE, 0.0) == 0) goto error;
	}
	else if (!strcmp(argv[1], "marker")) {
		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_MARKER, FALSE, 0.0) == 0) goto error;
	}
	else if (!strcmp(argv[1], "all")) {
		if (site_highlight_loop(gp, argvPtr, argcPtr, SITE_HIGHLIGHT_ALL, FALSE, 0.0) == 0) goto error;
	}
	else
		goto error;

	Tcl_Free((char *) argvPtr);
	return(TCL_OK);

error:
	if (argvPtr) Tcl_Free((char *) argvPtr);
	return(TCL_ERROR);
}


int Nsite_unhighlight_all_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker/all) / argv[2] = id */
	geosite * gp;
	geopoint* gpt;

	if (argc < 3) return(TCL_ERROR);
	if ((gp = gp_get_site(atoi(argv[2]))) == NULL) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		for (gpt = gp->points; gpt; gpt = gpt->next) gpt->highlight_size=FALSE;
	}
	else if (!strcmp(argv[1], "color")) {
		for (gpt = gp->points; gpt; gpt = gpt->next) gpt->highlight_color=FALSE;
	}
	else if (!strcmp(argv[1], "marker")) {
		for (gpt = gp->points; gpt; gpt = gpt->next) gpt->highlight_marker=FALSE;
	}
	else if (!strcmp(argv[1], "all")) {
		for (gpt = gp->points; gpt; gpt = gpt->next) {
			gpt->highlight_color=FALSE;
			gpt->highlight_size=FALSE;
			gpt->highlight_marker=FALSE;
		}
	}
	else
		return(TCL_ERROR);

	return(TCL_OK);
}

/*
  Commands for highlight panel, not depending from sites
*/

int Nsite_highlight_set_default_value_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) / argv[2] = value */

	if (argc < 3) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		default_highlight_size_value = atof(argv[2]);
	}
	else if (!strcmp(argv[1], "color")) {
		default_highlight_color_value = site_highlight_get_int_BBGGRR(argv[2]);
	}
	else if (!strcmp(argv[1], "marker")) {
		default_highlight_marker_value = atoi(argv[2]);
	}
	else
		return(TCL_ERROR);

	return(TCL_OK);
}

int Nsite_highlight_get_default_value_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) */
	char buf[256];

	if (argc < 2) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		sprintf(buf, "%.1f", default_highlight_size_value);
	}
	else if (!strcmp(argv[1], "color")) {
		sprintf(buf, "#%x", default_highlight_color_value);
	}
	else if (!strcmp(argv[1], "marker")) {
		sprintf(buf, "%d", default_highlight_marker_value);
	}
	else
		return(TCL_ERROR);

	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return(TCL_OK);
}





int Nsite_highlight_set_default_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) / argv[2] = value */

	if (argc < 3) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		default_highlight_size = atoi(argv[2]);
	}
	else if (!strcmp(argv[1], "color")) {
		default_highlight_color = atoi(argv[2]);
	}
	else if (!strcmp(argv[1], "marker")) {
		default_highlight_marker = atoi(argv[2]);
	}
	else
		return(TCL_ERROR);

	return(TCL_OK);
}

int Nsite_highlight_get_default_cmd(Nv_data * data, Tcl_Interp *interp, int argc, char **argv)
{
/* argv[1] = what (color/size/marker) */
	char buf[256];

	if (argc < 2) return(TCL_ERROR);

	if (!strcmp(argv[1], "size")) {
		sprintf(buf, "%d", default_highlight_size);
	}
	else if (!strcmp(argv[1], "color")) {
		sprintf(buf, "%d", default_highlight_color);
	}
	else if (!strcmp(argv[1], "marker")) {
		sprintf(buf, "%d", default_highlight_marker);
	}
	else
		return(TCL_ERROR);

	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return(TCL_OK);
}

