/* ***************************************************************
 * *
 * * MODULE:       v.clean
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 * *
 * * PURPOSE:      Clean vector features
 * *               
 * * COPYRIGHT:    (C) 2001-2009, 2011 by the GRASS Development Team
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
    int i, otype, with_z, native;
    struct GModule *module;
    struct {
	struct Option *in, *field, *out, *type, *tool, *thresh,
	    *err;
    } opt;
    struct {
	struct Flag *no_build, *combine;
    } flag;
    int *tools, ntools, atools;
    double *threshs;
    int count;
    double size;
    char *desc;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("topology"));
    G_add_keyword(_("geometry"));
    module->description = _("Toolset for cleaning topology of vector map.");

    opt.in = G_define_standard_option(G_OPT_V_INPUT);

    opt.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.field->answer = "-1";
    opt.field->guisection = _("Selection");

    opt.type = G_define_standard_option(G_OPT_V3_TYPE);
    opt.type->guisection = _("Selection");

    opt.out = G_define_standard_option(G_OPT_V_OUTPUT);

    opt.err = G_define_standard_option(G_OPT_V_OUTPUT);
    opt.err->key = "error";
    opt.err->description = _("Name of output map where errors are written");
    opt.err->required = NO;

    opt.tool = G_define_option();
    opt.tool->key = "tool";
    opt.tool->type = TYPE_STRING;
    opt.tool->required = YES;
    opt.tool->multiple = YES;
    opt.tool->options =
	"break,snap,rmdangle,chdangle,rmbridge,chbridge,rmdupl,rmdac,bpol,prune,"
	"rmarea,rmline,rmsa";
    opt.tool->description = _("Cleaning tool");
    desc = NULL;
    G_asprintf(&desc,
	       "break;%s;"
	       "rmdupl;%s;"
	       "rmdangle;%s;"
	       "chdangle;%s;"
	       "rmbridge;%s;"
	       "chbridge;%s;"
	       "snap;%s;"
	       "rmdac;%s;"
	       "bpol;%s;"
	       "prune;%s;"
	       "rmarea;%s;"
	       "rmline;%s;"
	       "rmsa;%s",
	       _("break lines at each intersection"),
	       _("remove duplicate geometry features (pay attention to categories!)"),
	       _("remove dangles, threshold ignored if < 0"),
	       _("change the type of boundary dangle to line, "
		 "threshold ignored if < 0, input line type is ignored"),
	       _("remove bridges connecting area and island or 2 islands"),
	       _("change the type of bridges connecting area and island "
		 "or 2 islands from boundary to line"),
	       _("snap lines to vertex in threshold"),
	       _("remove duplicate area centroids ('type' option ignored)"),
	       _("break (topologically clean) polygons (imported from "
		 "non topological format, like ShapeFile). Boundaries are broken on each "
		 "point shared between 2 and more polygons where angles of segments are different"),
	       _("remove vertices in threshold from lines and boundaries, "
		 "boundary is pruned only if topology is not damaged (new intersection, "
		 "changed attachment of centroid), first and last segment of the boundary "
		 "is never changed"),
	       _("remove small areas, the longest boundary with adjacent area is removed"),
	       _("remove all lines or boundaries of zero length, threshold is ignored"),
	       _("remove small angles between lines at nodes"));
    opt.tool->descriptions = desc;
    
    opt.thresh = G_define_option();
    opt.thresh->key = "thresh";
    opt.thresh->type = TYPE_DOUBLE;
    opt.thresh->required = NO;
    opt.thresh->multiple = YES;
    opt.thresh->label = _("Threshold in map units, one value for each tool");
    opt.thresh->description = _("Default: 0.0[,0.0,...])");

    flag.no_build = G_define_flag();
    flag.no_build->key = 'b';
    flag.no_build->description =
	_("Don't build topology for the output vector");

    flag.combine = G_define_flag();
    flag.combine->key = 'c';
    flag.combine->description =
	_("Combine tools with recommended follow-up tools.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    otype = Vect_option_to_types(opt.type);

    Vect_check_input_output_name(opt.in->answer, opt.out->answer,
				 G_FATAL_EXIT);
    if (opt.err->answer) {
	Vect_check_input_output_name(opt.in->answer, opt.err->answer,
				     G_FATAL_EXIT);
    }

    atools = 20;
    tools = (int *)G_malloc(atools * sizeof(int));

    /* Read tools */
    ntools = 0;
    i = 0;
    if (strlen(opt.tool->answer) < 1)
	G_fatal_error(_("You must select at least one tool"));
    while (opt.tool->answers[i]) {
	if (i + 1 >= atools) {
	    atools += 20;
	    G_realloc(tools, atools * sizeof(int));
	}

	G_debug(1, "tool : %s", opt.tool->answers[i]);
	if (strcmp(opt.tool->answers[i], "break") == 0)
	    tools[ntools] = TOOL_BREAK;
	else if (strcmp(opt.tool->answers[i], "rmdupl") == 0)
	    tools[ntools] = TOOL_RMDUPL;
	else if (strcmp(opt.tool->answers[i], "rmdangle") == 0)
	    tools[ntools] = TOOL_RMDANGLE;
	else if (strcmp(opt.tool->answers[i], "chdangle") == 0)
	    tools[ntools] = TOOL_CHDANGLE;
	else if (strcmp(opt.tool->answers[i], "rmbridge") == 0)
	    tools[ntools] = TOOL_RMBRIDGE;
	else if (strcmp(opt.tool->answers[i], "chbridge") == 0)
	    tools[ntools] = TOOL_CHBRIDGE;
	else if (strcmp(opt.tool->answers[i], "snap") == 0)
	    tools[ntools] = TOOL_SNAP;
	else if (strcmp(opt.tool->answers[i], "rmdac") == 0)
	    tools[ntools] = TOOL_RMDAC;
	else if (strcmp(opt.tool->answers[i], "bpol") == 0)
	    tools[ntools] = TOOL_BPOL;
	else if (strcmp(opt.tool->answers[i], "prune") == 0)
	    tools[ntools] = TOOL_PRUNE;
	else if (strcmp(opt.tool->answers[i], "rmarea") == 0)
	    tools[ntools] = TOOL_RMAREA;
	else if (strcmp(opt.tool->answers[i], "rmsa") == 0)
	    tools[ntools] = TOOL_RMSA;
	else if (strcmp(opt.tool->answers[i], "rmline") == 0)
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
    while (opt.thresh->answers && opt.thresh->answers[i]) {
	threshs[i] = atof(opt.thresh->answers[i]);
	G_debug(1, "thresh : %s -> %f ", opt.tool->answers[i], threshs[i]);

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
	    G_message("%s: %.15g", _("Break"), threshs[i]);
	    break;
	case (TOOL_RMDUPL):
	    G_message("%s: %.15g", _("Remove duplicates"), threshs[i]);
	    break;
	case (TOOL_RMDANGLE):
	    G_message("%s: %.15g", _("Remove dangles"), threshs[i]);
	    break;
	case (TOOL_CHDANGLE):
	    G_message("%s: %.15g", _("Change type of boundary dangles"),
		      threshs[i]);
	    break;
	case (TOOL_RMBRIDGE):
	    G_message("%s: %.15g", _("Remove bridges"), threshs[i]);
	    break;
	case (TOOL_CHBRIDGE):
	    G_message("%s: %.15g", _("Change type of boundary bridges"),
		      threshs[i]);
	    break;
	case (TOOL_SNAP):
	    G_message("%s: %.15g", _("Snap vertices"), threshs[i]);
	    break;
	case (TOOL_RMDAC):
	    G_message("%s: %.15g", _("Remove duplicate area centroids"),
		      threshs[i]);
	    break;
	case (TOOL_BPOL):
	    G_message("%s: %.15g", _("Break polygons"), threshs[i]);
	    break;
	case (TOOL_PRUNE):
	    G_message("%s: %.15g", _("Prune"), threshs[i]);
	    break;
	case (TOOL_RMAREA):
	    G_message("%s: %.15g", _("Remove small areas"), threshs[i]);
	    break;
	case (TOOL_RMSA):
	    G_message("%s: %.15g", _("Remove small angles at nodes"),
		      threshs[i]);
	    break;
	case (TOOL_RMLINE):
	    G_message("%s: %.15g",
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
    Vect_open_old2(&In, opt.in->answer, "", opt.field->answer);

    with_z = Vect_is_3d(&In);
    
    if (0 > Vect_open_new(&Out, opt.out->answer, with_z)) {
	Vect_close(&In);
	exit(EXIT_FAILURE);
    }

    if (opt.err->answer) {
	Vect_set_open_level(2);
	if (0 > Vect_open_new(&Err, opt.err->answer, with_z)) {
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

    native = Vect_maptype(&Out) == GV_FORMAT_NATIVE;

    if (!native) {
	/* area cleaning tools might produce unexpected results for 
	 * non-native vectors */
	G_warning(_("Topological cleaning works best with native GRASS vector format"));
	/* Copy attributes (OGR format) */
	Vect_copy_map_dblinks(&In, &Out, TRUE);
    }
    
    /* This works for both level 1 and 2 */
    Vect_copy_map_lines_field(&In, Vect_get_field_number(&In, opt.field->answer), &Out);
    
    if (native) {
	/* Copy attribute tables (native format only) */
	if (Vect_copy_tables(&In, &Out, 0))
	    G_warning(_("Failed to copy attribute table to output vector map"));
    }
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
	    if (flag.combine->answer && (otype & GV_LINES)) {
		G_message(_("Tool: Split lines"));
		split_lines(&Out, otype, pErr);
	    }
	    G_message(_("Tool: Break lines at intersections"));
	    Vect_break_lines(&Out, otype, pErr);
	    if (flag.combine->answer) {
		G_message(_("Tool: Remove duplicates"));
		Vect_remove_duplicates(&Out, otype, pErr);
		if (otype & GV_LINES) {
		    G_message(_("Tool: Merge lines"));
		    Vect_merge_lines(&Out, otype, NULL, pErr);
		}
	    }
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
	    Vect_remove_bridges(&Out, pErr, NULL, NULL);
	    break;
	case TOOL_CHBRIDGE:
	    G_message(_("Tool: Change type of boundary bridges"));
	    Vect_chtype_bridges(&Out, pErr, NULL, NULL);
	    break;
	case TOOL_RMDAC:
	    G_message(_("Tool: Remove duplicate area centroids"));
	    count = rmdac(&Out, pErr);
	    break;
	case TOOL_SNAP:
	    G_message(_("Tool: Snap line to vertex in threshold"));
	    Vect_snap_lines(&Out, otype, threshs[i], pErr);
	    if (flag.combine->answer) {
		int nmod;

		if (otype & GV_LINES) {
		    G_message(_("Tool: Split lines"));
		    split_lines(&Out, otype, pErr);
		}
		do {
		    G_message(_("Tool: Break lines at intersections"));
		    Vect_break_lines(&Out, otype, pErr);
		    G_message(_("Tool: Remove duplicates"));
		    Vect_remove_duplicates(&Out, otype, pErr);
		    G_message(_("Tool: Remove small angles at nodes"));
		    nmod =
			Vect_clean_small_angles_at_nodes(&Out, otype, pErr);
		} while (nmod > 0);
		if (otype & GV_LINES) {
		    G_message(_("Tool: Merge lines"));
		    Vect_merge_lines(&Out, otype, NULL, pErr);
		}
	    }
	    break;
	case TOOL_BPOL:
	    G_message(_("Tool: Break polygons"));
	    Vect_break_polygons(&Out, otype, pErr);
	    if (flag.combine->answer) {
		G_message(_("Tool: Remove duplicates"));
		Vect_remove_duplicates(&Out, otype, pErr);
	    }
	    break;
	case TOOL_PRUNE:
	    G_message(_("Tool: Prune lines/boundaries"));
	    prune(&Out, otype, threshs[i], pErr);
	    break;
	case TOOL_RMAREA:
	    G_message(_("Tool: Remove small areas"));
	    count =
		Vect_remove_small_areas(&Out, threshs[i], pErr, &size);
	    if (flag.combine->answer && count > 0) {
		Vect_build_partial(&Out, GV_BUILD_BASE);
		G_message(SEP);
		G_message(_("Tool: Merge boundaries"));
		Vect_merge_lines(&Out, GV_BOUNDARY, NULL, pErr);
	    }
	    break;
	case TOOL_RMSA:
	    G_message(_("Tool: Remove small angles at nodes"));
	    if (!flag.combine->answer) {
		count =
		    Vect_clean_small_angles_at_nodes(&Out, otype, pErr);
	    }
	    else {
		int nmod;

		if (otype & GV_LINES) {
		    G_message(_("Tool: Split lines"));
		    split_lines(&Out, otype, pErr);
		}
		while ((nmod =
		          Vect_clean_small_angles_at_nodes(&Out, otype, pErr)) > 0) {
		    count += nmod;
		    G_message(_("Tool: Break lines at intersections"));
		    Vect_break_lines(&Out, otype, pErr);
		    G_message(_("Tool: Remove duplicates"));
		    Vect_remove_duplicates(&Out, otype, pErr);
		    G_message(_("Tool: Remove small angles at nodes"));
		}
		if (otype & GV_LINES) {
		    G_message(_("Tool: Merge lines"));
		    Vect_merge_lines(&Out, otype, NULL, pErr);
		}
	    }
	    break;
	case TOOL_RMLINE:
	    G_message(_("Tool: Remove all lines and boundaries of zero length"));
	    count = remove_zero_line(&Out, otype, pErr);
	    break;
	}

	G_message(SEP);
    }

    if (!flag.no_build->answer) {
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
