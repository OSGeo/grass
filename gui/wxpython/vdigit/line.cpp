/**
   \file line.cpp

   \brief Feature manipulation (add, delete, move)

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008 by The GRASS development team

   \author Martin Landa <landa.martin gmail.com>

   \date 2008 
*/

extern "C" {
#include <grass/vedit.h>
#include <grass/dbmi.h>
}
#include "driver.h"
#include "digit.h"

/**
   \brief Add new vector feature

   \param type   feature type
   \param coords pairs of coordinates list (2D or 3D map)
   \param layer  layer number (layer < 1 -> no category)
   \param cat    category number
   \param bgmap  map of background map or NULL
   \param snap   snapping mode (see vedit.h)
   \param thresh threshold value for snapping

   \return fid on success
   \return -1 on failure
*/
int Digit::AddLine(int type, std::vector<double> coords, int layer, int cat,
		   const char *bgmap, int snap, double threshold)
{
    size_t i;
    size_t npoints;

    int newline;
    int changeset;

    struct line_pnts *Points;
    struct line_cats *Cats;

    struct Map_info **BgMap; /* backgroud vector maps */
    int nbgmaps;             /* number of registrated background maps */

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    npoints = coords.size() / (Vect_is_3d(display->mapInfo) ? 3 : 2);
    if (coords.size() != npoints * (Vect_is_3d(display->mapInfo) ? 3 : 2)) {
	wxString msg;
	msg.Printf(_("Incorrent number of points (%d)"), coords.size());
	wxMessageDialog dlg(display->parentWin, msg,
			    display->msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
	dlg.ShowModal();
	return -1;
    }

    G_debug(2, "wxDigit.AddLine(): npoints=%d, layer=%d, cat=%d, snap=%d",
	    (int) npoints, layer, cat, snap);

    /* TODO: 3D */
    if (!(type & (GV_POINTS | GV_LINES))) {
	display->Only2DMsg();
	return -1;
    }

    BgMap = NULL;
    nbgmaps = 0;
    if (bgmap && strlen(bgmap) > 0) {
	BgMap = OpenBackgroundVectorMap(bgmap);
	if (!BgMap) {
	    display->BackgroundMapMsg(bgmap);
	    return -1;
	}
	else {
	    nbgmaps = 1;
	}
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (layer > 0) {
	Vect_cat_set(Cats, layer, cat);
	
	if (cat > GetCategory(layer)) {
	    SetCategory(layer, cat); /* set up max category for layer */
	}
    }

    i = 0;
    while (i < coords.size()) {
	if (Vect_is_3d(display->mapInfo)) {
	    Vect_append_point(Points, coords[i], coords[i+1], coords[i+2]);
	    i += 3;
	}
	else {
	    Vect_append_point(Points, coords[i], coords[i+1], 0.0);
	    i += 2;
	}
    }

    if (type & GV_BOUNDARY) { /* close boundary */
	int last = Points->n_points-1;
	if (Vect_points_distance(Points->x[0], Points->x[0], Points->z[0],
				 Points->x[last], Points->x[last], Points->z[last],
				 Vect_is_3d(display->mapInfo)) <= threshold) {
	    Points->x[last] = Points->x[0];
	    Points->y[last] = Points->y[0];
	    Points->z[last] = Points->z[0];
	    G_debug(3, "wxDigit.AddLine(): boundary closed");
	}
    }

    if (snap != NO_SNAP && (type & (GV_POINT | GV_LINES))) { /* apply snapping (node or vertex) */
	Vedit_snap_line(display->mapInfo, BgMap, nbgmaps,
			-1, Points,
			threshold, (snap == SNAP) ? 0 : 1); 
    }

    newline = Vect_write_line(display->mapInfo, type, Points, Cats);
    if (newline < 0) {
	display->WriteLineMsg();
	return -1;
    }

    /* register changeset */
    changeset = changesets.size();
    AddActionToChangeset(changeset, ADD, newline);

    /* break at intersection */
    if (settings.breakLines) {
	BreakLineAtIntersection(newline, Points, changeset);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    if (BgMap && BgMap[0]) {
	Vect_close(BgMap[0]);
    }

    return newline;
}

/**
   \brief Rewrite given line

   \param line line id
   \param coords line geometry
   \param bgmap  map of background map or NULL
   \param snap   snapping mode (see vedit.h)
   \param thresh threshold value for snapping

   \return new line id
   \return -1 error
*/
int Digit::RewriteLine(int line, std::vector<double> coords,
		       const char *bgmap, int snap, double threshold)
{
    int newline, type, dim;
    int nlines, changeset;

    struct line_pnts *points;
    struct line_cats *cats;

    struct Map_info **BgMap; /* backgroud vector maps */
    int nbgmaps;             /* number of registrated background maps */

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    /* line alive ? */
    if (!Vect_line_alive(display->mapInfo, line)) {
	display->WriteLineMsg();
	return -1;
    }

    BgMap = NULL;
    nbgmaps = 0;
    if (bgmap && strlen(bgmap) > 0) {
	BgMap = OpenBackgroundVectorMap(bgmap);
	if (!BgMap) {
	    display->BackgroundMapMsg(bgmap);
	    return -1;
	}
	else {
	    nbgmaps = 1;
	}
    }
    
    cats = Vect_new_cats_struct();

    /* read line */
    type = Vect_read_line(display->mapInfo, NULL, cats, line);
    if (type < 0) {
	Vect_destroy_cats_struct(cats);
	if (BgMap && BgMap[0]) {
	    Vect_close(BgMap[0]);
	}
	
	display->ReadLineMsg(line);
	
	return -1;
    }

    points = Vect_new_line_struct();

    /* define line geometry */
    if (Vect_is_3d(display->mapInfo)) {
	dim = 3;
    }
    else {
	dim = 2;
    }
    for(size_t i = dim - 1; i < coords.size(); i += dim) {
	if (dim == 2) {
	    Vect_append_point(points, coords[i-1], coords[i], 0.0);
	}
	else {
	    Vect_append_point(points, coords[i-2], coords[i-1], coords[i]);
	}
    }

    if (snap != NO_SNAP) { /* apply snapping (node or vertex) */
	Vedit_snap_line(display->mapInfo, BgMap, nbgmaps,
			-1, points,
			threshold, (snap == SNAP) ? 0 : 1); 
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = changesets.size();
    AddActionToChangeset(changeset, DEL, line);

    /* rewrite line */
    newline = Vect_rewrite_line(display->mapInfo, line, type, points, cats);

    AddActionToChangeset(changeset, ADD, newline);

    if (newline > 0 && settings.breakLines) {
	BreakLineAtIntersection(newline, points, changeset);
    }

    if (newline < 0)
	display->WriteLineMsg();
    
    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(cats);
    
    if (BgMap && BgMap[0]) {
	Vect_close(BgMap[0]);
    }

    return newline;
}

/**
   \brief Split/break selected line

   Shape of line is not changed.

   \param x,y,z coordinates (z is used only if map is 3d)
   \param thresh threshold value to find a point on line

   \return number of modified lines
   \return -1 on error
*/
int Digit::SplitLine(double x, double y, double z,
		     double thresh)
{
    int ret;
    int nlines, changeset;

    struct line_pnts *point;
    struct ilist *list;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    point = Vect_new_line_struct();
    list  = Vect_new_list();

    Vect_append_point(point, x, y, z);

    nlines = Vect_get_num_lines(display->mapInfo);

    changeset = AddActionsBefore();
    
    ret = Vedit_split_lines(display->mapInfo, display->selected.values,
			    point, thresh, list);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    Vect_destroy_list(list);
    Vect_destroy_line_struct(point);

    return ret;
}

/**
   \brief Delete selected vector features

   \param delete_records delete also attribute records

   \return number of deleted lines
   \return -1 on error
*/
int Digit::DeleteLines(bool delete_records)
{
    int ret;
    int n_dblinks;
    int changeset;

    struct line_cats *Cats, *Cats_del;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    n_dblinks = Vect_get_num_dblinks(display->mapInfo);
    Cats_del = NULL;
    
    /* collect categories if needed */
    if (delete_records) {
	Cats = Vect_new_cats_struct();
	Cats_del = Vect_new_cats_struct();
	for (int i = 0; i < display->selected.values->n_values; i++) {
	    if (Vect_read_line(display->mapInfo, NULL, Cats, display->selected.values->value[i]) < 0) {
		Vect_destroy_cats_struct(Cats_del);
		display->ReadLineMsg(display->selected.values->value[i]);
		return -1;
	    }
	    for (int j = 0; j < Cats->n_cats; j++) {
		/*
		  To find other vector objects with the same category,
		  category index is need to be updated (i.e. to
		  rebuild topo, sidx, cidx). This can be time-consuming
		  task for large vector maps.
		*/
		/*
		  Vect_build(display->mapInfo, NULL);
		  Vect_cidx_find_all(display->mapInfo, Cats->field[j],
		  GV_POINTS | GV_LINES, Cats->cat[j],
		  List);
		if (List->n_values == 1 &&
		    List->value[0] == display->selected->value[i]) {
		    Vect_cat_set(Cats_del, Cats->field[j], Cats->cat[j]);
		}
		*/
		Vect_cat_set(Cats_del, Cats->field[j], Cats->cat[j]);
	    }
	}
	Vect_destroy_cats_struct(Cats);
    }

    /* register changeset */
    changeset = AddActionsBefore();

    ret = Vedit_delete_lines(display->mapInfo, display->selected.values);
    
    if (ret > 0 && delete_records) {
	struct field_info *fi;
	char buf[GSQL_MAX];
	dbDriver *driver;
	dbHandle handle;
	dbString stmt;

	for (int dblink = 0; dblink < n_dblinks; dblink++) {
	    fi = Vect_get_dblink(display->mapInfo, dblink);
	    if (fi == NULL) {
		display->DblinkMsg(dblink+1);
		return -1;
	    }

	    driver = db_start_driver(fi->driver);
	    if (driver == NULL) {
		display->DbDriverMsg(fi->driver);
		return -1;
	    }

	    db_init_handle (&handle);
	    db_set_handle (&handle, fi->database, NULL);
	    if (db_open_database(driver, &handle) != DB_OK) {
		display->DbDatabaseMsg(fi->driver, fi->database);
		return -1;
	    }

	    db_init_string (&stmt);
	    sprintf (buf, "DELETE FROM %s WHERE", fi->table);
	    db_set_string(&stmt, buf);
	    int n_cats = 0;
	    for (int c = 0; c < Cats_del->n_cats; c++) {
		if (Cats_del->field[c] == fi->number) {
		    if (n_cats > 0) {
			sprintf (buf, " or");
			db_append_string(&stmt, buf);
		    }
		    sprintf (buf, " %s = %d", fi->key, Cats_del->cat[c]);
		    db_append_string(&stmt, buf);
		    n_cats++;
		}
	    }

	    Vect_cat_del(Cats_del, fi->number);

	    if (n_cats &&
		db_execute_immediate (driver, &stmt) != DB_OK ) {
		display->DbExecuteMsg(db_get_string(&stmt));
		return -1;
	    }
	    
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	}
    }

    /* update category settings */
    // InitCats();

    if (Cats_del) {
	Vect_destroy_cats_struct(Cats_del);
    }
    
    return ret;
}


/** 
    \brief Move selected vector features

    \param move_x,move_y,move_z move direction (move_z is used only if map is 3D)
    \param bgmap  map of background map or NULL
    \param snap snapping move (see vedit.h)
    \param thresh threshold value for snapping

    \return number of moved features
    \return -1 on error
*/
int Digit::MoveLines(double move_x, double move_y, double move_z,
		     const char *bgmap, int snap, double thresh)
{
    int ret;
    int nlines, changeset;
    struct Map_info **BgMap; /* backgroud vector maps */
    int nbgmaps;             /* number of registrated background maps */

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    BgMap = NULL;
    nbgmaps = 0;
    if (bgmap && strlen(bgmap) > 0) {
	BgMap = OpenBackgroundVectorMap(bgmap);
	if (!BgMap) {
	    display->BackgroundMapMsg(bgmap);
	    return -1;
	}
	else {
	    nbgmaps = 1;
	}
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = AddActionsBefore();
    
    ret = Vedit_move_lines(display->mapInfo, BgMap, nbgmaps,
			   display->selected.values,
			   move_x, move_y, move_z,
			   snap, thresh);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    if (ret > 0 && settings.breakLines) {
	for(int i = 1; i <= ret; i++) {
	    BreakLineAtIntersection(nlines + i, NULL, changeset);
	}
    }
    
    if (BgMap && BgMap[0]) {
	Vect_close(BgMap[0]);
    }

    return ret;
}

/**
   \brief Flip selected lines/boundaries

   \return number of modified lines
   \return -1 on error
*/
int Digit::FlipLines()
{
    int ret;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }


    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = AddActionsBefore();
    
    ret = Vedit_flip_lines(display->mapInfo, display->selected.values);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    return ret;
}

/**
   \brief Merge selected lines/boundaries

   \return number of modified lines
   \return -1 on error
*/
int Digit::MergeLines()
{
    int ret;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);
    
    changeset = AddActionsBefore();

    ret = Vedit_merge_lines(display->mapInfo, display->selected.values);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }

    return ret;
}

/**
   \brief Breaks selected lines/boundaries

   \todo undo

   \return number of modified lines
   \return -1 on error
*/
int Digit::BreakLines()
{
    int ret;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);
    
    changeset = AddActionsBefore();
    
    ret = Vect_break_lines_list(display->mapInfo, display->selected.values, NULL,
				GV_LINES, NULL);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }

    return ret;
}

/**
   \brief Snap selected lines/boundaries

   \todo undo

   \param thresh threshold value for snapping

   \return 0 on success 
   \return -1 on error
*/
int Digit::SnapLines(double thresh)
{
    int changeset, nlines;
    
    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);
    
    changeset = AddActionsBefore();
    
    Vect_snap_lines_list (display->mapInfo, display->selected.values,
			  thresh, NULL);

    if (nlines < Vect_get_num_lines(display->mapInfo)) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }

    return 0;
}

/**
   \brief Connect selected lines/boundaries

   \return number of modified lines
   \return -1 on error
*/
int Digit::ConnectLines(double thresh)
{
    int ret;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = AddActionsBefore();

    ret = Vedit_connect_lines(display->mapInfo, display->selected.values,
			      thresh);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    return ret;
}

/**
   \brief Automated labeling (z coordinate assignment) of vector lines (contours).

   Modified vector map must be 3D.

   \todo Undo

   \param x1,y1,x2,y2 line nodes for intersection
   \param start starting value
   \param step  step value for labeling

   \return number of modified lines
   \return -1 on error
*/
int Digit::ZBulkLabeling(double x1, double y1, double x2, double y2,
			 double start, double step)
{
    int ret;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = AddActionsBefore();

    ret = Vedit_bulk_labeling (display->mapInfo, display->selected.values,
			       x1, y1, x2, y2, start, step);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }

    return ret;
}

/**
   \brief Copy vector features

   \param ids line ids to be copied (if not given selected are used)
   \param bgmap name of background map (if not given, copy features from input)

   \return number of copied features
   \return -1 on error
*/
int Digit::CopyLines(std::vector<int> ids, const char* bgmap_name)
{
    int ret;
    int changeset, nlines;
    struct Map_info *bgMap;
    struct ilist *list;

    bgMap = NULL;
    list = NULL;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    if (bgmap_name) {
	const char *mapset;
	bgMap = (struct Map_info *) G_malloc(sizeof (struct Map_info));
	mapset = G_find_vector2 (bgmap_name, ""); 
	Vect_open_old(bgMap, (char *) bgmap_name, (char *) mapset); /* TODO */
    }

    if (!ids.empty()) {
	list = Vect_new_list();
	for (std::vector<int>::const_iterator b = ids.begin(), e = ids.end();
	     b != e; ++b) {
	    Vect_list_append(list, *b);
	}
    }
    else {
	list = display->selected.values;
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    ret = Vedit_copy_lines (display->mapInfo, bgMap,
			    list);

    if (ret > 0) {
	changeset = changesets.size();
	for (int line = nlines + 1; line <= Vect_get_num_lines(display->mapInfo); line++) {
	    AddActionToChangeset(changeset, ADD, line);
	}
    }
    else {
	changesets.erase(changeset);
    }

    if (ret > 0 && bgMap && settings.breakLines) {
	for(int i = 1; i <= ret; i++)
	    BreakLineAtIntersection(nlines + i, NULL, changeset);
    }

    if (list != display->selected.values) {
	Vect_destroy_list(list);
    }

    if (bgMap) {
	Vect_close(bgMap);
	G_free ((void *) bgMap);
    }

    return ret;
}

/**
   \brief Open background vector map

   @todo support more background maps then only one

   \param bgmap pointer to vector map name

   \return vector map array
   \return NULL on error
*/
struct Map_info** Digit::OpenBackgroundVectorMap(const char *bgmap)
{
    char name[GNAME_MAX];
    char mapset[GMAPSET_MAX];
    int nbgmaps;
    struct Map_info** BgMap;

    if (!display->mapInfo) {
	return NULL;
    }

    if (G_find_vector2 (bgmap, "") == NULL) {
	return NULL;
    }
    
    nbgmaps = 0;
    BgMap = NULL;

    if (!G__name_is_fully_qualified(bgmap, name, mapset)) {
	strncpy(name, bgmap, GNAME_MAX);
	mapset[0] = '\0';
    }
    if (strcmp(G_fully_qualified_name((const char*) display->mapInfo->name, (const char*) G_mapset()),
	       G_fully_qualified_name((const char*) bgmap, (const char*) mapset))) {
	nbgmaps = 1;
	BgMap = (struct Map_info**) G_malloc (nbgmaps * sizeof(struct Map_info*));
	BgMap[nbgmaps-1] = (struct Map_info *) G_malloc (sizeof(struct Map_info));
	
	// avoid GUI crash
	Vect_set_fatal_error(GV_FATAL_PRINT);
	
	if (Vect_open_old(BgMap[nbgmaps-1], name, mapset) == -1) {
	    G_free ((void *) BgMap[nbgmaps-1]);
	    BgMap = NULL;
	}
    }
    
    return BgMap;
}

/**
   \brief Type conversion of selected features

   Supported conversions:
    - point <-> centroid
    - line <-> boundary

   \return number of modified features
   \return -1 on error
*/
int Digit::TypeConvLines()
{
    int ret;
    int npoints, ncentroids, nboundaries;
    int changeset, nlines;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    nlines = Vect_get_num_lines(display->mapInfo);

    /* register changeset */
    changeset = AddActionsBefore();
    
    ret = Vedit_chtype_lines (display->mapInfo, display->selected.values,
			       &npoints, &ncentroids,
			       &nlines, &nboundaries);

    if(ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }

    return ret;
}

/*!
  \brief Break given line at intersection

  \param line line id
  
  \return number of modified lines
*/
int Digit::BreakLineAtIntersection(int line, struct line_pnts* points_line,
				   int changeset)
{
    int ret, type, nlines;
    int lineBreak;
    BOUND_BOX lineBox;
    struct ilist *list, *listBreak, *listRef;
    struct line_pnts *points_check, *points;

    if (!Vect_line_alive(display->mapInfo, line))
	return 0;
    
    if (!points_line) {
	points = Vect_new_line_struct();
	if (Vect_read_line(display->mapInfo, points, NULL, line) < 0) {
	    display->ReadLineMsg(line);
	    return -1;
	}
    }
    else {
	points = points_line;
    }

    list = Vect_new_list();
    listRef = Vect_new_list();
    listBreak = Vect_new_list();
    
    points_check = Vect_new_line_struct();
    
    /* find all relevant lines */
    Vect_get_line_box(display->mapInfo, line, &lineBox);
    Vect_select_lines_by_box(display->mapInfo, &lineBox,
			     GV_LINES, list);
    
    /* check for intersection */
    Vect_list_append(listBreak, line);
    Vect_list_append(listRef, line);
    for (int i = 0; i < list->n_values; i++) {
	lineBreak = list->value[i];
	if (lineBreak == line)
		continue;
	
	type = Vect_read_line(display->mapInfo, points_check, NULL, lineBreak);
	if (!(type & GV_LINES))
	    continue;
	
	if (Vect_line_check_intersection(points, points_check,
					 WITHOUT_Z))
	    Vect_list_append(listBreak, lineBreak);
    }
    
    nlines = Vect_get_num_lines(display->mapInfo);

    for (int i = 0; i < listBreak->n_values; i++) {	
	AddActionToChangeset(changeset, DEL, listBreak->value[i]);
    }

    ret = Vect_break_lines_list(display->mapInfo, listBreak, listRef,
				GV_LINES, NULL);

    for (int i = 0; i < listBreak->n_values; i++) {	
	if (Vect_line_alive(display->mapInfo, listBreak->value[i]))
	    RemoveActionFromChangeset(changeset, DEL, listBreak->value[i]);
    }

    for (int line = nlines + 1; line <= Vect_get_num_lines(display->mapInfo); line++) {
	AddActionToChangeset(changeset, ADD, line);
    }
    
    Vect_destroy_line_struct(points_check);

    if (points != points_line)
	Vect_destroy_line_struct(points);

    Vect_destroy_list(list);
    Vect_destroy_list(listBreak);
    Vect_destroy_list(listRef);

    return ret;
}
