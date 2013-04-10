
/****************************************************************
 *
 * MODULE:     v.edit
 *
 * PURPOSE:    Non-interactive editing vector map.
 *
 * AUTHOR(S):  Wolf Bergenheim
 *             Jachym Cepicky
 *             Major updates by Martin Landa <landa.martin gmail.com>
 *
 * COPYRIGHT:  (C) 2006-2011, 2013 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that comes
 *             with GRASS for details.
 *
 * TODO:       3D support (done for move and vertexmove)
 ****************************************************************/

#include "global.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct GParams params;
    struct Map_info Map;
    struct Map_info **BgMap;	/* backgroud vector maps */
    int nbgmaps;		/* number of registrated background maps */
    enum mode action_mode;
    FILE *ascii;

    int i;
    int move_first, snap;
    int ret, layer;
    double move_x, move_y, move_z, thresh[3];

    struct line_pnts *coord;

    struct ilist *List;

    struct cat_list *Clist;

    ascii = NULL;
    List = NULL;
    BgMap = NULL;
    nbgmaps = 0;
    coord = NULL;
    Clist = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->overwrite = TRUE;
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("editing"));
    module->description = _("Edits a vector map, allows adding, deleting "
			    "and modifying selected vector features.");

    if (!parser(argc, argv, &params, &action_mode))
	exit(EXIT_FAILURE);

    /* get list of categories */
    Clist = Vect_new_cat_list();
    if (params.cat->answer && Vect_str_to_cat_list(params.cat->answer, Clist)) {
	G_fatal_error(_("Unable to get category list <%s>"),
		      params.cat->answer);
    }

    /* open input file */
    if (params.in->answer) {
	if (strcmp(params.in->answer, "-") != 0) {
	    ascii = fopen(params.in->answer, "r");
	    if (ascii == NULL)
		G_fatal_error(_("Unable to open file <%s>"),
			      params.in->answer);
	}
	else {
	    ascii = stdin;
	}
    }
    if (!ascii && action_mode == MODE_ADD)
	G_fatal_error(_("Required parameter <%s> not set"), params.in->key);
    
    if (action_mode == MODE_CREATE) {
	int overwrite, map_type;
	
	overwrite = G_check_overwrite(argc, argv);
	if (G_find_vector2(params.map->answer, G_mapset()) &&
	    (!G_find_file("", "OGR", G_mapset()) &&
	     !G_find_file("", "PG", G_mapset()))) {
	    if (!overwrite)
		G_fatal_error(_("Vector map <%s> already exists"),
			      params.map->answer);
	}

	/* 3D vector maps? */
        putenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE=1");
	ret = Vect_open_new(&Map, params.map->answer, WITHOUT_Z);
	if (ret == -1) {
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  params.map->answer);
	}
	Vect_set_error_handler_io(NULL, &Map);

	/* native or external data source ? */
	map_type = Vect_maptype(&Map);
	if (map_type != GV_FORMAT_NATIVE) {
	    int type;
	    type = Vect_option_to_types(params.type);
	    if (type != GV_POINT && !(type & GV_LINES))
		G_fatal_error("%s: point,line,boundary",
                              _("Supported feature types for non-native formats:"));
            /* create OGR or PostGIS layer */
            if (Vect_write_line(&Map, type, NULL, NULL) < 0)
                G_fatal_error(_("Unable to create vector map <%s>"),
			      params.map->answer);
	}
	
	G_debug(1, "Map created");

	if (ascii) {
	    /* also add new vector features */
	    action_mode = MODE_ADD;
	}
    }
    else {			/* open selected vector file */
	if (action_mode == MODE_ADD)	/* write */
	    ret = Vect_open_update2(&Map, params.map->answer, G_mapset(), params.fld->answer);
	else			/* read-only -- select features */
	    ret = Vect_open_old2(&Map, params.map->answer, G_mapset(), params.fld->answer);
	
	if (ret < 2)
	    G_fatal_error(_("Unable to open vector map <%s> on topological level. "
			    "Try to rebuild vector topology by v.build."),
			  params.map->answer);
    }

    G_debug(1, "Map opened");
    
    /* open backgroud maps */
    if (params.bmaps->answer) {
	i = 0;

	while (params.bmaps->answers[i]) {
	    const char *bmap = params.bmaps->answers[i];
	    const char *mapset = G_find_vector2(bmap, "");
	    if (!mapset)
		G_fatal_error(_("Vector map <%s> not found"), bmap);

	    if (strcmp(
		    G_fully_qualified_name(params.map->answer, G_mapset()),
		    G_fully_qualified_name(bmap, mapset)) == 0) {
		G_fatal_error(_("Unable to open vector map <%s> as the background map. "
			       "It is given as vector map to be edited."),
			      bmap);
	    }
	    nbgmaps++;
	    BgMap = (struct Map_info **)G_realloc(
		BgMap, nbgmaps * sizeof(struct Map_info *));
	    BgMap[nbgmaps - 1] =
		(struct Map_info *)G_malloc(sizeof(struct Map_info));
	    if (Vect_open_old(BgMap[nbgmaps - 1], bmap, "") == -1)
		G_fatal_error(_("Unable to open vector map <%s>"), bmap);
	    G_verbose_message(_("Background vector map <%s> registered"), bmap);
	    i++;
	}
    }

    layer = Vect_get_field_number(&Map, params.fld->answer);
    i = 0;
    while (params.maxdist->answers[i]) {
	switch (i) {
	case THRESH_COORDS:
	    thresh[THRESH_COORDS] =
		max_distance(atof(params.maxdist->answers[THRESH_COORDS]));
	    thresh[THRESH_SNAP] = thresh[THRESH_QUERY] =
		thresh[THRESH_COORDS];
	    break;
	case THRESH_SNAP:
	    thresh[THRESH_SNAP] =
		max_distance(atof(params.maxdist->answers[THRESH_SNAP]));
	    break;
	case THRESH_QUERY:
	    thresh[THRESH_QUERY] =
		atof(params.maxdist->answers[THRESH_QUERY]);
	    break;
	default:
	    break;
	}
	i++;
    }

    move_first = params.move_first->answer ? 1 : 0;
    snap = NO_SNAP;
    if (strcmp(params.snap->answer, "node") == 0)
	snap = SNAP;
    else if (strcmp(params.snap->answer, "vertex") == 0)
	snap = SNAPVERTEX;
    if (snap != NO_SNAP && thresh[THRESH_SNAP] <= 0) {
	G_warning(_("Threshold for snapping must be > 0. No snapping applied."));
	snap = NO_SNAP;
    }
    
    if (action_mode != MODE_CREATE && action_mode != MODE_ADD) {
	/* select lines */
	List = Vect_new_list();
	G_message(_("Selecting features..."));
	if (action_mode == MODE_COPY && BgMap && BgMap[0]) {
	    List = select_lines(BgMap[0], action_mode, &params, thresh, List);
	}
	else {
	    List = select_lines(&Map, action_mode, &params, thresh, List);
	}
    }

    if ((action_mode != MODE_CREATE && action_mode != MODE_ADD &&
	 action_mode != MODE_SELECT)) {
	if (List->n_values < 1) {
	    G_warning(_("No features selected, nothing to edit"));
	    action_mode = MODE_NONE;
	    ret = 0;
	}
	else {
	    /* reopen the map for updating */
	    if (action_mode == MODE_ZBULK && !Vect_is_3d(&Map)) {
		Vect_close(&Map);
		G_fatal_error(_("Vector map <%s> is not 3D. Tool '%s' requires 3D vector map. "
			       "Please convert the vector map "
			       "to 3D using e.g. %s."), params.map->answer,
			      params.tool->answer, "v.extrude");
	    }
	    Vect_close(&Map);

	    Vect_open_update2(&Map, params.map->answer, G_mapset(), params.fld->answer);
	}
    }

    /* coords option -> array */
    if (params.coord->answers) {
	coord = Vect_new_line_struct();
	int i = 0;
	double east, north;

	while (params.coord->answers[i]) {
	    east = atof(params.coord->answers[i]);
	    north = atof(params.coord->answers[i + 1]);
	    Vect_append_point(coord, east, north, 0.0);
	    i += 2;
	}
    }

    /* perform requested editation */
    switch (action_mode) {
    case MODE_CREATE:
	break;
    case MODE_ADD:
	if (!params.header->answer)
	    Vect_read_ascii_head(ascii, &Map);
	int num_lines;
	num_lines = Vect_get_num_lines(&Map);
	
	ret = Vect_read_ascii(ascii, &Map);
	if (ret > 0) {
	    int iline;
	    struct ilist *List_added;
	    
	    G_message(_("%d features added"), ret);
	    
	    List_added = Vect_new_list();
	    for (iline = num_lines + 1; iline <= Vect_get_num_lines(&Map); iline++)
		Vect_list_append(List_added, iline);
	    
	    G_verbose_message(_("Threshold value for snapping is %.2f"),
			      thresh[THRESH_SNAP]);
	    if (snap != NO_SNAP) { /* apply snapping */
		/* snap to vertex ? */
		Vedit_snap_lines(&Map, BgMap, nbgmaps, List_added,
				 thresh[THRESH_SNAP],
				 snap == SNAP ? FALSE : TRUE); 
	    }
	    if (params.close->answer) {	/* close boundaries */
		int nclosed;

		nclosed = close_lines(&Map, GV_BOUNDARY, thresh[THRESH_SNAP]);
		G_message(_("%d boundaries closed"), nclosed);
	    }
	    Vect_destroy_list(List_added);
	}
	break;
    case MODE_DEL:
	ret = Vedit_delete_lines(&Map, List);
	G_message(_("%d features deleted"), ret);
	break;
    case MODE_MOVE:
	move_x = atof(params.move->answers[0]);
	move_y = atof(params.move->answers[1]);
	move_z = atof(params.move->answers[2]);
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = Vedit_move_lines(&Map, BgMap, nbgmaps, List, move_x, move_y, move_z, snap, thresh[THRESH_SNAP]);
	G_message(_("%d features moved"), ret);
	break;
    case MODE_VERTEX_MOVE:
	move_x = atof(params.move->answers[0]);
	move_y = atof(params.move->answers[1]);
	move_z = atof(params.move->answers[2]);
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = Vedit_move_vertex(&Map, BgMap, nbgmaps, List, coord, thresh[THRESH_COORDS], thresh[THRESH_SNAP], move_x, move_y, move_z, move_first, snap);
	G_message(_("%d vertices moved"), ret);
	break;
    case MODE_VERTEX_ADD:
	ret = Vedit_add_vertex(&Map, List, coord, thresh[THRESH_COORDS]);
	G_message(_("%d vertices added"), ret);
	break;
    case MODE_VERTEX_DELETE:
	ret = Vedit_remove_vertex(&Map, List, coord, thresh[THRESH_COORDS]);
	G_message(_("%d vertices removed"), ret);
	break;
    case MODE_BREAK:
	if (params.coord->answer) {
	    ret = Vedit_split_lines(&Map, List,
				    coord, thresh[THRESH_COORDS], NULL);
	}
	else {
	    ret = Vect_break_lines_list(&Map, List, NULL, GV_LINES, NULL);
	}
	G_message(_("%d lines broken"), ret);
	break;
    case MODE_CONNECT:
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = Vedit_connect_lines(&Map, List, thresh[THRESH_SNAP]);
	G_message(_("%d lines connected"), ret);
	break;
    case MODE_MERGE:
	ret = Vedit_merge_lines(&Map, List);
	G_message(_("%d lines merged"), ret);
	break;
    case MODE_SELECT:
	ret = print_selected(List);
	break;
    case MODE_CATADD:
	ret = Vedit_modify_cats(&Map, List, layer, 0, Clist);
	G_message(_("%d features modified"), ret);
	break;
    case MODE_CATDEL:
	ret = Vedit_modify_cats(&Map, List, layer, 1, Clist);
	G_message(_("%d features modified"), ret);
	break;
    case MODE_COPY:
	if (BgMap && BgMap[0]) {
	    if (nbgmaps > 1)
		G_warning(_("Multiple background maps were given. "
			    "Selected features will be copied only from "
			    "vector map <%s>."),
			  Vect_get_full_name(BgMap[0]));

	    ret = Vedit_copy_lines(&Map, BgMap[0], List);
	}
	else {
	    ret = Vedit_copy_lines(&Map, NULL, List);
	}
	G_message(_("%d features copied"), ret);
	break;
    case MODE_SNAP:
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = snap_lines(&Map, List, thresh[THRESH_SNAP]);
	break;
    case MODE_FLIP:
	ret = Vedit_flip_lines(&Map, List);
	G_message(_("%d lines flipped"), ret);
	break;
    case MODE_NONE:
	break;
    case MODE_ZBULK: {
	double start, step;
	double x1, y1, x2, y2;
	
	start = atof(params.zbulk->answers[0]);
	step = atof(params.zbulk->answers[1]);
	
	x1 = atof(params.bbox->answers[0]);
	y1 = atof(params.bbox->answers[1]);
	x2 = atof(params.bbox->answers[2]);
	y2 = atof(params.bbox->answers[3]);
	
	ret = Vedit_bulk_labeling(&Map, List,
				  x1, y1, x2, y2, start, step);
	
	G_message(_("%d lines labeled"), ret);
	break;
    }
    case MODE_CHTYPE:
	ret = Vedit_chtype_lines(&Map, List);
	
	if (ret > 0) {
	    G_message(_("%d features converted"), ret);
	}
	else {
	    G_message(_("No feature modified"));
	}
	break;
    case MODE_AREA_DEL: {
	ret = 0;
	for (i = 0; i < List->n_values; i++) {
	    if (Vect_get_line_type(&Map, List->value[i]) != GV_CENTROID) {
		G_warning(_("Select feature %d is not centroid, ignoring..."),
			  List->value[i]);
		continue;
	    }
	    
	    ret += Vedit_delete_area_centroid(&Map, List->value[i]);
	}
	G_message(_("%d areas removed"), ret);
	break;
    }
    default:
	G_warning(_("Operation not implemented"));
	ret = -1;
	break;
    }
    
    Vect_hist_command(&Map);

    /* build topology only if requested or if tool!=select */
    if (!(action_mode == MODE_SELECT || params.topo->answer == 1 ||
	 !MODE_NONE)) {
	Vect_build_partial(&Map, GV_BUILD_NONE);
	Vect_build(&Map);
    }

    if (List)
	Vect_destroy_list(List);

    Vect_close(&Map);

    G_debug(1, "Map closed");

    /* close background maps */
    for (i = 0; i < nbgmaps; i++) {
	Vect_close(BgMap[i]);
	G_free((void *)BgMap[i]);
    }
    G_free((void *)BgMap);

    if (coord)
	Vect_destroy_line_struct(coord);

    if (Clist)
	Vect_destroy_cat_list(Clist);

    G_done_msg(" ");

    if (ret > -1) {
	exit(EXIT_SUCCESS);
    }
    else {
	exit(EXIT_FAILURE);
    }
}
