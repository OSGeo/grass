/*!
  \brief Old sites library

  These functions and definitions support the site format for 5.0
  (format proposed by Dave Gerdes):
  
  \verbatim
  easting|northing|[z|[d4|]...][#category_int] [ [@attr_text OR %flt] ... ]
  \endverbatim
 
  to allow multidimensions (everything preceding the last '|') and any
  number of text or numeric attribute fields.
  
  \author James Darrell McCauley <mccauley@ecn.purdue.edu> (31 Jan 1994)
*/

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/site.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#define DQUOTE '"'
#define SPACE ' '
#define BSLASH 92
#define PIPE '|'

#define ispipe(c) (c==PIPE)
#define isnull(c) (c=='\0')

#define FOUND_ALL(s,n,dim,c,d) (((s->cattype != -1 && !n) || \
				 (dim < s->dim_alloc) || \
				 (c < s->str_alloc) || \
				 (d < s->dbl_alloc))?0:1)


static char *next_att(const char *);
static int cleanse_string(char *);
static int G__oldsite_get(FILE *, Site *, int);

static int site_att_cmp(const void *pa, const void *pb)
{
    const struct site_att *a = pa, *b = pb;

    return a->cat - b->cat;
}

/*!
  \brief Get site
  
  \param Map
  \param s

  \return 0 on success
  \return -1 on EOF
  \return -2 on other fatal error or insufficient data,
  \return 1 on format mismatch (extra data)
*/
int G_site_get(struct Map_info *Map, Site * s)
{
    int i, type, cat;
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    struct site_att *sa;

    if (Points == NULL)
	Points = Vect_new_line_struct();
    if (Cats == NULL)
	Cats = Vect_new_cats_struct();

    while (1) {
	type = Vect_read_next_line(Map, Points, Cats);

	if (type == -1)
	    return -2;		/* Error */
	if (type == -2)
	    return -1;		/* EOF */
	if (type != GV_POINT)
	    continue;		/* Is not point */

	Vect_cat_get(Cats, 1, &cat);

	G_debug(4, "Site: %f|%f|%f|#%d", Points->x[0], Points->y[0],
		Points->z[0], cat);

	s->east = Points->x[0];
	s->north = Points->y[0];
	if (Vect_is_3d(Map))
	    s->dim[0] = Points->z[0];

	s->ccat = cat;

	/* find att */

	if (Map->n_site_att > 0) {
	    sa = (struct site_att *) bsearch((void *)&cat, (void *)Map->site_att,
				      Map->n_site_att, sizeof(struct site_att),
				      site_att_cmp);

	    if (sa == NULL) {
		G_warning(_("Attributes for category %d not found"), cat);
		for (i = 0; i < Map->n_site_dbl; i++)
		    s->dbl_att[i] = 0;
		for (i = 0; i < Map->n_site_str; i++)
		    strncpy(s->str_att[i], "", MAX_SITE_STRING);
	    }
	    else {
		for (i = 0; i < Map->n_site_dbl; i++)
		    s->dbl_att[i] = sa->dbl[i];
		for (i = 0; i < Map->n_site_str; i++)
		    strncpy(s->str_att[i], sa->str[i], MAX_SITE_STRING);
	    }
	}

	return 0;
    }
}

/*!
  \brief Writes a site to file open on fptr

  \param Map
  \param s
  
  \return
*/
int G_site_put(struct Map_info *Map, const Site * s)
{
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;

    if (Points == NULL)
	Points = Vect_new_line_struct();
    if (Cats == NULL)
	Cats = Vect_new_cats_struct();

    Vect_reset_line(Points);
    Vect_reset_cats(Cats);

    /* no 3D support so far: s->dim[0] */
    Vect_append_point(Points, s->east, s->north, 0.0);

    G_debug(4, "cattype = %d", s->cattype);

    if (s->cattype == FCELL_TYPE || s->cattype == DCELL_TYPE)
	G_fatal_error(_("Category must be integer"));

    if (s->cattype == CELL_TYPE)
	Vect_cat_set(Cats, 1, s->ccat);

    Vect_write_line(Map, GV_POINT, Points, Cats);

    return 0;
}

/*!
  \brief Tries to guess the format of a sites list

   The dimensionality, the presence/type of a category, and the number
   of string and decimal attributes) by reading the first record in
   the file.

   \return 0 on success
   \return -1 on EOF
   \return -2 for other error
*/
int G_site_describe(struct Map_info *Map, int *dims, int *cat, int *strs,
		    int *dbls)
{
    if (Vect_is_3d(Map)) {
	G_debug(1, "Vector is 3D -> number of site dimensions is 3");
	*dims = 3;
    }
    else {
	G_debug(1, "Vector is 2D -> number of site dimensions is 2");
	*dims = 2;
    }

    *cat = CELL_TYPE;

    /* attributes ignored for now, later read from DB */
    *dbls = Map->n_site_dbl;
    *strs = Map->n_site_str;

    return 0;
}

/*!
  \brief Writes site_head struct
*/
int G_site_put_head(struct Map_info *Map, Site_head * head)
{
    static char buf[128];

    if (head->name != NULL)
	Vect_set_map_name(Map, head->name);

    /* crashes:
       if (head->desc!=NULL)
       Vect_set_comment (Map, head->desc);
     */

    /*
       if (head->form!=NULL)
       fprintf(ptr,"form|%s\n",head->form);
       if (head->labels!=NULL)
       fprintf(ptr,"labels|%s\n",head->labels);
     */
    /* time could be in (char *) stime, (struct TimeStamp *) time, 
       both, or neither */
    if (head->stime != NULL || head->time != NULL) {
	if (head->time != NULL) {	/* TimeStamp struct has precendence */
	    G_format_timestamp(head->time, buf);
	    Vect_set_date(Map, buf);
	}
	else if (head->stime != NULL) {	/* next check string */
	    if (head->time == NULL) {
		if ((head->time =
		     (struct TimeStamp *)G_malloc(sizeof(struct TimeStamp)))
		    == NULL)
		    G_fatal_error(_("Memory error in writing timestamp"));
		else if (G_scan_timestamp(head->time, head->stime) < 0) {
		    G_warning(_("Illegal TimeStamp string"));
		    return -1;	/* added to prevent crash 5/2000 MN */
		}
	    }
	    G_format_timestamp(head->time, buf);
	    head->stime = G_store(buf);
	    Vect_set_date(Map, head->stime);
	}
    }
    return 0;
}

struct Map_info *G_sites_open_old(const char *name, const char *mapset)
{
    struct Map_info *Map;
    struct field_info *fi;
    int more, nrows, row, ncols, col, ndbl, nstr, adbl, astr, ctype;
    struct site_att *sa;

    dbDriver *driver;
    dbString stmt;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;

    G_message(
	_("Dev note: Adapted sites library used for vector points. "
	  "(module should be updated to GRASS 6 vector library)"));

    Map = (struct Map_info *)G_malloc(sizeof(struct Map_info));

    Vect_set_open_level(1);
    Vect_open_old(Map, name, mapset);

    G_debug(1, "Vector map opened");

    /* Load attributes */
    Map->site_att = NULL;
    Map->n_site_att = 0;
    Map->n_site_dbl = 0;
    Map->n_site_str = 0;

    fi = Vect_get_field(Map, 1);
    if (fi == NULL) {		/* not attribute table */
	G_debug(1, "No attribute table");
	return Map;
    }

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database,
		      fi->driver);

    db_init_string(&stmt);
    db_set_string(&stmt, "select * from ");
    db_append_string(&stmt, fi->table);

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error(_("Unable to open select cursor: '%s'"),
		      db_get_string(&stmt));

    nrows = db_get_num_rows(&cursor);
    G_debug(1, "%d rows selected from vector attribute table", nrows);

    Map->site_att = (struct site_att *) malloc(nrows * sizeof(struct site_att));
    Map->n_site_att = nrows;

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);

    row = 0;
    adbl = astr = 0;
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error(_("Cannot fetch row"));

	if (!more)
	    break;

	/* Get number of each type */
	if (row == 0) {
	    for (col = 0; col < ncols; col++) {
		column = db_get_table_column(table, col);
		ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));

		if (strcmp(db_get_column_name(column), fi->key) == 0)
		    continue;

		switch (ctype) {
		case DB_C_TYPE_INT:
		case DB_C_TYPE_DOUBLE:
		    adbl++;
		    break;
		case DB_C_TYPE_STRING:
		case DB_C_TYPE_DATETIME:
		    astr++;
		    break;
		}
	    }
	    Map->n_site_dbl = adbl;
	    Map->n_site_str = astr;
	    G_debug(1, "adbl = %d astr = %d", adbl, astr);
	}

	sa = &(Map->site_att[row]);
	sa->dbl = (double *)malloc(adbl * sizeof(double));
	sa->str = (char **)malloc(astr * sizeof(char *));

	ndbl = nstr = 0;
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	    value = db_get_column_value(column);

	    if (strcmp(db_get_column_name(column), fi->key) == 0) {
		sa->cat = db_get_value_int(value);
	    }
	    else {
		switch (ctype) {
		case DB_C_TYPE_INT:
		    sa->dbl[ndbl] = db_get_value_int(value);
		    ndbl++;
		    break;
		case DB_C_TYPE_DOUBLE:
		    sa->dbl[ndbl] = db_get_value_double(value);
		    ndbl++;
		    break;
		case DB_C_TYPE_STRING:
		    sa->str[nstr] = G_store(db_get_value_string(value));
		    nstr++;
		    break;
		case DB_C_TYPE_DATETIME:
		    sa->str[nstr] = "";	/* TODO */
		    nstr++;
		    break;
		}
	    }
	}
	row++;
    }
    db_close_database_shutdown_driver(driver);

    /* sort attributes */
    qsort((void *)Map->site_att, Map->n_site_att, sizeof(struct site_att),
	  site_att_cmp);

    return Map;
}


struct Map_info *G_sites_open_new(const char *name)
{
    struct Map_info *Map;

    G_message(
	_("Dev note: Adapted sites library used for vector points. "
	  "(module should be updated to GRASS 6 vector library)"));
    G_warning("Site/vector attributes ignored.");

    Map = (struct Map_info *)G_malloc(sizeof(struct Map_info));

    Vect_open_new(Map, name, 0);

    G_debug(1, "New vector map opened");

    return Map;
}

struct Map_info *G_fopen_sites_old(const char *name, const char *mapset)
{
    return G_sites_open_old(name, mapset);
}

struct Map_info *G_fopen_sites_new(const char *name)
{
    return G_sites_open_new(name);
}

/*!
  \brief Free memory for a Site struct

  \param s
*/
void G_site_free_struct(Site * s)
{
    if (s->dim_alloc)
	G_free(s->dim);
    if (s->str_alloc)
	G_free(s->str_att);
    if (s->dbl_alloc)
	G_free(s->dbl_att);
    G_free(s);

    return;
}

/*!
  \brief Allocate memory for a Site struct.

  cattype= -1 (no cat), CELL_TYPE, FCELL_TYPE, or DCELL_TYPE 
  
  \return properly allocated site struct or NULL on error. 
*/
Site *G_site_new_struct(RASTER_MAP_TYPE cattype,
			int n_dim, int n_s_att, int n_d_att)
{
    int i;
    Site *s;

    if (n_dim < 2 || n_s_att < 0 || n_d_att < 0)
	G_fatal_error(_("G_oldsite_new_struct: invalid # dims or fields"));

    if ((s = (Site *) G_malloc(sizeof(Site))) == NULL)
	return (Site *) NULL;

    s->cattype = cattype;
    s->ccat = s->fcat = s->dcat = 0;

    if (n_dim > 2) {
	if ((s->dim =
	     (double *)G_malloc((n_dim - 2) * sizeof(double))) == NULL) {
	    G_free(s);
	    return (Site *) NULL;
	}
    }
    s->dim_alloc = n_dim - 2;

    if (n_d_att > 0) {
	if ((s->dbl_att =
	     (double *)G_malloc(n_d_att * sizeof(double))) == NULL) {
	    if (n_dim > 2)
		G_free(s->dim);
	    G_free(s);
	    return (Site *) NULL;
	}
    }
    s->dbl_alloc = n_d_att;

    if (n_s_att > 0) {
	if ((s->str_att =
	     (char **)G_malloc(n_s_att * sizeof(char *))) == NULL) {
	    if (n_d_att > 0)
		G_free(s->dbl_att);
	    if (n_dim > 2)
		G_free(s->dim);
	    G_free(s);
	    return (Site *) NULL;
	}
	else
	    for (i = 0; i < n_s_att; ++i)
		if ((s->str_att[i] =
		     (char *)G_malloc(MAX_SITE_STRING * sizeof(char))) ==
		    NULL) {
		    while (--i)
			G_free(s->str_att[i]);
		    G_free(s->str_att);
		    if (n_d_att > 0)
			G_free(s->dbl_att);
		    if (n_dim > 2)
			G_free(s->dim);
		    G_free(s);
		    return (Site *) NULL;
		}
    }
    s->str_alloc = n_s_att;

    return s;
}

/*!
  \brief Writes a site to file open on fptr
*/
int G_oldsite_get(FILE * fptr, Site * s)
{
    return G__oldsite_get(fptr, s, G_projection());
}

/*!
  \brief Get site (old version)

  \return 0 on success,
  \return -1 on EOF,
  \return -2 on other fatal error or insufficient data,
  \return 1 on format mismatch (extra data)
*/
int G__oldsite_get(FILE * ptr, Site * s, int fmt)
{
    char sbuf[MAX_SITE_LEN], *buf, *last, *p1, *p2;
    char ebuf[128], nbuf[128];
    int n = 0, d = 0, c = 0, dim = 0, err = 0, tmp;

    buf = sbuf;

    if ((buf = fgets(sbuf, 1024, ptr)) == (char *)NULL)
	return EOF;

    while ((*buf == '#' || !isdigit(*buf)) && *buf != '-' && *buf != '+')
	if ((buf = fgets(sbuf, 1024, ptr)) == (char *)NULL)
	    return EOF;

    if (buf[strlen(buf) - 1] == '\n')
	buf[strlen(buf) - 1] = '\0';

    if (sscanf(buf, "%[^|]|%[^|]|%*[^\n]", ebuf, nbuf) < 2) {
	fprintf(stderr, "ERROR: ebuf %s nbuf %s\n", ebuf, nbuf);
	return -2;
    }

    if (!G_scan_northing(nbuf, &(s->north), fmt) ||
	!G_scan_easting(ebuf, &(s->east), fmt)) {
	fprintf(stderr, "ERROR: ebuf %s nbuf %s\n", ebuf, nbuf);
	return -2;
    }

    /* move pointer past easting and northing fields */
    if (NULL == (buf = strchr(buf, PIPE)))
	return -2;
    if (NULL == (buf = strchr(buf + 1, PIPE)))
	return -2;

    /* check for remaining dimensional fields */
    do {
	buf++;
	if (isnull(*buf))
	    return (FOUND_ALL(s, n, dim, c, d) ? 0 : -2);
	last = buf;
	if (dim < s->dim_alloc) {	/* should be more dims to read */
	    if (sscanf(buf, "%lf|", &(s->dim[dim++])) < 1)
		return -2;	/* no more dims, though expected */
	}
	else if (NULL != (p1 = strchr(buf, PIPE))) {
	    if (NULL == (p2 = strchr(buf, DQUOTE)))
		err = 1;	/* more dims, though none expected */
	    else if (strlen(p1) > strlen(p2))
		err = 1;	/* more dims, though none expected */
	}
    } while ((buf = strchr(buf, PIPE)) != NULL);
    buf = last;

    /* no more dimensions-now we parse attribute fields */
    while (!isnull(*buf)) {
	switch (*buf) {
	case '#':		/* category field */
	    if (n == 0) {
		switch (s->cattype) {
		case CELL_TYPE:
		    if (sscanf(buf, "#%d", &s->ccat) == 1)
			n++;
		    break;
		case FCELL_TYPE:
		    if (sscanf(buf, "#%f", &s->fcat) == 1)
			n++;
		    break;
		case DCELL_TYPE:
		    if (sscanf(buf, "#%lf", &s->dcat) == 1)
			n++;
		    break;
		default:
		    err = 1;	/* has cat, none expected */
		    break;
		}
	    }
	    else {
		err = 1;	/* extra cat */
	    }

	    /* move to beginning of next attribute */
	    if ((buf = next_att(buf)) == (char *)NULL)
		return (FOUND_ALL(s, n, dim, c, d) ? err : -2);
	    break;

	case '%':		/* decimal attribute */
	    if (d < s->dbl_alloc) {
		p1 = ++buf;
		errno = 0;
		s->dbl_att[d++] = strtod(buf, &p1);
		if (p1 == buf || errno == ERANGE) {
		    /* replace with:
		     * s->dbl_att[d - 1] = NAN
		     * when we add NULL attribute support
		     */
		    return -2;
		}
		/* err = 0; Make sure this is zeroed */
	    }
	    else {
		err = 1;	/* extra decimal */
	    }

	    if ((buf = next_att(buf)) == (char *)NULL) {
		return (FOUND_ALL(s, n, dim, c, d)) ? err : -2;
	    }
	    break;
	case '@':		/* string attribute */
	    if (isnull(*buf) || isnull(*(buf + 1)))
		return (FOUND_ALL(s, n, dim, c, d) ? err : -2);
	    else
		buf++;
	default:		/* defaults to string attribute */
	    /* allow both prefixed and unprefixed strings */
	    if (c < s->str_alloc) {
		if ((tmp = cleanse_string(buf)) > 0) {
		    strncpy(s->str_att[c++], buf, tmp);
		    buf += tmp;
		}
		else
		    return (FOUND_ALL(s, n, dim, c, d) ? err : -2);
	    }
	    if ((buf = next_att(buf)) == (char *)NULL) {
		return (FOUND_ALL(s, n, dim, c, d) ? err : -2);
	    }
	    break;
	}
    }

    return (FOUND_ALL(s, n, dim, c, d) ? err : -2);
}

/*!
  \brief Tries to guess the format of a sites list (old version)

  The dimensionality, the presence/type of a category, and the number
  of string and decimal attributes) by reading the first record in the
  file.

  \return 0 on success,
  \return -1 on EOF,
  \return -2 for other error
*/
int G_oldsite_describe(FILE * ptr, int *dims, int *cat, int *strs, int *dbls)
{
    char sbuf[MAX_SITE_LEN], *buf;
    char ebuf[128], nbuf[128];
    int err;
    int itmp;
    float ftmp;

    if (G_ftell(ptr) != 0L) {
	G_warning(_("G_oldsite_describe() must be called "
		    "immediately after G_fopen_sites_old()."));
	return -2;
    }

    *dims = *strs = *dbls = 0;
    *cat = -1;
    buf = sbuf;

    if ((buf = fgets(sbuf, 1024, ptr)) == (char *)NULL) {
	rewind(ptr);
	return EOF;
    }
    /* skip over comment & header lines */
    while ((*buf == '#' || !isdigit(*buf)) && *buf != '-' && *buf != '+')
	if ((buf = fgets(sbuf, 1024, ptr)) == (char *)NULL) {
	    rewind(ptr);
	    return EOF;
	}

    if (buf[strlen(buf) - 1] == '\n')
	buf[strlen(buf) - 1] = '\0';

    if ((err = sscanf(buf, "%[^|]|%[^|]|%*[^\n]", ebuf, nbuf)) < 2) {
	G_debug(1, "ebuf %s nbuf %s", ebuf, nbuf);
	rewind(ptr);
	return -2;
    }
    *dims = 2;

    /* move pointer past easting and northing fields */
    while (!ispipe(*buf) && !isnull(*buf))
	buf++;
    if (!isnull(*buf) && !isnull(*(buf + 1)))
	buf++;
    else {
	rewind(ptr);
	return -2;
    }
    while (!ispipe(*buf) && !isnull(*buf))
	buf++;
    if (!isnull(*buf) && !isnull(*(buf + 1)))
	buf++;
    else {
	rewind(ptr);
	return 0;
    }

    /* check for remaining dimensional fields */
    while (strchr(buf, PIPE) != (char *)NULL) {
	(*dims)++;
	while (!ispipe(*buf) && !isnull(*buf))
	    buf++;
	if (isnull(*buf) || isnull(*(buf + 1))) {
	    rewind(ptr);
	    return 0;
	}
	if (!isnull(*(buf + 1)))
	    buf++;
	else {
	    rewind(ptr);
	    return -2;
	}
    }

    /* no more dimensions-now we parse attribute fields */
    while (!isnull(*buf)) {
	switch (*buf) {
	case '#':		/* category field */
	    sscanf(buf, "#%s ", ebuf);
	    if (strstr(ebuf, ".") == NULL && sscanf(ebuf, "%d", &itmp) == 1)
		*cat = CELL_TYPE;
	    else if (strstr(ebuf, ".") != NULL &&
		     sscanf(ebuf, "%f", &ftmp) == 1)
		*cat = FCELL_TYPE;
	    else
		*cat = -1;

	    /* move to beginning of next attribute */
	    while (!isspace(*buf) && !isnull(*buf))
		buf++;
	    if (isnull(*buf) || isnull(*(buf + 1))) {
		rewind(ptr);
		return 0;
	    }
	    else
		buf++;
	    break;
	case '%':		/* decimal attribute */
	    (*dbls)++;
	    /* move to beginning of next attribute */
	    while (!isspace(*buf) && !isnull(*buf))
		buf++;
	    if (isnull(*buf) || isnull(*(buf + 1))) {
		rewind(ptr);
		return 0;
	    }
	    else
		buf++;
	    break;
	case '@':		/* string attribute */
	    if (isnull(*buf) || isnull(*(buf + 1))) {
		rewind(ptr);
		return 0;
	    }
	    else
		buf++;
	default:		/* defaults to string attribute */
	    /* allow both prefixed and unprefixed strings */
	    if ((err = cleanse_string(buf)) > 0) {
		(*strs)++;
		buf += err;
	    }

	    /* move to beginning of next attribute */
	    while (!isspace(*buf) && !isnull(*buf))
		buf++;
	    if (isnull(*buf) || isnull(*(buf + 1))) {
		rewind(ptr);
		return 0;
	    }
	    else
		buf++;
	    break;
	}
    }

    rewind(ptr);
    return 0;
}

/*!
  \brief Test if site is in region

  \return 1 if site is contained within region
  \return 0 otherwise
*/
int G_site_in_region(const Site * site, const struct Cell_head *region)
{
    /* northwest corner is in region, southeast corner is not. */
    double e_ing;

    e_ing = G_adjust_easting(site->east, region);
    if (e_ing >= region->west &&
	e_ing < region->east &&
	site->north <= region->north && site->north > region->south)
	return 1;

    return 0;
}

int cleanse_string(char *buf)
{
    char *stop, *p, *p2;

    p = buf;

    /*
     * get rid of any SPACEs at beginning while ( !isspace(*buf) && *buf !=
     * (char) NULL) buf++; if (*buf == (char) NULL) return -1;
     */

    /* find where this string terminates */
    if (*buf != DQUOTE) {	/* if no DQUOTEs, */
	stop = strchr(buf, SPACE);	/* then SPACE separates */
	if (stop == (char *)NULL)
	    return strlen(buf);
	else
	    return (int)(stop - buf);
    }
    else {			/* otherwise string is in DQUOTEs */
	/* but we must skip over escaped */
	/* (BSLASHed) DQUOTEs */
	if (*p == DQUOTE) {
	    while (*p != '\0') {	/* get rid of first DQUOTE */
		*p = *(p + 1);
		p++;
	    }
	    p = buf;
	    stop = strchr(p + 1, DQUOTE);
	    while (*(stop - 1) == BSLASH)
		stop = strchr(++stop, DQUOTE);
	}
    }
    /* remove backslashes between buf and stop */
    p = buf;
    while ((p = strchr(p, BSLASH)) != (char *)NULL && p <= stop) {
	p2 = p + 1;
	if (*p2 != '\0' && (*p2 == DQUOTE || *p2 == BSLASH)) {
	    while (*p != '\0') {
		*p = *(p + 1);
		p++;
	    }
	    stop--;
	}
	p = p2;
    }
    return (int)(stop - buf);
}

char *next_att(const char *buf)
{
    while (!isspace(*buf) && !isnull(*buf))
	buf++;
    if (isnull(*buf) || isnull(*(buf + 1)))
	return NULL;
    else
	while (isspace(*(buf + 1)) && !isnull(*(buf + 1)))
	    buf++;
    buf++;
    return (char *)buf;
}

int G_oldsite_s_cmp(const void *a, const void *b)
/* qsort() comparison function for sorting an array of
   site structures by first decimal attribute. */
{
    return strcmp((*(char **)((*(Site **) a)->str_att)),
		  (*(char **)((*(Site **) b)->str_att)));
}

/*!
  \brief Open site list (old version)

  Opens the existing site list file 'name' in the 'mapset'.

  \param name 
  \param mapset mapset (empty for search path)

  \return pointer to FILE
*/
FILE *G_oldsites_open_old(const char *name, const char *mapset)
{
    return G_fopen_old("site_lists", name, mapset);
}
