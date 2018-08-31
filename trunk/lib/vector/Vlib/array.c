/*!
   \file lib/vector/Vlib/array.c

   \brief Vector library - category array

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>


/* function prototypes */
static int cmp(const void *pa, const void *pb);
static int in_array(int *cats, size_t ncats, int cat);


/*!
   \brief Create new struct varray and allocate space for given number of items.

   Space allocated is 'size + 1' so that lines are accessed by line id.
   Array values are set to 0.

   \param size size of array

   \return pointer to new struct varray
   \return NULL if failed
 */

struct varray *Vect_new_varray(int size)
{
    struct varray *p;

    p = (struct varray *) G_malloc(sizeof(struct varray));

    if (p == NULL)
	return NULL;

    p->size = size;
    p->c = (int *)G_calloc(sizeof(char) * size + 1, sizeof(int));

    if (p->c == NULL) {
	G_free(p);
	return NULL;
    }

    return p;
}

/*!
   \brief Set values in 'varray' to 'value' from category string.

   If category of object of given type is in <em>cstring</em> (string
   representing category list like: '1,3,5-7'). <em>type</em> may be
   either: GV_AREA or: GV_POINT | GV_LINE | GV_BOUNDARY | GV_CENTROID

   Array is not reset to zero before, but old values (if any > 0) are
   overwritten.  Array must be initialised by Vect_new_varray() call.

   \param Map vector map
   \param field layer number
   \param cstring pointer to string with categories
   \param type feature type
   \param value value to set up
   \param[out] varray varray structure to modify

   \return number of items set
   \return -1 on error
 */
int
Vect_set_varray_from_cat_string(const struct Map_info *Map, int field,
				const char *cstring, int type, int value,
				struct varray * varray)
{
    int ret;
    struct cat_list *Clist;

    G_debug(4, "Vect_set_varray_from_cat_string(): cstring = '%s'", cstring);

    Clist = Vect_new_cat_list();

    ret = Vect_str_to_cat_list(cstring, Clist);

    if (ret > 0)
	G_warning(_("%d errors in category string"), ret);

    G_debug(4, "  %d ranges in clist", Clist->n_ranges);

    ret =
	Vect_set_varray_from_cat_list(Map, field, Clist, type, value, varray);

    Vect_destroy_cat_list(Clist);

    return ret;
}

/*!
   \brief Set values in 'varray' to 'value' from category list

   If category of object of given type is in <em>clist</em> (category
   list).  <em>type</em> may be either: GV_AREA or: GV_POINT | GV_LINE
   | GV_BOUNDARY | GV_CENTROID

   Array is not reset to zero before, but old values (if any > 0) are
   overwritten.  Array must be initialised by Vect_new_varray() call.

   \param Map vector map
   \param field layer number
   \param clist list of categories
   \param type feature type
   \param value value to set up
   \param[out] varray varray structure to modify

   \return number of items set
   \return -1 on error
 */
int
Vect_set_varray_from_cat_list(const struct Map_info *Map, int field,
			      struct cat_list *clist, int type, int value,
			      struct varray * varray)
{
    int i, n, centr, cat;
    int ni = 0;			/* number of items set */
    int ltype;			/* line type */
    struct line_cats *Cats;

    G_debug(4, "Vect_set_varray_from_cat_list(): field = %d", field);

    /* Check type */
    if ((type & GV_AREA) && (type & (GV_POINTS | GV_LINES))) {
	G_warning(_("Mixed area and other type requested for vector array"));
	return 0;
    }

    Cats = Vect_new_cats_struct();

    if (type & GV_AREA) {	/* Areas */
	n = Vect_get_num_areas(Map);

	if (n > varray->size) {	/* not enough space */
	    G_warning(_("Not enough space in vector array"));
	    return 0;
	}

	for (i = 1; i <= n; i++) {
	    centr = Vect_get_area_centroid(Map, i);
	    if (centr <= 0)
		continue;	/* No centroid */

	    Vect_read_line(Map, NULL, Cats, centr);
	    if (!Vect_cat_get(Cats, field, &cat))
		continue;	/* No such field */

	    if (Vect_cat_in_cat_list(cat, clist)) {	/* cat is in list */
		varray->c[i] = value;
		ni++;
	    }
	}
    }
    else {			/* Lines */
	n = Vect_get_num_lines(Map);

	if (n > varray->size) {	/* not enough space */
	    G_warning(_("Not enough space in vector array"));
	    return 0;
	}

	for (i = 1; i <= n; i++) {
	    ltype = Vect_read_line(Map, NULL, Cats, i);

	    if (!(ltype & type))
		continue;	/* is not specified type */

	    if (!Vect_cat_get(Cats, field, &cat))
		continue;	/* No such field */

	    if (Vect_cat_in_cat_list(cat, clist)) {	/* cat is in list */
		varray->c[i] = value;
		ni++;
	    }
	}

    }

    Vect_destroy_cats_struct(Cats);

    return ni;
}

/* compare 2 integers in array */
static int cmp(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

/* check if cat is in array */
static int in_array(int *cats, size_t ncats, int cat)
{
    int *p;

    p = (int *)bsearch((void *)&cat, cats, ncats, sizeof(int), cmp);

    if (p == NULL)
	return 0;

    return 1;
}

/*!
   \brief Set values in 'varray' to 'value' from DB (where statement)

   I category of object of given type is in categories selected from
   DB based on where statement (given without where). <em>type</em>
   may be either: GV_AREA or: GV_POINT | GV_LINE | GV_BOUNDARY |
   GV_CENTROID

   Array is not reset to zero before, but old values (if any > 0) are
   overwritten. Array must be initialised by Vect_new_varray() call.

   \param Map vector map
   \param field layer number
   \param where where statement
   \param type feature type
   \param value value to set up
   \param[out] varray varray structure to modify

   \return number of items set
   \return -1 on error
 */
int
Vect_set_varray_from_db(const struct Map_info *Map, int field, const char *where,
			int type, int value, struct varray * varray)
{
    int i, n, c, centr, *cats;
    int ncats;
    int ni = 0;			/* number of items set */
    int ltype;			/* line type */
    struct line_cats *Cats;
    struct field_info *Fi;
    dbDriver *driver;

    G_debug(4, "Vect_set_varray_from_db(): field = %d where = '%s'", field,
	    where);

    /* Note: use category index once available */

    /* Check type */
    if ((type & GV_AREA) && (type & (GV_POINTS | GV_LINES))) {
	G_warning(_("Mixed area and other type requested for vector array"));
	return 0;
    }

    Cats = Vect_new_cats_struct();

    /* Select categories from DB to array */
    Fi = Vect_get_field(Map, field);
    if (Fi == NULL) {
	G_warning(_("Database connection not defined for layer %d"), field);
	return -1;
    }

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  Fi->database, Fi->driver);
	return -1;
    }

    ncats = db_select_int(driver, Fi->table, Fi->key, where, &cats);

    db_close_database_shutdown_driver(driver);

    if (ncats == -1) {
	G_warning(_("Unable to select record from table <%s> (key %s, where %s)"),
		  Fi->table, Fi->key, where);
	return -1;
    }

    if (type & GV_AREA) {	/* Areas */
	n = Vect_get_num_areas(Map);

        /* IMHO varray should be allocated only when it's required AND only as large as required
        as WHERE will create a small subset of all vector features and thus on large datasets
        it's waste of memory to allocate it for all features. */
	if (n > varray->size) {	/* not enough space */
	    G_warning(_("Not enough space in vector array"));
	    return 0;
	}

	for (i = 1; i <= n; i++) {
	    centr = Vect_get_area_centroid(Map, i);
	    if (centr <= 0)
		continue;	/* No centroid */

	    Vect_read_line(Map, NULL, Cats, centr);
	    /*if ( !Vect_cat_get(Cats, field, &cat) ) continue; No such field */
	    for (c = 0; c < Cats->n_cats; c++) {
		if (Cats->field[c] == field &&
		    in_array(cats, ncats, Cats->cat[c])) {
		    varray->c[i] = value;
		    ni++;
		    break;
		}
	    }

	    /*
	       if ( in_array ( cats, ncats, cat ) ) {
	       varray->c[i] = value;
	       ni++;
	       }
	     */
	}
    }
    else {			/* Lines */
	n = Vect_get_num_lines(Map);

	if (n > varray->size) {	/* not enough space */
	    G_warning(_("Not enough space in vector array"));
	    return 0;
	}

	for (i = 1; i <= n; i++) {
	    ltype = Vect_read_line(Map, NULL, Cats, i);

	    if (!(ltype & type))
		continue;	/* is not specified type */

	    /* if ( !Vect_cat_get(Cats, field, &cat) ) continue;  No such field */
	    for (c = 0; c < Cats->n_cats; c++) {
		if (Cats->field[c] == field &&
		    in_array(cats, ncats, Cats->cat[c])) {
		    varray->c[i] = value;
		    ni++;
		    break;
		}
	    }
	    /*
	       if ( in_array ( cats, ncats, cat ) ) {
	       varray->c[i] = value;
	       ni++;
	       }
	     */
	}

    }

    G_free(cats);
    Vect_destroy_cats_struct(Cats);

    return ni;
}
