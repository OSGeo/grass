
/****************************************************************
 *
 * MODULE:     v.edit
 *
 * PURPOSE:    Editing vector map.
 *
 * AUTHOR(S):  GRASS Development Team
 *             Wolf Bergenheim, Jachym Cepicky, Martin Landa
 *
 * COPYRIGHT:  (C) 2006-2008 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 * TODO:       3D support
 ****************************************************************/

#include "global.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct GParams params;
    struct Map_info Map;
    struct Map_info **BgMap;	/* backgroud vector maps */
    int nbgmaps;		/* number of registrated background maps */
    char *mapset;
    enum mode action_mode;
    FILE *ascii;

    int i;
    int move_first, snap;
    int ret, print, layer;
    double move_x, move_y, thresh[3];

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
    module->keywords = _("vector, editing, geometry");
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
    if (G_strcasecmp(params.in->answer, "-") == 0 ||
	(action_mode != MODE_CREATE && params.in->answer == NULL)) {
	ascii = stdin;
    }
    else if (params.in->answer) {
	ascii = fopen(params.in->answer, "r");
	if (ascii == NULL) {
	    G_fatal_error(_("Unable to open ASCII file <%s>"),
			  params.in->answer);
	}
    }

    if (action_mode == MODE_CREATE) {
	int overwrite;

	if (G_legal_filename(params.map->answer) == -1) {
	    G_fatal_error(_("<%s> is an illegal file name"),
			  params.map->answer);
	}

	overwrite = G_check_overwrite(argc, argv);
	if (G_find_vector2(params.map->answer, G_mapset())) {
	    if (!overwrite)
		G_fatal_error(_("Vector map <%s> already exists"),
			      params.map->answer);
	}

	/* 3D vector maps? */
	ret = Vect_open_new(&Map, params.map->answer, WITHOUT_Z);
	if (ret == -1) {
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
	mapset = G_find_vector2(params.map->answer, G_mapset());
	if (mapset == NULL) {
	    G_fatal_error(_("Vector map <%s> not found in the current mapset"),
			  params.map->answer);
	}
	else if (action_mode == MODE_ADD) {	/* write */
	    ret = Vect_open_update(&Map, params.map->answer, mapset);
	}
	else {			/* read-only -- select features */
	    ret = Vect_open_old(&Map, params.map->answer, mapset);
	}

	if (ret < 2)
	    G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
			  params.map->answer, 2);
    }

    G_debug(1, "Map opened");

    /* open backgroud maps */
    if (params.bmaps->answer) {
	i = 0;
	char *bmap;

	while (params.bmaps->answers[i]) {
	    bmap = params.bmaps->answers[i];
	    mapset = G_find_vector2(bmap, "");
	    if (mapset == NULL) {
		G_fatal_error(_("Vector map <%s> not found"), bmap);
	    }

	    if (strcmp
		(G_fully_qualified_name
		 ((const char *)params.map->answer, (const char *)G_mapset()),
		 G_fully_qualified_name((const char *)bmap,
					(const char *)mapset)) == 0) {
		G_fatal_error(_("Unable to open vector map <%s> as the backround map. "
			       "It is given as vector map to be edited."),
			      bmap);
	    }
	    nbgmaps++;
	    BgMap =
		(struct Map_info **)G_realloc((void *)BgMap,
					      nbgmaps *
					      sizeof(struct Map_info *));
	    BgMap[nbgmaps - 1] =
		(struct Map_info *)G_malloc(sizeof(struct Map_info));
	    if (Vect_open_old(BgMap[nbgmaps - 1], bmap, mapset) == -1) {
		G_fatal_error(_("Unable to open vector map <%s>"),
			      G_fully_qualified_name(bmap, mapset));
	    }
	    G_verbose_message(_("Background vector map <%s> registered"),
			      G_fully_qualified_name(bmap, mapset));
	    i++;
	}
    }

    layer = atoi(params.fld->answer);
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

    if (action_mode != MODE_CREATE && action_mode != MODE_ADD) {
	/* select lines */
	List = Vect_new_list();
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

	    Vect_open_update(&Map, params.map->answer, G_mapset());
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
	print = 0;		/* do not print id's */
	break;
    case MODE_ADD:
	print = 0;
	if (!params.header->answer)
	    read_head(ascii, &Map);
	struct ilist *List_added;

	List_added = Vect_new_list();
	ret = asc_to_bin(ascii, &Map, List_added);
	G_message(_("%d features added"), ret);
	if (ret > 0) {
	    G_verbose_message(_("Threshold value for snapping is %.2f"),
			      thresh[THRESH_SNAP]);
	    if (snap != NO_SNAP) {	/* apply snapping */
		Vedit_snap_lines(&Map, BgMap, nbgmaps, List_added, thresh[THRESH_SNAP], snap == SNAP ? 0 : 1);	/* snap to vertex ? */
	    }
	    if (params.close->answer) {	/* close boundaries */
		int nclosed;

		nclosed = close_lines(&Map, GV_BOUNDARY, thresh[THRESH_SNAP]);
		G_message(_("%d boundaries closed"), nclosed);
	    }
	}
	Vect_destroy_list(List_added);
	break;
    case MODE_DEL:
	ret = Vedit_delete_lines(&Map, List);
	G_message(_("%d features deleted"), ret);
	break;
    case MODE_MOVE:
	move_x = atof(params.move->answers[0]);
	move_y = atof(params.move->answers[1]);
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = Vedit_move_lines(&Map, BgMap, nbgmaps, List, move_x, move_y, 0.0, snap, thresh[THRESH_SNAP]);	/* TODO: 3D */
	G_message(_("%d features moved"), ret);
	break;
    case MODE_VERTEX_MOVE:
	move_x = atof(params.move->answers[0]);
	move_y = atof(params.move->answers[1]);
	G_verbose_message(_("Threshold value for snapping is %.2f"),
			  thresh[THRESH_SNAP]);
	ret = Vedit_move_vertex(&Map, BgMap, nbgmaps, List, coord, thresh[THRESH_COORDS], thresh[THRESH_SNAP], move_x, move_y, 0.0,	/* TODO: 3D */
				move_first, snap);
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
	    ret = Vect_break_lines_list(&Map, List, NULL,
					GV_LINES, NULL, NULL);
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
	print = 1;
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
	print = 0;
	break;
    case MODE_ZBULK:{
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
    case MODE_CHTYPE:{
	    int npoints, nlines, ncentroids, nboundaries;

	    ret = Vedit_chtype_lines(&Map, List,
				     &npoints, &ncentroids,
				     &nlines, &nboundaries);

	    if (ret > 0) {
		if (npoints > 0) {
		    G_message(_("%d points converted to centroids"), npoints);
		}
		if (ncentroids > 0) {
		    G_message(_("%d centroids converted to points"),
			      ncentroids);
		}
		if (nlines > 0) {
		    G_message(_("%d lines converted to boundaries"), nlines);
		}
		if (nboundaries > 0) {
		    G_message(_("%d boundaries converted to lines"),
			      nboundaries);
		}
	    }
	    else {
		G_message(_("No feature modified"));
	    }
	    break;
	}
    default:
	G_warning(_("Operation not implemented"));
	ret = -1;
	break;
    }

    /*
       if (print && ret > 0) {
       for (i = 0; i < Vect_get_num_updated_lines(&Map); i++) {
       if (i > 0)
       fprintf (stdout, ",");
       fprintf (stdout, "%d", Vect_get_updated_line(&Map, i));
       }
       if (Vect_get_num_updated_lines(&Map) > 0)
       fprintf (stdout, "\n");
       fflush (stdout);
       }
     */

    Vect_hist_command(&Map);

    /* build topology only if requested or if tool!=select */
    if (!
	(action_mode == MODE_SELECT || params.topo->answer == 1 ||
	 !MODE_NONE)) {
	Vect_build_partial(&Map, GV_BUILD_NONE, NULL);
	if (G_verbose() > G_verbose_min())
	    Vect_build(&Map, stderr);
	else
	    Vect_build(&Map, NULL);
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
