/**
   \file vdigit/cats.cpp

   \brief wxvdigit - category management

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com>
*/

extern "C" {
#include <grass/dbmi.h>
}

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
	/* DisplayMsg(); */
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
	    cats[b->first] = 0; /* first category 1 */
	    G_debug(3, "wxDigit.InitCats(): layer=%d, cat=%d", b->first, cats[b->first]);
	}

    }

    return 0;
}

/**
   \brief Get max category number for layer

   \param layer layer number

   \return category number (0 if no category found)
   \return -1 on error
*/
int Digit::GetCategory(int layer)
{
    if (cats.find(layer) != cats.end()) {
	G_debug(3, "v.digit.GetCategory(): layer=%d, cat=%d", layer, cats[layer]);
	return cats[layer];
    }

    return 0;
}

/**
   \brief Set max category number for layer

   \param layer layer number
   \param cats  category number to be set

   \return previously set category
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
    int line;
    struct line_cats *Cats;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return lc;
    }

    if (line_id == -1 && display->selected.ids->n_values < 1) {
	/* GetLineCatsMsg(line_id); */
	return lc;
    }
    
    line = line_id;
    if (line_id == -1) {
	line = display->selected.ids->value[0];
    }

    if (!Vect_line_alive(display->mapInfo, line)) {
	display->DeadLineMsg(line);
	return lc;
    }

    Cats = Vect_new_cats_struct();

    if (Vect_read_line(display->mapInfo, NULL, Cats, line) < 0) {
	Vect_destroy_cats_struct(Cats);
	display->ReadLineMsg(line);
	return lc;
    }
    
    for (int i = 0; i < Cats->n_cats; i++) {
	if (lc.find(Cats->field[i]) == lc.end()) {
	    std::vector<int> cats;
	    lc[Cats->field[i]] = cats;
	}
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
	display->DisplayMsg();
	return -1;
    }

    if (line_id == -1 && display->selected.ids->n_values < 1) {
	display->GetLineCatsMsg(line_id);
	return -1;
    }
    
    if (line_id == -1) {
	line = display->selected.ids->value[0];
    }
    else {
	line = line_id;
    }

    if (!Vect_line_alive(display->mapInfo, line)) {
	display->DeadLineMsg(line);
	return -1;
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    type = Vect_read_line(display->mapInfo, Points, Cats, line);
    if (type < 0) {
	Vect_destroy_line_struct(Points);
	Vect_destroy_cats_struct(Cats);
	display->ReadLineMsg(line);
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
    // AddActionToChangeset(changesets.size(), REWRITE, display->selected.ids->value[0]);

    ret = Vect_rewrite_line(display->mapInfo, line, type,
			    Points, Cats);

    /* TODO
       updates feature id (id is changed since line has been rewriten)
       if (ret > 0) {
       
       changesets[changesets.size()-1][0].line = ret;
       }
       else {
       changesets.erase(changesets.size()-1);
       }
    */
    
    if (line_id == -1) {
	/* update line id since the line was rewritten */
	display->selected.ids->value[0] = ret;
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return ret;
}

/**
   \brief Copy categories from one vector feature to other

   \param fromId list of 'from' feature ids
   \param toId   list of 'to' feature ids
   \param copyAttrb duplicate attribures instead of copying categories

   \return number of modified features
   \return -1 on error
*/
int Digit::CopyCats(std::vector<int> fromId, std::vector<int> toId, bool copyAttrb)
{
    int fline, tline, nlines, type;
    int cat;
    
    struct line_pnts *Points;
    struct line_cats *Cats_from, *Cats_to;

    Points = Vect_new_line_struct();
    Cats_from = Vect_new_cats_struct();
    Cats_to = Vect_new_cats_struct();

    nlines = 0;

    for (std::vector<int>::const_iterator fi = fromId.begin(), fe = fromId.end();
	 fi != fe; ++fi) {

	fline = *fi;
	if (!Vect_line_alive(display->mapInfo, fline))
	    continue;

	type = Vect_read_line(display->mapInfo, NULL, Cats_from, fline);
	if (type < 0) {
	    display->ReadLineMsg(fline);
	    return -1;
	}

	for(std::vector<int>::const_iterator ti = toId.begin(), te = toId.end();
	    ti != te; ++ti) {

	    tline = *ti;
	    if (!Vect_line_alive(display->mapInfo, tline))
		continue;

	    type = Vect_read_line(display->mapInfo, Points, Cats_to, tline);
	    if (type < 0) {
		display->ReadLineMsg(tline);
		return -1;
	    }

	    for (int i = 0; i < Cats_from->n_cats; i++) {
		if (!copyAttrb) {
		    cat = Cats_from->cat[i]; /* duplicate category */
		}
		else {
		    /* duplicate attributes */
		    struct field_info *fi;
		    char buf[GSQL_MAX];
		    dbDriver *driver;
		    dbHandle handle;
		    dbCursor cursor;
		    dbTable *table;
		    dbColumn *column;
		    dbValue *value;
		    dbString stmt, value_string;
		    int col, ncols;
		    int more, ctype;
		    		    
		    cat = ++cats[Cats_from->field[i]];

		    fi = Vect_get_field(display->mapInfo, Cats_from->field[i]);

		    if (fi == NULL) {
			display->DblinkMsg(Cats_from->field[i]);
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
			db_shutdown_driver(driver);
			display->DbDatabaseMsg(fi->driver, fi->database);
			return -1;
		    }

		    db_init_string (&stmt);
		    sprintf (buf, "SELECT * FROM %s WHERE %s=%d",
			     fi->table, fi->key, Cats_from->cat[i]);
		    db_set_string(&stmt, buf);

		    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK) {
			db_close_database(driver);
			db_shutdown_driver(driver);
			display->DbSelectCursorMsg(db_get_string(&stmt));
			return -1;
		    }


		    table = db_get_cursor_table(&cursor);
		    ncols = db_get_table_number_of_columns(table);
		    
		    sprintf(buf, "INSERT INTO %s VALUES (", fi->table);
		    db_set_string(&stmt, buf);

		    /* fetch the data */
		    while (1) {
			if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
			    db_close_database(driver);
			    db_shutdown_driver(driver);
			    return -1;
			}
			if (!more)
			    break;
			
			for (col = 0; col < ncols; col++) {
			    if (col > 0)
				db_append_string(&stmt, ",");
			    
			    column = db_get_table_column(table, col);
			    if (strcmp(db_get_column_name(column), fi->key) == 0) {
				sprintf(buf, "%d", cat);
				db_append_string(&stmt, buf);
				continue;
			    }
			    
			    value = db_get_column_value(column);
			    db_convert_column_value_to_string(column, &value_string);
			    if (db_test_value_isnull(value))
				db_append_string(&stmt, "NULL");
			    else {
				ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
				if (ctype != DB_C_TYPE_STRING) 
				    db_append_string(&stmt, db_get_string(&value_string));
				else {
				    sprintf(buf, "'%s'", db_get_string(&value_string));
				    db_append_string(&stmt, buf);
				}
			    }
			}
		    }
		    db_append_string(&stmt, ")");
		    
		    if (db_execute_immediate (driver, &stmt) != DB_OK ) {
			db_close_database(driver);
			db_shutdown_driver(driver);
			display->DbExecuteMsg(db_get_string(&stmt));
			return -1;
		    }
		    
		    db_close_database(driver);
		    db_shutdown_driver(driver);
		}
		
		if (Vect_cat_set(Cats_to, Cats_from->field[i], cat) < 1) {
		    continue;
		}
	    }
	    
	    if (Vect_rewrite_line(display->mapInfo, tline, type, Points, Cats_to) < 0) {
		display->WriteLineMsg();
		return -1;
	    }
	
	    G_debug(1, "Digit::CopyCats(): fline=%d, tline=%d", fline, tline);
	    
	    nlines++;
	}
    }
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats_from);
    Vect_destroy_cats_struct(Cats_to);

    return nlines;
}
