/* ***************************************************************
 * *
 * * MODULE:       v.clean
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 * *
 * * PURPOSE:      Clean vector features
 * *               
 * * COPYRIGHT:    (C) 2001-2009 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

int main(int argc, char *argv[])
{
    struct Map_info In, Out, Err, *pErr;
    int i, otype, with_z;
    struct GModule *module;
    struct Option *in_opt, *field_opt, *out_opt, *type_opt, *tool_opt, *thresh_opt,
	*err_opt;
    struct Flag *no_build_flag;
    int *tools, ntools, atools;
    double *threshs;
    int level;
    int count;
    double size;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("topology"));
    module->description = _("Toolset for cleaning topology of vector map.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->answer = "-1";
    field_opt->guisection = _("Selection");
    type_opt = G_define_standard_option(G_OPT_V3_TYPE);
    type_opt->guisection = _("Selection");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    err_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    err_opt->key = "error";
    err_opt->description = _("Name of output map where errors are written");
    err_opt->required = NO;

    tool_opt = G_define_option();
    tool_opt->key = "tool";
    tool_opt->type = TYPE_STRING;
    tool_opt->required = YES;
    tool_opt->multiple = YES;
    tool_opt->options =
	"break,snap,rmdangle,chdangle,rmbridge,chbridge,rmdupl,rmdac,bpol,prune,"
	"rmarea,rmline,rmsa";
    tool_opt->description = _("Cleaning tool");
    tool_opt->descriptions =
	_("break;break lines at each intersection;"
	  "rmdupl;remove duplicate geometry features (pay attention to categories!);"
	  "rmdangle;remove dangles, threshold ignored if < 0;"
	  "chdangle;change the type of boundary dangle to line, "
	  "threshold ignored if < 0, input line type is ignored;"
	  "rmbridge;remove bridges connecting area and island or 2 islands;"
	  "chbridge;change the type of bridges connecting area and island "
	  "or 2 islands from boundary to line;"
	  "snap;snap lines to vertex in threshold;"
	  "rmdac;remove duplicate area centroids ('type' option ignored);"
	  "bpol;break (topologically clean) polygons (imported from "
	  "non topological format, like ShapeFile). Boundaries are broken on each "
	  "point shared between 2 and more polygons where angles of segments are different;"
	  "prune;remove vertices in threshold from lines and boundaries, "
	  "boundary is pruned only if topology is not damaged (new intersection, "
	  "changed attachement of centroid), first and last segment of the boundary "
	  "is never changed;"
	  "rmarea;remove small areas, the longest boundary with adjacent area is removed;"
	  "rmline;remove all lines or boundaries of zero length, threshold is ignored;"
	  "rmsa;remove small angles between lines at nodes");

    thresh_opt = G_define_option();
    thresh_opt->key = "thresh";
    thresh_opt->type = TYPE_DOUBLE;
    thresh_opt->required = NO;
    thresh_opt->multiple = YES;
    thresh_opt->label = _("Threshold in map units, one value for each tool");
    thresh_opt->description = _("Default: 0.0[,0.0,...])");

    no_build_flag = G_define_flag();
    no_build_flag->key = 'b';
    no_build_flag->description =
	_("Don't build topology for the output vector");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    otype = Vect_option_to_types(type_opt);

    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 GV_FATAL_EXIT);
    if (err_opt->answer) {
	Vect_check_input_output_name(in_opt->answer, err_opt->answer,
				     GV_FATAL_EXIT);
    }

    atools = 20;
    tools = (int *)G_malloc(atools * sizeof(int));

    /* Read tools */
    ntools = 0;
    i = 0;
    if (strlen(tool_opt->answer) < 1)
	G_fatal_error(_("You must select at least one tool"));
    while (tool_opt->answers[i]) {
	if (i + 1 >= atools) {
	    atools += 20;
	    G_realloc(tools, atools * sizeof(int));
	}

	G_debug(1, "tool : %s", tool_opt->answers[i]);
	if (strcmp(tool_opt->answers[i], "break") == 0)
	    tools[ntools] = TOOL_BREAK;
	else if (strcmp(tool_opt->answers[i], "rmdupl") == 0)
	    tools[ntools] = TOOL_RMDUPL;
	else if (strcmp(tool_opt->answers[i], "rmdangle") == 0)
	    tools[ntools] = TOOL_RMDANGLE;
	else if (strcmp(tool_opt->answers[i], "chdangle") == 0)
	    tools[ntools] = TOOL_CHDANGLE;
	else if (strcmp(tool_opt->answers[i], "rmbridge") == 0)
	    tools[ntools] = TOOL_RMBRIDGE;
	else if (strcmp(tool_opt->answers[i], "chbridge") == 0)
	    tools[ntools] = TOOL_CHBRIDGE;
	else if (strcmp(tool_opt->answers[i], "snap") == 0)
	    tools[ntools] = TOOL_SNAP;
	else if (strcmp(tool_opt->answers[i], "rmdac") == 0)
	    tools[ntools] = TOOL_RMDAC;
	else if (strcmp(tool_opt->answers[i], "bpol") == 0)
	    tools[ntools] = TOOL_BPOL;
	else if (strcmp(tool_opt->answers[i], "prune") == 0)
	    tools[ntools] = TOOL_PRUNE;
	else if (strcmp(tool_opt->answers[i], "rmarea") == 0)
	    tools[ntools] = TOOL_RMAREA;
	else if (strcmp(tool_opt->answers[i], "rmsa") == 0)
	    tools[ntools] = TOOL_RMSA;
	else if (strcmp(tool_opt->answers[i], "rmline") == 0)
	    tools[ntools] = TOOL_RMLINE;
	else
	    G_fatal_error(_("Tool doesn't exist"));

	ntools++;
	i++;
    }

    G_debug(1, "ntools = %d", ntools);
    threshs = (double *)G_malloc(ntools * sizeof(double));

    /* Read thresholds */
    for (i = 0; i < ntools; i++)
	threshs[i] = 0.0;
    i = 0;
    while (thresh_opt->answers && thresh_opt->answers[i]) {
	threshs[i] = atof(thresh_opt->answers[i]);
	G_debug(1, "thresh : %s -> %f ", tool_opt->answers[i], threshs[i]);

	if (threshs[i] != 0 && tools[i] != TOOL_SNAP &&
	    tools[i] != TOOL_RMDANGLE && tools[i] != TOOL_CHDANGLE && 
	    tools[i] != TOOL_PRUNE && tools[i] != TOOL_RMAREA) {
	    G_warning(_("Threshold for tool %d may not be > 0, set to 0"),
		      i + 1);
	    threshs[i] = 0.0;
	}
	i++;
    }

    /* Print tool table */
    G_message(SEP);
    G_message(_("Tool: Threshold"));

    for (i = 0; i < ntools; i++) {
	switch (tools[i]) {
	case (TOOL_BREAK):
	    G_message("%s: %13e", _("Break"), threshs[i]);
	    break;
	case (TOOL_RMDUPL):
	    G_message("%s: %13e", _("Remove duplicates"), threshs[i]);
	    break;
	case (TOOL_RMDANGLE):
	    G_message("%s: %13e", _("Remove dangles"), threshs[i]);
	    break;
	case (TOOL_CHDANGLE):
	    G_message("%s: %13e", _("Change type of boundary dangles"),
		      threshs[i]);
	    break;
	case (TOOL_RMBRIDGE):
	    G_message("%s: %13e", _("Remove bridges"), threshs[i]);
	    break;
	case (TOOL_CHBRIDGE):
	    G_message("%s: %13e", _("Change type of boundary bridges"),
		      threshs[i]);
	    break;
	case (TOOL_SNAP):
	    G_message("%s: %13e", _("Snap vertices"), threshs[i]);
	    break;
	case (TOOL_RMDAC):
	    G_message("%s: %13e", _("Remove duplicate area centroids"),
		      threshs[i]);
	    break;
	case (TOOL_BPOL):
	    G_message("%s: %13e", _("Break polygons"), threshs[i]);
	    break;
	case (TOOL_PRUNE):
	    G_message("%s: %13e", _("Prune"), threshs[i]);
	    break;
	case (TOOL_RMAREA):
	    G_message("%s: %13e", _("Remove small areas"), threshs[i]);
	    break;
	case (TOOL_RMSA):
	    G_message("%s: %13e", _("Remove small angles at nodes"),
		      threshs[i]);
	    break;
	case (TOOL_RMLINE):
	    G_message("%s: %13e",
		      _("Remove all lines or boundaries of zero length"),
		      threshs[i]);
	    break;
	}
    }

    G_message(SEP);

    /* Input vector may be both on level 1 and 2. Level 2 is necessary for 
     * virtual centroids (shapefile/OGR) and level 1 is better if input is too big 
     * and build in previous module (like v.in.ogr or other call to v.clean) would take 
     * a long time */
    level = Vect_open_old2(&In, in_opt->answer, "", field_opt->answer);

    with_z = Vect_is_3d(&In);

    Vect_set_fatal_error(GV_FATAL_PRINT);
    if (0 > Vect_open_new(&Out, out_opt->answer, with_z)) {
	Vect_close(&In);
	exit(EXIT_FAILURE);
    }

    if (err_opt->answer) {
	Vect_set_fatal_error(GV_FATAL_PRINT);
	Vect_set_open_level(2);
	if (0 > Vect_open_new(&Err, err_opt->answer, with_z)) {
	    Vect_close(&In);
	    Vect_close(&Out);
	    exit(EXIT_FAILURE);
	}
	pErr = &Err;
    }
    else {
	pErr = NULL;
    }

    /* Copy input to output */
    G_message(_("Copying vector features..."));
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* This works for both level 1 and 2 */
    Vect_copy_map_lines_field(&In, Vect_get_field_number(&In, field_opt->answer), &Out);
    if (Vect_copy_tables(&In, &Out, 0))
        G_warning(_("Failed to copy attribute table to output map"));

    Vect_set_release_support(&In);
    Vect_close(&In);

    /* Start with GV_BUILD_NONE and for each tool use unly the necessary level! */

    for (i = 0; i < ntools; i++) {
	if (tools[i] == TOOL_RMDAC || tools[i] == TOOL_PRUNE ||
	    tools[i] == TOOL_RMAREA) {
	    if (Vect_get_built(&Out) >= GV_BUILD_CENTROIDS) {
		Vect_build_partial(&Out, GV_BUILD_CENTROIDS);
		G_message(SEP);
	    }
	    else {
		G_important_message(_("Rebuilding parts of topology..."));
		Vect_build_partial(&Out, GV_BUILD_CENTROIDS);
		G_message(SEP);
	    }
	}
	else {
	    if (Vect_get_built(&Out) >= GV_BUILD_BASE) {
		Vect_build_partial(&Out, GV_BUILD_BASE);
		G_message(SEP);
	    }
	    else {
		G_important_message(_("Rebuilding parts of topology..."));
		Vect_build_partial(&Out, GV_BUILD_BASE);
		G_message(SEP);
	    }
	}

	switch (tools[i]) {
	case TOOL_BREAK:
	    G_message(_("Tool: Break lines at intersections"));
	    Vect_break_lines(&Out, otype, pErr);
	    break;
	case TOOL_RMDUPL:
	    G_message(_("Tool: Remove duplicates"));
	    Vect_remove_duplicates(&Out, otype, pErr);
	    break;
	case TOOL_RMDANGLE:
	    G_message(_("Tool: Remove dangles"));
	    Vect_remove_dangles(&Out, otype, threshs[i], pErr);
	    break;
	case TOOL_CHDANGLE:
	    G_message(_("Tool: Change type of boundary dangles"));
	    Vect_chtype_dangles(&Out, threshs[i], pErr);
	    break;
	case TOOL_RMBRIDGE:
	    G_message(_("Tool: Remove bridges"));
	    Vect_remove_bridges(&Out, pErr);
	    break;
	case TOOL_CHBRIDGE:
	    G_message(_("Tool: Change type of boundary bridges"));
	    Vect_chtype_bridges(&Out, pErr);
	    break;
	case TOOL_RMDAC:
	    G_message(_("Tool: Remove duplicate area centroids"));
	    count = rmdac(&Out, pErr);
	    break;
	case TOOL_SNAP:
	    G_message(_("Tool: Snap line to vertex in threshold"));
	    Vect_snap_lines(&Out, otype, threshs[i], pErr);
	    break;
	case TOOL_BPOL:
	    G_message(_("Tool: Break polygons"));
	    Vect_break_polygons(&Out, otype, pErr);
	    break;
	case TOOL_PRUNE:
	    G_message(_("Tool: Prune lines/boundaries"));
	    prune(&Out, otype, threshs[i], pErr);
	    break;
	case TOOL_RMAREA:
	    G_message(_("Tool: Remove small areas"));
	    count =
		Vect_remove_small_areas(&Out, threshs[i], pErr, &size);
	    break;
	case TOOL_RMSA:
	    G_message(_("Tool: Remove small angles at nodes"));
	    count =
		Vect_clean_small_angles_at_nodes(&Out, otype, pErr);
	    break;
	case TOOL_RMLINE:
	    G_message(_("Tool: Remove all lines and boundaries of zero length"));
	    count = remove_zero_line(&Out, otype, pErr);
	    break;
	}

	G_message(SEP);
    }

    if (!no_build_flag->answer) {
	G_important_message(_("Rebuilding topology for output vector map..."));
	Vect_build_partial(&Out, GV_BUILD_NONE);
	Vect_build(&Out);
    }
    else {
	Vect_build_partial(&Out, GV_BUILD_NONE);	/* -> topo not saved */
    }
    Vect_close(&Out);

    if (pErr) {
	G_message(SEP);
	G_important_message(_("Building topology for error vector map..."));
	Vect_build(pErr);
	Vect_close(pErr);
    }

    exit(EXIT_SUCCESS);
}
