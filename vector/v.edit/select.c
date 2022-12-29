
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

#include <grass/dbmi.h>
#include "global.h"

static char first_selection = 1;
static int merge_lists(struct ilist *, struct ilist *);
static int merge_lists2(struct ilist *, struct boxlist *);

/**
   \brief Select vector features

   \param[in] Map vector map
   \param[in] action_mode tool
   \param[in] params GRASS parameters
   \param[in] List list of selected features
   
   \return list of newly selected features
*/
struct ilist *select_lines(struct Map_info *Map, enum mode action_mode,
			   struct GParams *params, double *thresh,
			   struct ilist *List)
{
    int layer, type;

    layer = Vect_get_field_number(Map, params->fld->answer);
    type = Vect_option_to_types(params->type);

    /* select by id's */
    if (params->id->answer != NULL) {
	sel_by_id(Map, type, params->id->answer, List);
    }

    /* select by category (ignore tools catdel and catadd) */
    if ((action_mode != MODE_CATADD && action_mode != MODE_CATDEL) &&
	params->cat->answer != NULL) {
	sel_by_cat(Map, NULL, layer, type, params->cat->answer, List);
    }

    /* select by coordinates (+threshold) */
    if (params->coord->answer != NULL) {
	int i;
	double east, north;
	struct line_pnts *coords;

	coords = Vect_new_line_struct();
	i = 0;
	while (params->coord->answers[i]) {
	    east = atof(params->coord->answers[i]);
	    north = atof(params->coord->answers[i + 1]);
	    Vect_append_point(coords, east, north, 0.0);
	    i += 2;
	}

	G_verbose_message(_("Threshold value for coordinates is %.2f"),
			  thresh[THRESH_COORDS]);
	sel_by_coordinates(Map, type, coords, thresh[THRESH_COORDS], List);

	Vect_destroy_line_struct(coords);
    }

    /* select by bbox */
    if (params->bbox->answer != NULL) {
	struct line_pnts *bbox;
	double x1, y1, x2, y2;

	bbox = Vect_new_line_struct();

	x1 = atof(params->bbox->answers[0]);
	y1 = atof(params->bbox->answers[1]);
	x2 = atof(params->bbox->answers[2]);
	y2 = atof(params->bbox->answers[3]);

	Vect_append_point(bbox, x1, y1, -PORT_DOUBLE_MAX);
	Vect_append_point(bbox, x2, y1, PORT_DOUBLE_MAX);
	Vect_append_point(bbox, x2, y2, -PORT_DOUBLE_MAX);
	Vect_append_point(bbox, x1, y2, PORT_DOUBLE_MAX);
	Vect_append_point(bbox, x1, y1, -PORT_DOUBLE_MAX);

	/* sel_by_bbox not used */
	/*
	   sel_by_bbox(Map,
	   type, x1, y1, x2, y2,
	   List);
	 */
	sel_by_polygon(Map, type, bbox, List);

	Vect_destroy_line_struct(bbox);
    }

    /* select by polygon  */
    if (params->poly->answer != NULL) {
	int i;
	struct line_pnts *Polygon;

	Polygon = Vect_new_line_struct();

	for (i = 0; params->poly->answers[i]; i += 2) {
	    Vect_append_point(Polygon,
			      atof(params->poly->answers[i]),
			      atof(params->poly->answers[i + 1]), 0.0);
	}

	/* if first and last point of polygon does not match */
	if (atof(params->poly->answers[i - 1]) !=
	    atof(params->poly->answers[0])) {
	    Vect_append_point(Polygon, atof(params->poly->answers[0]),
			      atof(params->poly->answers[1]), 0.0);
	}

	sel_by_polygon(Map, type, Polygon, List);

	Vect_destroy_line_struct(Polygon);
    }

    /* select by where statement */
    if (params->where->answer != NULL) {
	sel_by_where(Map, layer, type, params->where->answer, List);
    }

    /* selecy by query */
    if (params->query->answer != NULL) {
	int query_type;
	struct ilist *List_tmp;

	if (first_selection) {
	    List_tmp = List;
	    first_selection = 0;
	}
	else {
	    List_tmp = Vect_new_list();
	}

	query_type = QUERY_UNKNOWN;
	if (strcmp(params->query->answer, "length") == 0) {
	    query_type = QUERY_LENGTH;
	}
	else if (strcmp(params->query->answer, "dangle") == 0) {
	    query_type = QUERY_DANGLE;
	}

	G_verbose_message(_("Threshold value for querying is %.2f"),
			  thresh[THRESH_QUERY]);
	Vedit_select_by_query(Map, type, layer, thresh[THRESH_QUERY],
			      query_type, List_tmp);

	/* merge lists (only duplicate items) */
	if (List_tmp != List) {
	    merge_lists(List, List_tmp);
	    Vect_destroy_list(List_tmp);
	}
    }

    if (params->reverse->answer) {
	reverse_selection(Map, type, &List);
    }

    G_message(n_("%d of %d feature selected from vector map <%s>",
                 "%d of %d features selected from vector map <%s>",
                 Vect_get_num_lines(Map)
    ),
	      List->n_values,
	      Vect_get_num_lines(Map), Vect_get_full_name(Map));

    return List;
}

/**
   \brief Print selected vector features
 
   \param[in] List list of selected features

   \return number of selected features
   \return -1 on error
*/
int print_selected(struct ilist *List)
{
    int i;

    /* print the result */
    for (i = 0; i < List->n_values; i++) {
	fprintf(stdout, "%d%s",
		List->value[i], i < List->n_values - 1 ? "," : "");
    }
    if (List->n_values > 0) {
	fprintf(stdout, "\n");
    }
    fflush(stdout);

    return List->n_values;
}

/**
   \brief Select features by category
 
   \param[in] Map vector map
   \param[in] cl_orig original list of categories (previously selected)
   \param[in] layer layer number
   \param[in] type feature type
   \param[in] cat category string
   \param[in,out] List list of selected features

   \return number of selected lines
*/
int sel_by_cat(struct Map_info *Map, struct cat_list *cl_orig,
	       int layer, int type, char *cats, struct ilist *List)
{
    struct ilist *List_tmp, *List_tmp1;
    struct cat_list *cl;

    int i, cat;

    if (first_selection || cl_orig) {
	List_tmp = List;
	first_selection = 0;
    }
    else {
	List_tmp = Vect_new_list();
    }

    List_tmp1 = Vect_new_list();

    if (cl_orig == NULL) {
	cl = Vect_new_cat_list();

	Vect_str_to_cat_list(cats, cl);
    }
    else {
	cl = cl_orig;
    }

    for (i = 0; i < cl->n_ranges; i++) {
	for (cat = cl->min[i]; cat <= cl->max[i]; cat++) {
	    Vect_cidx_find_all(Map, layer, type, cat, List_tmp1);
	    Vect_list_append_list(List_tmp, List_tmp1);
	}
    }

    G_debug(1, "  %d lines selected (by category)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    if (List_tmp != List) {
	merge_lists(List, List_tmp);
	Vect_destroy_list(List_tmp);
    }

    Vect_destroy_list(List_tmp1);

    return List->n_values;
}

/**
   \brief Select features by coordinates
 
   \param[in] Map vector map
   \param[in] type feature type
   \param[in] coords coordinates GRASS parameters
   \param[in] thresh threshold value for searching
   \param[in,out] List list of selected features

   \return number of selected lines
*/
int sel_by_coordinates(struct Map_info *Map,
		       int type, struct line_pnts *coords, double thresh,
		       struct ilist *List)
{
    int i;
    double east, north, maxdist;

    struct ilist *List_tmp, *List_in_box;
    struct line_pnts *box;

    if (first_selection) {
	List_tmp = List;
	first_selection = 0;
    }
    else {
	List_tmp = Vect_new_list();
    }

    box = Vect_new_line_struct();
    List_in_box = Vect_new_list();

    if (thresh < 0)
	maxdist = max_distance(thresh);
    else
	maxdist = thresh;

    for (i = 0; i < coords->n_points; i++) {
	east = coords->x[i];
	north = coords->y[i];

	coord2bbox(east, north, maxdist, box);

	Vect_select_lines_by_polygon(Map, box, 0, NULL, type, List_in_box);

	if (List_in_box->n_values > 0)
	    Vect_list_append_list(List_tmp, List_in_box);
    }

    G_debug(1, "  %d lines selected (by coordinates)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    if (List_tmp != List) {
	merge_lists(List, List_tmp);
	Vect_destroy_list(List_tmp);
    }

    Vect_destroy_line_struct(box);
    Vect_destroy_list(List_in_box);

    return List->n_values;
}

/**
   \brief Select features by bbox
   
   \param[in] Map vector map
   \param[in] type feature type
   \param[in] bbox_opt bounding boxes
   \param[in,out] List list of selected features

   \return number of selected lines
*/
int sel_by_bbox(struct Map_info *Map,
		int type, double x1, double y1, double x2, double y2,
		struct ilist *List)
{
    struct bound_box bbox;

    struct boxlist *List_tmp;
    
    List_tmp = Vect_new_boxlist(0);
    
    /* bounding box */
    bbox.N = y1 < y2 ? y2 : y1;
    bbox.S = y1 < y2 ? y1 : y2;
    bbox.W = x1 < x2 ? x1 : x2;
    bbox.E = x1 < x2 ? x2 : x1;
    bbox.T = PORT_DOUBLE_MAX;
    bbox.B = -PORT_DOUBLE_MAX;
    
    Vect_select_lines_by_box(Map, &bbox, type, List_tmp);

    G_debug(1, "  %d lines selected (by bbox)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    merge_lists2(List, List_tmp);
    Vect_destroy_boxlist(List_tmp);
    
    return List->n_values;
}

/**
   \brief Select features by polygon

   \param[in] Map vector map
   \param[in] type feature type
   \param[in] poly polygon coordinates
   \param[in,out] List list of selected features
   
   \return number of selected lines
*/
int sel_by_polygon(struct Map_info *Map,
		   int type, struct line_pnts *Polygon, struct ilist *List)
{
    struct ilist *List_tmp;

    if (first_selection) {
	List_tmp = List;
	first_selection = 0;
    }
    else {
	List_tmp = Vect_new_list();
    }

    /* no isles */
    Vect_select_lines_by_polygon(Map, Polygon, 0, NULL, type, List_tmp);

    G_debug(1, "  %d lines selected (by polygon)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    if (List_tmp != List) {
	merge_lists(List, List_tmp);
	Vect_destroy_list(List_tmp);
    }

    return List->n_values;
}

/**
   \brief Select features by id

   \param[in] Map vector map
   \param[in] type feature type
   \param[in] ids ids list
   \param[in,out] List list of selected features
 
   \return number of selected lines
*/
int sel_by_id(struct Map_info *Map, int type, char *ids, struct ilist *List)
{
    int i;
    int num, id;
    struct cat_list *il;	/* NOTE: this is not cat list, but list of id's */
    struct ilist *List_tmp;

    if (first_selection) {
	List_tmp = List;
	first_selection = 0;
    }
    else {
	List_tmp = Vect_new_list();
    }

    il = Vect_new_cat_list();
    Vect_str_to_cat_list(ids, il);

    num = Vect_get_num_lines(Map);

    for (i = 0; i < il->n_ranges; i++) {
	for (id = 1; id <= num; id++) {
	    if (!(Vect_read_line(Map, NULL, NULL, id) & type)) {
		continue;
	    }
	    if (id >= il->min[i] && id <= il->max[i]) {
		Vect_list_append(List_tmp, id);
	    }
	}
    }

    G_debug(1, "  %d lines selected (by id)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    if (List_tmp != List) {
	merge_lists(List, List_tmp);
	Vect_destroy_list(List_tmp);
    }

    Vect_destroy_cat_list(il);

    return List->n_values;
}

/**
   \brief Select features according to SQL where statement

   \param[in] Map vector map
   \param[in] layer layer number
   \param[in] type feature type
   \param[in] where 'where' statement
   \param[in,out] List list of selected features
 
   \return number of selected lines
*/
int sel_by_where(struct Map_info *Map,
		 int layer, int type, char *where, struct ilist *List)
{
    struct cat_list *cat_list;
    struct ilist *List_tmp;
    struct field_info *Fi;
    dbDriver *driver;
    dbHandle handle;

    int *cats, ncats;

    if (first_selection) {
	List_tmp = List;
	first_selection = 0;
    }
    else {
	List_tmp = Vect_new_list();
    }

    cat_list = Vect_new_cat_list();

    if (layer < 1) {
	G_fatal_error(_("Layer must be > 0 for 'where'"));
    }

    Fi = Vect_get_field(Map, layer);

    if (!Fi) {
	G_fatal_error(_("Database connection not defined for layer %d"),
		      layer);
    }

    driver = db_start_driver(Fi->driver);

    if (!driver)
	G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);

    db_init_handle(&handle);

    db_set_handle(&handle, Fi->database, NULL);

    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    db_set_error_handler_driver(driver);

    ncats = db_select_int(driver, Fi->table, Fi->key, where, &cats);

    db_close_database(driver);
    db_shutdown_driver(driver);

    Vect_array_to_cat_list(cats, ncats, cat_list);

    /* free array of cats */
    if (ncats >= 0)
	G_free(cats);

    sel_by_cat(Map, cat_list, layer, type, NULL, List_tmp);

    G_debug(1, "  %d lines selected (by where)", List_tmp->n_values);

    /* merge lists (only duplicate items) */
    if (List_tmp != List) {
	merge_lists(List, List_tmp);
	Vect_destroy_list(List_tmp);
    }

    Vect_destroy_cat_list(cat_list);

    return List->n_values;
}

/**
   \brief merge two lists, i.e. store only duplicate items

   \param[in] alist,blist list to be merged

   \return result number of items
*/
static int merge_lists(struct ilist *alist, struct ilist *blist)
{
    int i;

    struct ilist *list_del;

    list_del = Vect_new_list();

    for (i = 0; i < alist->n_values; i++) {
	if (!Vect_val_in_list(blist, alist->value[i]))
	    Vect_list_append(list_del, alist->value[i]);
    }

    Vect_list_delete_list(alist, list_del);

    Vect_destroy_list(list_del);

    return alist->n_values;
}

/**
   \brief merge two lists, i.e. store only duplicate items

   \param[in] alist,blist list to be merged

   \return result number of items
*/
static int merge_lists2(struct ilist *alist, struct boxlist *blist)
{
    int i;

    struct ilist *list_del;

    list_del = Vect_new_list();

    for (i = 0; i < alist->n_values; i++) {
	if (!Vect_val_in_boxlist(blist, alist->value[i]))
	    Vect_list_append(list_del, alist->value[i]);
    }

    Vect_list_delete_list(alist, list_del);

    Vect_destroy_list(list_del);

    return alist->n_values;
}

/**
   \brief Reverse list selection
   
   \param[in] Map vector map
   \param[in] type feature type
   \param[in,out] reversed list

   \return 1
*/
int reverse_selection(struct Map_info *Map, int type, struct ilist **List)
{

    struct ilist *list_reverse;
    int line, nlines, ltype;

    list_reverse = Vect_new_list();

    nlines = Vect_get_num_lines(Map);

    for (line = 1; line <= nlines; line++) {
	ltype = Vect_read_line(Map, NULL, NULL, line);

	if (!(ltype & type))
	    continue;

	if (!Vect_val_in_list(*List, line))
	    Vect_list_append(list_reverse, line);
    }

    Vect_destroy_list(*List);
    *List = list_reverse;

    return 1;
}
