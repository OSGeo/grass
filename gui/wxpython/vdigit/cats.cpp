/**
   \file cats.cpp

   \brief Category management

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008 by The GRASS development team

   \author Martin Landa <landa.martin gmail.com>

   \date 2008 
*/

#include "driver.h"
#include "digit.h"

/**
   \brief Initialize cats structure.

   \return 0 on success
   \return -1 on error
*/
int Digit::InitCats()
{
    int ndblinks, nfields, field, ncats;
    int cat, type, id; 

    struct field_info *fi;

    if (!cats.empty()) {	
	cats.clear();
    }

    if (!display->mapInfo) {
	DisplayMsg();
	return -1;
    }

    /* initialization */
    ndblinks = Vect_get_num_dblinks(display->mapInfo);
    for (int i = 0; i < ndblinks; i++) {
	fi = Vect_get_dblink(display->mapInfo, i);
	if (fi) {
	    cats[fi->number] = PORT_INT_MIN;
	}
    }

    /* find max category */
    nfields = Vect_cidx_get_num_fields (display->mapInfo);
    G_debug(2, "wxDigit.InitCats(): nfields=%d", nfields);    

    for (int i = 0; i < nfields; i++ ) {
	field = Vect_cidx_get_field_number(display->mapInfo, i);
	ncats = Vect_cidx_get_num_cats_by_index(display->mapInfo, i);
	if (field <= 0) {
	    continue;
	}
	for (int j = 0; j < ncats; j++) {
	    Vect_cidx_get_cat_by_index (display->mapInfo, i, j, &cat, &type, &id);
	    if (cat > cats[field])
		cats[field] = cat;
	}

	G_debug(3, "wxDigit.InitCats(): layer=%d, cat=%d", field, cats[field]);
    }

    /* set default values */
    for(std::map<int, int>::const_iterator b = cats.begin(), e = cats.end();
	b != e; ++b ) {
	if (b->second == PORT_INT_MIN) {
	    cats[b->first] = 0;
	    G_debug(3, "wxDigit.InitCats(): layer=%d, cat=%d", b->first, cats[b->first]);
	}

    }

    return 0;
}

/**
   \brief Get max category number for layer

   \param layer layer number

   \return category number (1 if no category found)
   \return -1 on error
*/
int Digit::GetCategory(int layer)
{
    if (cats.find(layer) != cats.end()) {
	G_debug(3, "v.digit.GetCategory(): layer=%d, cat=%d", layer, cats[layer]);
	return cats[layer];
    }

    return -1;
}

/**
   \brief Set max category number for layer

   \param layer layer number
   \param cats  category number to be set

   \return previosly set category
   \return -1 if layer not available
*/
int Digit::SetCategory(int layer, int cat)
{
    int old_cat;

    if (cats.find(layer) != cats.end()) {
	old_cat = cats[layer];
    }
    else {
	old_cat = -1;
    }

    cats[layer] = cat;
    G_debug(3, "wxDigit.SetCategory(): layer=%d, cat=%d old_cat=%d",
	    layer, cat, old_cat);

    return old_cat;
}

/**
   Get list of layers

   Requires InitCats() to be called.

   \return list of layers
*/
std::vector<int> Digit::GetLayers()
{
    std::vector<int> layers;

    for(std::map<int, int>::const_iterator b = cats.begin(), e = cats.end();
	b != e; ++b ) {
	layers.push_back(b->first);
    }

    return layers;
}

/**
   \brief Get list of layer/category(ies) for selected feature.

   \param line feature id (-1 for first selected feature)

   \return list of layer/cats
*/
std::map<int, std::vector<int> > Digit::GetLineCats(int line_id)
{
    std::map<int, std::vector<int> > lc;
    int line, n_dblinks;
    struct line_cats *Cats;
    struct field_info *fi;

    if (!display->mapInfo) {
	DisplayMsg();
	return lc;
    }

    if (line_id == -1 && display->selected.values->n_values < 1) {
	GetLineCatsMsg(line_id);
	return lc;
    }

    line = line_id;
    if (line_id == -1) {
	line = display->selected.values->value[0];
    }

    if (!Vect_line_alive(display->mapInfo, line)) {
	DeadLineMsg(line);
	return lc;
    }

    Cats = Vect_new_cats_struct();

    if (Vect_read_line(display->mapInfo, NULL, Cats, line) < 0) {
	Vect_destroy_cats_struct(Cats);
	ReadLineMsg(line);
	return lc;
    }

    n_dblinks = Vect_get_num_dblinks(display->mapInfo);

    for (int dblink = 0; dblink < n_dblinks; dblink++) {
	fi = Vect_get_dblink(display->mapInfo, dblink);
	if (fi == NULL) {
	    DblinkMsg(dblink+1);
	    continue;
	}
	std::vector<int> cats;
	lc[fi->number] = cats;
    }

    for (int i = 0; i < Cats->n_cats; i++) {
	lc[Cats->field[i]].push_back(Cats->cat[i]);
    }

    Vect_destroy_cats_struct(Cats);

    return lc;
}

/**
   \brief Set categories for given feature/layer

   \param line feature id (-1 for first selected feature)
   \param layer layer number
   \param cats list of cats
   \param add True for add, False for delete

   \return new feature id (feature need to be rewritten)
   \return -1 on error
*/
int Digit::SetLineCats(int line_id, int layer, std::vector<int> cats, bool add)
{
    int line, ret, type;
    struct line_pnts *Points;
    struct line_cats *Cats;

    if (!display->mapInfo) {
	DisplayMsg();
	return -1;
    }

    if (line_id == -1 && display->selected.values->n_values < 1) {
	GetLineCatsMsg(line_id);
	return -1;
    }
    
    line = -1;
    if (line_id == -1) {
	line = display->selected.values->value[0];
    }
     
    if (!Vect_line_alive(display->mapInfo, line)) {
	DeadLineMsg(line);
	return -1;
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    type = Vect_read_line(display->mapInfo, Points, Cats, line);
    if (type < 0) {
	Vect_destroy_line_struct(Points);
	Vect_destroy_cats_struct(Cats);
	ReadLineMsg(line);
	return -1;
    }

    for (std::vector<int>::const_iterator c = cats.begin(), e = cats.end();
	 c != e; ++c) {
	if (add) {
	    Vect_cat_set(Cats, layer, *c);
	}
	else {
	    Vect_field_cat_del(Cats, layer, *c);
	}
	G_debug(3, "Digit.SetLineCats(): layer=%d, cat=%d, add=%d",
		layer, *c, add);
    }

    /* register changeset */
    AddActionToChangeset(changesets.size(), REWRITE, display->selected.values->value[0]);

    ret = Vect_rewrite_line(display->mapInfo, line, type,
			    Points, Cats);

    if (ret > 0) {
	/* updates feature id (id is changed since line has been rewriten) */
	changesets[changesets.size()-1][0].line = ret;
    }
    else {
	changesets.erase(changesets.size()-1);
    }

    if (line_id == -1) {
	/* update line id since the line was rewritten */
	display->selected.values->value[0] = ret;
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return ret;
}

/**
   \brief Copy categories from one vector feature to other

   \param fromId list of 'from' feature ids
   \param toId   list of 'to' feature ids

   \return number of modified features
   \return -1 on error
*/
int Digit::CopyCats(std::vector<int> fromId, std::vector<int> toId)
{
    int fline, tline, nlines, type;
    bool error;
    
    struct line_pnts *Points;
    struct line_cats *Cats_from, *Cats_to;

    Points = Vect_new_line_struct();
    Cats_from = Vect_new_cats_struct();
    Cats_to = Vect_new_cats_struct();

    nlines = 0;
    error = false;
    for (std::vector<int>::const_iterator fi = fromId.begin(), fe = fromId.end();
	 fi != fe && !error; ++fi) {
	fline = *fi;
	if (!Vect_line_alive(display->mapInfo, fline))
	    continue;

	type = Vect_read_line(display->mapInfo, NULL, Cats_from, fline);
	if (type < 0) {
	    nlines = -1;
	    error = true;
	}

	for(std::vector<int>::const_iterator ti = toId.begin(), te = toId.end();
	    ti != te && !error; ++ti) {
	    tline = *ti;
	    if (!Vect_line_alive(display->mapInfo, tline))
		continue;
	    type = Vect_read_line(display->mapInfo, Points, Cats_to, tline);
	    if (type < 0) {
		nlines = -1;
		error = true;
	    }

	    for (int i = 0; Cats_from->n_cats; i++) {
		if (Vect_cat_set(Cats_to, Cats_from->field[i], Cats_from->field[i]) < 1) {
		    nlines = -1;
		    error = true;
		}
	    }
	    
	    if (Vect_rewrite_line(display->mapInfo, tline, type, Points, Cats_to) < 0) {
		nlines = -1;
		error = true;
	    }
		
	    nlines++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats_from);
    Vect_destroy_cats_struct(Cats_to);

    return nlines;
}
