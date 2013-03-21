/*!
 * \file lib/vector/Vlib/cats.c
 *
 * \brief Vector library - Category management
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2012 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL, probably Dave Gerdes or Mike Higgins
 * \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 * \author Various updates by Martin Landa <landa.martin gmail.com>
 * \author Various updates by Markus Metz
 */

#include <stdlib.h>
#include <string.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

static int cmp(const void *pa, const void *pb);
static struct line_cats *Vect__new_cats_struct(void);


/*!
   \brief Creates and initializes line_cats structure.

   This structure is used for reading and writing vector cats. The
   library routines handle all memory allocation.
   
   To free allocated memory call Vect_destroy_cats_struct().

   \return struct line_cats *
   \return NULL on error
 */
struct line_cats *Vect_new_cats_struct()
{
    struct line_cats *p;

    if (NULL == (p = Vect__new_cats_struct()))
	G_fatal_error(_("Vect_new_cats_struct(): Out of memory"));

    return p;
}

/*!
   \brief Creates and initializes line_cats structure (lower level fn)

   This structure is used for reading and writing vector cats. The
   library routines handle all memory allocation.

   \return struct line_cats *
 */
static struct line_cats *Vect__new_cats_struct()
{
    struct line_cats *p;

    p = (struct line_cats *)G_malloc(sizeof(struct line_cats));

    /* n_cats MUST be initialized to zero */
    if (p)
	p->n_cats = 0;

    if (p)
	p->alloc_cats = 0;

    return p;
}

/*!
   \brief Frees all memory associated with line_cats structure,
   including the struct itself.

   \param p line_cats structure
 */
void Vect_destroy_cats_struct(struct line_cats *p)
{
    if (p) {			/* probably a moot test */
	if (p->n_cats) {
	    G_free((void *)p->field);
	    G_free((void *)p->cat);
	}
	G_free((void *)p);
    }
}

/*!
   \brief Add new field/cat to category structure if doesn't exist
   yet.

   \param[in,out] Cats line_cats structure
   \param[in] field layer number
   \param[in] cat category number

   \return number of categories
   \return 0 if no space for new category in structure, n_cats would be > GV_NCATS_MAX
   \return -1 on out of memory
   \return -2 if field out of range: 1 - GV_FIELD_MAX or cat out of range:  1 - GV_CAT_MAX
 */
int Vect_cat_set(struct line_cats *Cats, int field, int cat)
{
    register int n;

    /* check input values */
    /* compiler may warn: 
     * comparison is always 0 due to limited range of data type
     * but remember that limit is set to portable data type length
     * and machine native size may be longer */
    /*
       if (field < 1 || field > GV_FIELD_MAX || cat < 0 || cat > GV_CAT_MAX)
       return (-2);
     */

    /* go through old cats and find if field/category exists */
    for (n = 0; n < Cats->n_cats; n++) {
	if (Cats->field[n] == field && Cats->cat[n] == cat)
	    return (1);
    }

    /* field was not found so we shall append new cat */
    /* test if space exist */
    if (n >= GV_NCATS_MAX) {
	G_fatal_error(_("Too many categories (%d), unable to set cat %d (layer %d)"),
		      Cats->n_cats, cat, field);
    }

    if (Cats->n_cats == Cats->alloc_cats) {
	if (0 > dig_alloc_cats(Cats, Cats->n_cats + 100))
	    return (-1);
    }

    n = Cats->n_cats;
    Cats->field[n] = field;
    Cats->cat[n] = cat;
    Cats->n_cats++;
    return (1);
}

/*!
   \brief Get first found category of given field.

   <em>cat</em> is set to first category found or -1 if field was not
   found

   \param Cats pointer line_cats structure
   \param field layer number
   \param[out] cat pointer to variable where cat will be written (can be NULL)

   \return number of found cats for given field (first reported)
   \return 0 layer does not exist
 */
int Vect_cat_get(const struct line_cats *Cats, int field, int *cat)
{
    int n, ret;

    /* field was not found */    
    ret = 0;
    if (cat)
	*cat = -1;
    
    /* check input value */
    if (field < 1 || field > GV_FIELD_MAX)
	return (0);

    /* go through cats and find if field exist */
    for (n = 0; n < Cats->n_cats; n++) {
	if (Cats->field[n] == field) {
	    if (cat && ret == 0) {
		*cat = Cats->cat[n];
	    }
	    ret++;
	}
    }
    
    return ret;
}

/*!
   \brief Get list of categories of given field.

   \param Cats line_cats structure
   \param field layer number
   \param[out] cats pointer to list where cats will be written

   \return number of found categories
   \return -1 on invalid field
 */
int Vect_field_cat_get(const struct line_cats *Cats, int field, struct ilist *cats)
{
    int n;
    
    /* reset list of categories */
    Vect_reset_list(cats);

    /* check input value */
    if (field < 1 || field > GV_FIELD_MAX)
	return -1;
    
    /* go through cats and find if field exist */
    for (n = 0; n < Cats->n_cats; n++) {
	if (Cats->field[n] == field)
	    Vect_list_append(cats, Cats->cat[n]);
    }

    return cats->n_values;
}

/*!
   \brief Delete all categories of given layer

   \param[in,out] Cats line_cats structure
   \param field layer number

   \return number of categories deleted
   \return 0 layer does not exist
 */
int Vect_cat_del(struct line_cats *Cats, int field)
{
    int n, m, found;

    /* check input value */
    /*
       if (field < 1 || field > GV_FIELD_MAX)
       return (0);
     */

    /* go through cats and find if field exist */
    m = 0;
    for (n = 0; n < Cats->n_cats; n++) {
	if (Cats->field[n] != field) {
	    Cats->field[m] = Cats->field[n];
	    Cats->cat[m] = Cats->cat[n];
	    m++;
	}
    }
    found = Cats->n_cats - m;
    Cats->n_cats = m;

    return (found);
}

/*!
   \brief Delete field/cat from line_cats structure

   \param[in,out] Cats line_cats structure
   \param field layer number
   \param cat category to be deleted or -1 to delete all cats of given field

   \return number of categories deleted
   \return 0 field/category number does not exist
 */
int Vect_field_cat_del(struct line_cats *Cats, int field, int cat)
{
    register int n, m, found;

    /* check input value */
    /*
       if (field < 1 || field > GV_FIELD_MAX)
       return (0);
     */
     
    if (cat == -1)
	return Vect_cat_del(Cats, field);

    /* go through cats and find if field exist */
    m = 0;
    for (n = 0; n < Cats->n_cats; n++) {
	if (Cats->field[n] != field || Cats->cat[n] != cat) {
	    Cats->field[m] = Cats->field[n];
	    Cats->cat[m] = Cats->cat[n];
	    m++;
	}
    }
    found = Cats->n_cats - m;
    Cats->n_cats = m;

    return (found);
}

/*!
   \brief Reset category structure to make sure cats structure is clean to be re-used.

   I.e. it has no cats associated with it. Cats must have
   previously been created with Vect_new_cats_struct()

   \param[out] Cats line_cats structure

   \return 0
 */
int Vect_reset_cats(struct line_cats *Cats)
{
    Cats->n_cats = 0;

    return 0;
}

/*!
   \brief Allocate memory for cat_list structure.

   \return pointer to allocated structure
   \return NULL if out of memory
 */
struct cat_list *Vect_new_cat_list()
{
    struct cat_list *p;

    p = (struct cat_list *)G_malloc(sizeof(struct cat_list));

    /* n_ranges MUST be initialized to zero */
    if (p) {
	p->n_ranges = 0;
	p->alloc_ranges = 0;
	p->field = 0;
	p->min = NULL;
	p->max = NULL;
    }

    return p;
}


/*!
   \brief Frees allocated cat_list memory.

   \param p pointer to line_cats structure
 */
void Vect_destroy_cat_list(struct cat_list *p)
{
    if (p) {			/* probably a moot test */
	if (p->n_ranges) {
	    G_free((void *)p->min);
	    G_free((void *)p->max);
	}
	G_free((void *)p);
    }
}


/*!
   \brief Converts string of categories and cat ranges separated by commas to cat_list.

   \par Examples of string:
   \verbatim
   5,6,7
   3-9
   2,3,5-9,20\endverbatim
   
   \par Example:
   \code
   ...
   str = "2,3,5-9,20"
   cat_list = Vect_new_cat_list()

   Vect_str_to_cat_list(str, cat_list)
   \endcode
   \verbatim
   cat_list->field = 0
   cat_list->n_ranges = 4
   cat_list->min = {2, 3, 5, 20}
   cat_list->max = {2, 3, 9, 20}\endverbatim

   \param[in] str category list as a string
   \param[in,out] list pointer to cat_list structure

   \return number of errors in ranges
 */
int Vect_str_to_cat_list(const char *str, struct cat_list *list)
{
    int i, nr, l, err = 0;
    const char *s, *e;
    char buf[100];
    int min, max;

    G_debug(3, "Vect_str_to_cat_list(): str = %s", str);

    list->n_ranges = 0;
    l = strlen(str);

    /* find number of ranges */
    nr = 1;			/* one range */
    for (i = 0; i < l; i++)
	if (str[i] == ',')
	    nr++;

    /* allocate space */
    if (list->alloc_ranges == 0) {
	list->min = (int *)G_malloc(nr * sizeof(int));
	list->max = (int *)G_malloc(nr * sizeof(int));
    }
    else if (nr > list->alloc_ranges) {
	list->min = (int *)G_realloc((void *)list->min, nr * sizeof(int));
	list->max = (int *)G_realloc((void *)list->max, nr * sizeof(int));
    }

    /* go through string and read ranges */
    i = 0;
    s = str;

    while (s) {
	e = (char *)strchr(s, ',');	/* first comma */
	if (e) {
	    l = e - s;
	    strncpy(buf, s, l);
	    buf[l] = '\0';
	    s = e + 1;
	}
	else {
	    strcpy(buf, s);
	    s = NULL;
	}

	G_debug(3, "  buf = %s", buf);
	if (sscanf(buf, "%d-%d", &min, &max) == 2) {
	}
	else if (sscanf(buf, "%d", &min) == 1)
	    max = min;
	else {			/* error */

	    G_warning(_("Unable to convert category string '%s' (from '%s') to category range"),
		      buf, str);
	    err++;
	    continue;
	}

	list->min[i] = min;
	list->max[i] = max;
	i++;
    }

    list->n_ranges = i;

    return (err);
}

/*!
   \brief Convert ordered array of integers to cat_list structure.

   \param vals array of integers
   \param nvals number of values
   \param[in,out] list pointer to cat_list structure

   \return number of ranges
 */
int Vect_array_to_cat_list(const int *vals, int nvals, struct cat_list *list)
{
    int i, range;

    G_debug(1, "Vect_array_to_cat_list()");
    range = -1;
    for (i = 0; i < nvals; i++) {
	if (i == 0 || (vals[i] - list->max[range]) > 1) {
	    range++;
	    if (range == list->alloc_ranges) {
		list->alloc_ranges += 1000;
		list->min = (int *)G_realloc((void *)list->min,
					     list->alloc_ranges *
					     sizeof(int));
		list->max =
		    (int *)G_realloc((void *)list->max,
				     list->alloc_ranges * sizeof(int));
	    }
	    list->min[range] = vals[i];
	    list->max[range] = vals[i];
	}
	else {
	    list->max[range] = vals[i];
	}
    }

    list->n_ranges = range + 1;

    return (list->n_ranges);
}

/*!
   \brief Check if category number is in list.

   \param cat category number
   \param list cat_list structure

   \return TRUE if cat is in list
   \return FALSE if not
 */
int Vect_cat_in_cat_list(int cat, const struct cat_list *list)
{
    int i;

    for (i = 0; i < list->n_ranges; i++)
	if (cat >= list->min[i] && cat <= list->max[i])
	    return (TRUE);

    return (FALSE);
}

/*!
   \brief Set category constraints using 'where' or 'cats' option and layer number.

   \param Map pointer to Map_info structure
   \param layer layer number
   \param where where statement
   \param catstr category list as string

   \return pointer to cat_list structure or NULL
 */
struct cat_list *Vect_cats_set_constraint(struct Map_info *Map, int layer,
                                         char *where, char *catstr)
{
    struct cat_list *list = NULL;
    int ret;

    if (layer < 1) {
	G_warning(_("Layer number must be > 0 for category constraints"));
	/* no valid constraints, all categories qualify */
	return list;
    }

    /* where has precedence over cats */
    if (where) {
	struct field_info *Fi = NULL;
	dbDriver *driver = NULL;
	int ncats, *cats = NULL;
	int i, j;

	if (catstr)
	    G_warning(_("'%s' and '%s' parameters were supplied, cats will be ignored"), "where", "cats");

	Fi = Vect_get_field(Map, layer);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  layer);
	}

	G_verbose_message(_("Loading categories from table <%s>..."), Fi->table);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	
	ncats = db_select_int(driver, Fi->table, Fi->key, where,
			      &cats);
	if (ncats == -1)
		G_fatal_error(_("Unable select records from table <%s>"),
			      Fi->table);
	G_verbose_message(_("%d categories loaded"), ncats);
	    
	db_close_database_shutdown_driver(driver);

	/* sort */
	qsort(cats, ncats, sizeof(int), cmp);
	
	/* remove duplicates */
	j = 1;
	for (i = 1; i < ncats; i++) {
	    if (cats[i] != cats[j - 1]) {
		cats[j] = cats[i];
		j++;
	    }
	}
	ncats = j;
	
	/* convert to cat list */
	list = Vect_new_cat_list();
	
	ret = Vect_array_to_cat_list(cats, ncats, list);
	if (ret == 0)
	    G_warning(_("No categories selected with '%s' option"), "where");
	
	if (cats)
	    G_free(cats);
    }
    else if (catstr) {
	list = Vect_new_cat_list();

	ret = Vect_str_to_cat_list(catstr, list);
	if (ret > 0)
	    G_warning(_("%d errors in '%s' option"), ret, "cats");
    }
    
    if (list) {
	if (list->n_ranges < 1) {
	    Vect_destroy_cat_list(list);
	    list = NULL;
	}
	else
	    list->field = layer;
    }
	
    return list;
}

/*!
   \brief Check if categories match with category constraints.

   \param Cats line_cats structure
   \param layer layer number
   \param list cat_list structure

   \return 0 no match, categories are outside constraints
   \return 1 match, categories are inside constraints
 */
int Vect_cats_in_constraint(struct line_cats *Cats, int layer,
			      struct cat_list *list)
{
    int i;

    if (layer < 1) {
	G_warning(_("Layer number must be > 0 for category constraints"));
	/* no valid constraint, all categories qualify */
	return 1;
    }

    if (list) {
	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == layer &&
		Vect_cat_in_cat_list(Cats->cat[i], list)) {
		return 1;
	    }
	}
	return 0;
    }

    for (i = 0; i < Cats->n_cats; i++) {
	if (Cats->field[i] == layer)
	    return 1;
    }
	
    return 0;
}


/*!
   \brief Check if category is in ordered array of integers.

   \param cat category number
   \param array ordered array of integers
   \param ncats number of categories in array

   \return TRUE if cat is in list
   \return FALSE if it is not
 */
int Vect_cat_in_array(int cat, const int *array, int ncats)
{
    int *i;

    i = bsearch((void *)&cat, (void *)array, (size_t) ncats,
		sizeof(int), cmp);

    return (i != NULL);
}

/* return -1 if *p1 < *p2
 * return  1 if *p1 > *p2
 * return  0 if *p1 == *p2 */
static int cmp(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    return (*p1 > *p2);
}
