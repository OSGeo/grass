/**********************************************************************
 *
 * Code in this file works with category files.  There are two formats:
 * Pre 3.0 direct category encoding form:
 * 
 *    2 categories
 *    Map Title
 *    Elevation: 1000.00 to 1005.00 feet
 *    Elevation: 1005.00 to 1010.00 feet
 *    Elevation: 1010.00 to 1015.00 feet
 *
 * 3.0 format
 * 
 *    # 2 categories
 *    Map Title
 *    Elevation: $1.2 to $2.2 feet       ## Format Statement
 *    5.0 1000 5.0 1005                  ## Coefficients
 *
 * The coefficient line can be followed by explicit category labels
 * which override the format label generation.
 *    0:no data
 *    2:   .
 *    5:   .                             ## explicit category labels
 *    7:   .
 * explicit labels can be also of the form:
 *    5.5:5:9 label description
 *    or
 *    15:30  label description
 *
 * In the format line
 *   $1 refers to the value num*5.0+1000 (ie, using the first 2 coefficients)
 *   $2 refers to the value num*5.0+1005 (ie, using the last 2 coefficients)
 *
 *   $1.2 will print $1 with 2 decimal places.
 *
 * Also, the form $?xxx$yyy$ translates into yyy if the category is 1, xxx 
 * otherwise. The $yyy$ is optional. Thus
 *
 *   $1 meter$?s
 *
 * will become: 1 meter (for category 1)
 *              2 meters (for category 2), etc.
 *
 * The format and coefficients above would be used to generate the
 * following statement in creation of the format appropriate category
 * string for category "num":
 *
 *   sprintf(buff,"Elevation: %.2f to %.2f feet", num*5.0+1000, num*5.0*1005)
 *
 * Note: while both the format and coefficent lins must be present
 *       a blank line for the fmt will effectively suppress automatic
 *       label generation
 *
 * Note: quant rules of Categories structures are heavily dependant
 * on the fact that rules are stored in the same order they are entered.
 * since i-th rule and i-th label are entered at the same time, we
 * know that i-th rule maps fp range to i, thus we know for sure
 * that cats.labels[i] corresponds to i-th quant rule
 * 
 **********************************************************************
 *
 *  G_read_[raster]_cats (name, mapset, pcats)
 *      char *name                   name of cell file
 *      char *mapset                 mapset that cell file belongs to
 *      struct Categories *pcats     structure to hold category info
 *
 *  Reads the category information associated with cell file "name"
 *  in mapset "mapset" into the structure "pcats".
 *
 *  returns:    0  if successful
 *             -1  on fail
 *
 *  note:   a warning message is printed if the file is
 *          "missing" or "invalid".
 **********************************************************************
 *
 *  G_copy_raster_cats (pcats_to, pcats_from)
 *      struct Categories *pcats_to
 *      const struct Categories *pcats_from
 *
 *  Allocates NEW space for quant rules and labels and copies
 *  all info from "from" cats to "to" cats
 *
 *  returns:    0  if successful
 *             -1  on fail
 *
 **********************************************************************
 *
 *  G_read_vector_cats (name, mapset, pcats)
 *      char *name                   name of vector map
 *      char *mapset                 mapset that vector map belongs to
 *      struct Categories *pcats     structure to hold category info
 *
 *
 *  returns:    0  if successful
 *             -1  on fail
 *
 *  note:   a warning message is printed if the file is
 *          "missing" or "invalid".
 **********************************************************************
 * Returns pointer to a string describing category.
 **********************************************************************
 *
 *  G_number_of_cats(name, mapset)
 *  returns the largest category number in the map.
 *  -1 on error
 *  WARING: do not use for fp maps!
 **********************************************************************
 *
 * char *
 * G_get_c/f/d_raster_cat (num, pcats)
 *      [F/D]CELL *val               pointer to cell value 
 *      struct Categories *pcats     structure to hold category info
 *
 * Returns pointer to a string describing category.
 *
 **********************************************************************
 *
 * char *
 * G_get_raster_cat (val, pcats, data_type)
 *      void *val               pointer to cell value 
 *      struct Categories *pcats     structure to hold category info
 *      RASTER_MAP_TYPE data_type    type of raster cell
 *
 * Returns pointer to a string describing category.
 *
 **********************************************************************
 *
 * char *
 * G_get_ith_c/f/d_raster_cat (pcats, i, rast1, rast2)
 *      [F/D]CELL *rast1, *rast2      cat range
 *      struct Categories *pcats     structure to hold category info
 *
 * Returns pointer to a string describing category.
 *
 **********************************************************************
 *
 * int
 * G_number_of_raster_cats (pcats)
 *      struct Categories *pcats     structure to hold category info
 *
 * Returns pcats->ncats number of labels
 *
 **********************************************************************
 *
 * char *
 * G_get_ith_raster_cat (pcats, i, rast1, rast2, data_type)
 *      void *rast1, *rast2      cat range
 *      struct Categories *pcats     structure to hold category info
 *      RASTER_MAP_TYPE data_type    type of raster cell
 *
 * Returns pointer to a string describing category.
 *
 **********************************************************************
 *
 * char *
 * G_get_[raster]_cats_title (pcats)
 *      struct Categories *pcats     structure to hold category info
 *
 * Returns pointer to a string with title
 *
 **********************************************************************
 *
 * G_init_cats (ncats, title, pcats)
 *      CELL ncats                   number of categories
 *      char *title                  cell title
 *      struct Categories *pcats     structure to hold category info
 *
 * Initializes the cats structure for subsequent calls to G_set_cat()
 **********************************************************************
 *
 * G_unmark_raster_cats (pcats)
 *      struct Categories *pcats     structure to hold category info
 *
 * initialize cats.marks: the statistics of how many values of map
 * have each label
 *
 **********************************************************************
 *
 * G_rewind_raster_cats (pcats)
 *      struct Categories *pcats     structure to hold category info
 *
 * after calll to this function G_get_next_marked_raster_cat() returns
 * rhe first marked cat label.
 *
 **********************************************************************
 *
 * char* G_get_next_marked_raster_cat(pcats, rast1, rast2, stats, data_type)
 *    struct Categories *pcats     structure to hold category info
 *    void *rast1, *rast2;         pointers to raster range
 *    long *stats;
 *    RASTER_MAP_TYPE  data_type
 *
 *    returns the next marked label.
 *    NULL if none found
 *
 **********************************************************************
 *
 * char* G_get_next_marked_f/d/craster_cat(pcats, rast1, rast2, stats)
 *    struct Categories *pcats     structure to hold category info
 *    [D/F]CELL *rast1, *rast2;    pointers to raster range
 *    long *stats;
 *
 *    returns the next marked label.
 *    NULL if none found
 *
 **********************************************************************
 *
 * G_mark_raster_cats (rast_row, ncols, pcats, data_type)
 *      void *raster_row;            raster row to update stats
 *      struct Categories *pcats     structure to hold category info
 *      RASTER_MAP_TYPE data_type;
 * Finds the index of label for each raster cell in a row, and
 * increases pcats->marks[index]
 * Note: if there are no explicit cats: only rules for cats, no 
 * marking is done.
 *
 **********************************************************************
 *
 * G_mark_c/d/f_raster_cats (rast_row, ncols, pcats)
 *      int ncols;
 *      [D?F]CELL *raster_row;            raster row to update stats
 *      struct Categories *pcats     structure to hold category info
 *
 * Finds the index of label for each raster cell in a row, and
 * increases pcats->marks[index]
 *
 **********************************************************************
 *
 * G_init_raster_cats (title, pcats)
 *      char *title                  cell title
 *      struct Categories *pcats     structure to hold category info
 *
 * Initializes the cats structure for subsequent calls to G_set_cat()
 *
 **********************************************************************
 *
 * G_set_[raster]_cats_fmt (fmt, m1, a1, m2, a2, pcats)
 *      char *fmt                    user form of the equation format
 *      float m1,a1,m2,a2            coefficients
 *      struct Categories *pcats     structure to hold category info
 *
 * configures the cats structure for the equation. Must be called
 * after G_init_cats().
 *
 **********************************************************************
 *
 * G_set_[raster]_cats_title (title, pcats)
 *      char *title                  cell file title
 *      struct Categories *pcats     structure holding category info
 *
 * Store title as cell file in cats structure
 * Returns nothing.
 *
 **********************************************************************
 *
 * G_set_cat (num, label, pcats)
 *      CELL num                     category number
 *      char *label                  category label
 *      struct Categories *pcats     structure to hold category info
 *
 * Adds the string buff to represent category "num" in category structure
 * pcats.
 *
 * Returns: 0 is cat is null value -1 too many cats, 1 ok.
 *
 **********************************************************************
 *
 * G_set_[f/d/c]_raster_cat (&val1, &val2, label, pcats)
 *      [D/F]CELL *val1, *val2;      pointers to raster values
 *      char *label                  category label
 *      struct Categories *pcats     structure to hold category info
 *
 * Adds the label for range val1 through val2 in category structure
 * pcats.
 *
 * Returns: 0 if cat is null value -1 too many cats, 1 ok.
 *
 **********************************************************************
 *
 * G_set_raster_cat (val1, val2, label, pcats, data_type)
 *      void *val1, *val2;           pointers to raster values
 *      char *label                  category label
 *      struct Categories *pcats     structure to hold category info
 *      RASTER_MAP_TYPE data_type    type of raster cell
 *
 * Adds the label for range val1 through val2 in category structure
 * pcats.
 *
 * Returns: 0 if cat is null value -1 too many cats, 1 ok.
 *
 **********************************************************************
 *
 *  G_write_[raster]_cats (name, pcats)
 *      char *name                   name of cell file
 *      struct Categories *pcats     structure holding category info
 *
 *  Writes the category information associated with cell file "name"
 *  into current mapset from the structure "pcats".
 *
 *   returns:    1  if successful
 *              -1  on fail
 **********************************************************************
 *
 *  G_write_vector_cats (name, pcats)
 *      char *name                   name of vector map
 *      struct Categories *pcats     structure holding category info
 *
 *  Writes the category information associated with vector map "name"
 *  into current mapset from the structure "pcats".
 *
 *   returns:    1  if successful
 *              -1  on fail
 **********************************************************************
 *
 * G_free_[raster]_cats (pcats)
 *      struct Categories *pcats     structure holding category info
 *
 * Releases memory allocated for the cats structure
 **********************************************************************/
  
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static int get_cond ( char **, char *, DCELL);
static int get_fmt ( char **, char *, int *);
static int cmp (const void *, const void *);


/*!
 * \brief read raster category file
 *
 * The category file for raster map
 * <b>name</b> in <b>mapset</b> is read into the <b>cats</b> structure. If
 * there is an error reading the category file, a diagnostic message is printed
 * and -1 is returned. Otherwise, 0 is returned.
 *
 *  \param name
 *  \param mapset
 *  \param cats
 *  \return int
 */

int G_read_cats (
    const char *name ,
    const char *mapset ,
    struct Categories *pcats )
{
    return G_read_raster_cats (name, mapset, pcats);
}
    

/*!
 * \brief 
 *
 * Is the same as existing G_read_cats()
 *
 *  \param name
 *  \param mapset
 *  \param pcats
 *  \return int
 */

int G_read_raster_cats (
    const char *name ,
    const char *mapset ,
    struct Categories *pcats )
{
    char *type;

    switch (G__read_cats ("cats", name, mapset, pcats, 1))
    {
    case -2:
	    type = "missing";
	    break;
    case -1:
	    type = "invalid";
	    break;
    default:
	    return 0;
    }

    G_warning (_("category support for [%s] in mapset [%s] %s"),
		    name, mapset, type);
    return -1;
}


/*!
 * \brief read vector category file
 *
 * The category file for vector map
 * <b>name</b> in <b>mapset</b> is read into the <b>cats</b> structure. If
 * there is an error reading the category file, a diagnostic message is printed
 * and -1 is returned. Otherwise, 0 is returned.
 *
 *  \param name
 *  \param mapset
 *  \param cats
 *  \return int
 */

int G_read_vector_cats (
    const char *name ,
    const char *mapset ,
    struct Categories *pcats )
{
    char *type;

    switch (G__read_cats ("dig_cats", name, mapset, pcats, 1))
    {
    case -2:
	    type = "missing";
	    break;
    case -1:
	    type = "invalid";
	    break;
    default:
	    return 0;
    }

    G_warning (_("category support for vector map [%s] in mapset [%s] %s"),
		    name, mapset, type);
    return -1;
}

CELL G_number_of_cats (
    const char *name ,
    const char *mapset)
{
    struct Range range;
    CELL min, max;

    /* return the max category number */
    if(G_read_range(name, mapset, &range) < 0)
	return -1;
    G_get_range_min_max(&range, &min, &max);
    if(G_is_c_null_value(&max)) max = 0;
    return max;
}

CELL G__read_cats (
    const char *element,
    const char *name ,
    const char *mapset ,
    struct Categories *pcats,
    int full)
{
    FILE *fd ;
    char buff[1024] ;
    CELL cat;
    DCELL val1, val2;
    int old=0, fp_map;
    long num=-1;


    if(strncmp(element, "dig", 3)== 0)
       fp_map = 0;
    else
       fp_map = G_raster_map_is_fp(name, mapset);
    
    if (!(fd = G_fopen_old (element, name, mapset)))
	return -2 ;

/* Read the number of categories */
    if (G_getl(buff,sizeof buff,fd) == 0)
	goto error;

    if (sscanf ( buff, "# %ld"   , &num) == 1)
	old = 0;
    else if (sscanf ( buff, "%ld"   , &num) == 1)
	old = 1;

    if (!full)
    {
	fclose (fd);
	if(num < 0) return 0; /* coorect */
	return (CELL) num;
    }

/* Read the title for the file */
    if (G_getl(buff,sizeof buff,fd) == 0)
	goto error;
    G_strip (buff);
/*    G_ascii_check(buff) ; */

    G_init_raster_cats (buff, pcats);
    if(num >= 0) pcats->num = num;

    if (!old)
    {
	char fmt[256];
	float m1,a1,m2,a2;
	if (G_getl(fmt,sizeof fmt,fd) == 0)
		goto error;
/* next line contains equation coefficients */
	if (G_getl(buff,sizeof buff,fd) == 0)
		goto error;
	if(sscanf(buff, "%f %f %f %f", &m1, &a1, &m2, &a2) != 4)
		goto error;
	G_set_raster_cats_fmt (fmt, m1, a1, m2, a2, pcats);
    }

/* Read all category names */
    for (cat=0;;cat++) 
    {
	char label[1024];
	if (G_getl(buff, sizeof buff, fd) == 0)
	    break;
	if (old)
	    G_set_cat (cat, buff, pcats) ;
	else
	{
	    *label = 0;
	    if (sscanf (buff, "%1s", label) != 1)
		continue;
	    if(*label == '#')
		continue;
	    *label = 0;
	    /* for fp maps try to read a range of data */
            if (fp_map 
		&& sscanf (buff, "%lf:%lf:%[^\n]", &val1, &val2, label) == 3)
	        G_set_raster_cat (&val1, &val2, label, pcats, DCELL_TYPE);
	    else if (sscanf (buff, "%d:%[^\n]", &cat, label) >= 1)
	        G_set_raster_cat (&cat, &cat, label, pcats, CELL_TYPE);
            else if (sscanf (buff, "%lf:%[^\n]", &val1, label) >= 1)
	        G_set_raster_cat (&val1, &val1, label, pcats, DCELL_TYPE);
            else goto error;
	}
    }

    fclose (fd);
    return 0 ;
error:
    fclose (fd);
    return -1 ;
}


/*!
 * \brief  get title from category structure struct
 *
 * Map layers store a one-line title in the category structure
 * as well. This routine returns a pointer to the title contained in the
 * <b>cats</b> structure.  A legal pointer is always returned. If the map
 * layer does not have a title, then a pointer to the empty string "" is
 * returned.
 *
 *  \param cats
 *  \return char * 
 */

char *G_get_cats_title (const struct Categories *pcats)
{
   return G_get_raster_cats_title (pcats);
}


/*!
 * \brief get raster cats title
 *
 * Returns pointer to a string with title.
 *
 *  \param pcats
 *  \return char * 
 */

char *G_get_raster_cats_title (const struct Categories *pcats)
{
    static char *none = "";
    return pcats->title ? pcats->title : none;
}


/*!
 * \brief get a category label
 *
 * This routine looks up category <b>n</b> in the <b>cats</b>
 * structure and returns a pointer to a string which is the label for the
 * category. A legal pointer is always returned. If the category does not exist
 * in <b>cats,</b> then a pointer to the empty string "" is returned.
 * <b>Warning.</b> The pointer that is returned points to a hidden static
 * buffer. Successive calls to G_get_cat( ) overwrite this buffer.
 *
 *  \param n
 *  \param cats
 *  \return char * 
 */

char *G_get_cat (CELL num, struct Categories *pcats)
{
    return G_get_c_raster_cat(&num, pcats);
}


/*!
 * \brief 
 *
 * given a CELL value <em>val</em> Returns pointer to a string describing
 * category.
 *
 *  \param val
 *  \param pcats
 *  \return char * 
 */

char *G_get_c_raster_cat ( CELL *rast, struct Categories *pcats)
{
    return G_get_raster_cat(rast, pcats, CELL_TYPE);
}


/*!
 * \brief 
 *
 * given a FCELL value <em>val</em> Returns pointer to a string
 * describing category.
 *
 *  \param val
 *  \param pcats
 *  \return char * 
 */

char *G_get_f_raster_cat (
    FCELL *rast,
    struct Categories *pcats )
{
    return G_get_raster_cat(rast, pcats, FCELL_TYPE);
}


/*!
 * \brief 
 *
 * given a DCELL value <em>val</em> Returns pointer to a string
 * describing category.
 *
 *  \param val
 *  \param pcats
 *  \return char * 
 */

char *G_get_d_raster_cat (
    DCELL *rast,
    struct Categories *pcats)
{
    return G_get_raster_cat(rast, pcats, DCELL_TYPE);
}


/*!
 * \brief 
 *
 *  given a raster value <em>val</em> of type <em>data_type</em> Returns pointer to a string
 *  describing category.
 *
 *  \param val
 *  \param pcats
 *  \param data_type
 *  \return char * 
 */

char *G_get_raster_cat (
    void *rast,
    struct Categories *pcats ,
    RASTER_MAP_TYPE data_type)
{
    static char label[1024] ;
    char *f, *l, *v;
    CELL i;
    DCELL val;
    float a[2];
    char fmt[30], value_str[30];

    if(G_is_null_value(rast, data_type)) 
    {
        sprintf(label, "no data");
        return label;
    }

/* first search the list of labels */
    *label = 0;
    val = G_get_raster_value_d(rast, data_type);
    i = G_quant_get_cell_value(&pcats->q, val);
    /* DEBUG fprintf (stderr, "val %lf found i %d\n", val, i);*/
    if(!G_is_c_null_value(&i) && i < pcats->ncats)
    {
         if(pcats->labels[i] != NULL) return pcats->labels[i];
     	 return label;
    }

/* generate the label */
    if ((f = pcats->fmt) == NULL)
	return label;

    a[0] = (float)val*pcats->m1+pcats->a1 ;
    a[1] = (float)val*pcats->m2+pcats->a2 ;
    l = label;
    while (*f)
    {
	if (*f == '$')
	{
	    f++;
	    if (*f == '$')
		*l++ = *f++;
	    else if (*f == '?')
	    {
		f++;
		get_cond (&f, v = value_str, val);
		while (*v)
		    *l++ = *v++;
	    }
	    else if (get_fmt (&f, fmt, &i))
	    {
		sprintf (v = value_str, fmt, a[i]);
		while (*v)
		    *l++ = *v++;
	    }
	    else
		*l++ = '$';
	}
	else
	{
	    *l++ = *f++;
	}
    }
    *l = 0;
    return label;
}


/*!
 * \brief 
 *
 * Sets marks
 * for all categories to 0. This initializes Categories structure for subsequest
 * calls to G_mark_raster_cats (rast_row,...) for each row of data, where
 * non-zero mark for i-th label means that some of the cells in rast_row are
 * labeled with i-th label and fall into i-th data range.
 * These marks help determine from the Categories structure which labels were
 * used and which weren't.
 *
 *  \param pcats
 *  \return int
 */

int G_unmark_raster_cats (
      struct Categories *pcats)    /* structure to hold category info */
{
      register int i;
      for(i=0; i<pcats->ncats; i++)
	 pcats->marks[i] = 0;
      return 0;
}


/*!
 * \brief 
 *
 * Looks up the category label for each raster value in
 * the <em>rast_row</em> and updates the marks for labels found.
 * NOTE: non-zero mark for i-th label stores the number of of raster cells read
 * so far which are labeled with i-th label and fall into i-th data range.
 *
 *  \param rast_row
 *  \param ncols
 *  \param pcats
 *  \return int
 */

int G_mark_c_raster_cats (
      CELL *rast_row,            /* raster row to update stats */
      int ncols,
      struct Categories *pcats)    /* structure to hold category info */
{
      G_mark_raster_cats (rast_row, ncols, pcats, CELL_TYPE);
      return 0;
}


/*!
 * \brief 
 *
 * Looks up the category label for each raster value in
 * the <em>rast_row</em> and updates the marks for labels found.
 * NOTE: non-zero mark for i-th label stores the number of of raster cells read
 * so far which are labeled with i-th label and fall into i-th data range.
 *
 *  \param rast_row
 *  \param ncols
 *  \param pcats
 *  \return int
 */

int G_mark_f_raster_cats (
      FCELL *rast_row,            /* raster row to update stats */
      int ncols,
      struct Categories *pcats)    /* structure to hold category info */
{
      G_mark_raster_cats (rast_row, ncols, pcats, FCELL_TYPE);
      return 0;
}


/*!
 * \brief 
 *
 * Looks up the category label for each raster value in
 * the <em>rast_row</em> and updates the marks for labels found.
 * NOTE: non-zero mark for i-th label stores the number of of raster cells read
 * so far which are labeled with i-th label and fall into i-th data range.
 *
 *  \param rast_row
 *  \param ncols
 *  \param pcats
 *  \return int
 */

int G_mark_d_raster_cats (
      DCELL *rast_row,            /* raster row to update stats */
      int ncols,
      struct Categories *pcats)    /* structure to hold category info */
{
      G_mark_raster_cats (rast_row, ncols, pcats, DCELL_TYPE);
      return 0;
}


/*!
 * \brief 
 *
 * Looks up the category
 * label for each raster value in the <em>rast_row</em> (row of raster cell value)
 * and updates the marks for labels found.
 * NOTE: non-zero mark for i-th label stores the number of of raster cells read
 * so far which are labeled with i-th label and fall into i-th data range.
 *
 *  \param rast_row
 *  \param ncols
 *  \param pcats
 *  \param data_type
 *  \return int
 */

int G_mark_raster_cats (
      void *rast_row,            /* raster row to update stats */
      int ncols,
      struct Categories *pcats,    /* structure to hold category info */
      RASTER_MAP_TYPE data_type)
{
   CELL i;

   while(ncols-- > 0)
   {
      i = G_quant_get_cell_value(&pcats->q, 
		 G_get_raster_value_d(rast_row, data_type));
      if(G_is_c_null_value(&i)) continue;
      if(i> pcats->ncats) return -1;
      pcats->marks[i]++;
      rast_row = G_incr_void_ptr(rast_row, G_raster_size(data_type));
   }
   return 1;
}


/*!
 * \brief 
 *
 * after call to
 * this function G_get_next_marked_raster_cat() returns the first marked
 * cat label.
 *
 *  \param pcats
 *  \return int
 */

int G_rewind_raster_cats (struct Categories *pcats)
{
    pcats->last_marked_rule = -1;
      return 0;
}

char *G_get_next_marked_d_raster_cat(
   struct Categories *pcats,    /* structure to hold category info */
   DCELL *rast1,DCELL *rast2,        /* pointers to raster range */
   long *count)
{
    char *descr=NULL;
    int found, i;

    found = 0;
    /* pcats->ncats should be == G_quant_nof_rules(&pcats->q) */
    /* DEBUG
    fprintf (stderr, "last marked %d nrules %d\n", pcats->last_marked_rule, G_quant_nof_rules(&pcats->q));
    */
    for (i = pcats->last_marked_rule + 1; i < G_quant_nof_rules(&pcats->q); i++)
    {
	descr = G_get_ith_d_raster_cat(pcats, i, rast1, rast2);
	/* DEBUG fprintf (stderr, "%d %d\n", i, pcats->marks[i]); */
	if(pcats->marks[i]) 
	{
	   found = 1;
	   break;
        }
    }

    if(!found) return NULL;

    *count = pcats->marks[i];
    pcats->last_marked_rule = i;
    return descr;
}

char *G_get_next_marked_c_raster_cat(
   struct Categories *pcats,    /* structure to hold category info */
   CELL *rast1,CELL *rast2,        /* pointers to raster range */
   long *count)
{
   return G_get_next_marked_raster_cat(pcats, rast1, rast2, count, CELL_TYPE);
}

char *G_get_next_marked_f_raster_cat(
   struct Categories *pcats,    /* structure to hold category info */
   FCELL *rast1,FCELL *rast2,        /* pointers to raster range */
   long *count)
{
   return G_get_next_marked_raster_cat(pcats, rast1, rast2, count, FCELL_TYPE);
}

char *G_get_next_marked_raster_cat(
   struct Categories *pcats,    /* structure to hold category info */
   void *rast1,void *rast2,        /* pointers to raster range */
   long *count,
   RASTER_MAP_TYPE data_type)
{
    DCELL val1, val2;
    char *lab;

    lab = G_get_next_marked_d_raster_cat(pcats, &val1, &val2, count);
    G_set_raster_value_d(rast1, val1, data_type);
    G_set_raster_value_d(rast2, val2, data_type);
    return lab;
}

static int get_fmt ( char **f, char *fmt, int *i)
{
    char *ff;

    ff = *f;
    if (*ff == 0) return 0;
    if (*ff == '$')
    {
	*f = ff+1;
	return 0;
    }
    switch (*ff++)
    {
    case '1': *i = 0; break;
    case '2': *i = 1; break;
    default: return 0;
    }
    *fmt++ = '%';
    *fmt++ = '.';
    if (*ff++ != '.')
    {
	*f = ff-1;
	*fmt++ = '0';
	*fmt++ = 'f';
	*fmt = 0;
	return 1;
    }
    *fmt = '0';
    while (*ff >= '0' && *ff <= '9')
	*fmt++ = *ff++;
    *fmt++ = 'f';
    *fmt = 0;
    *f = ff;
    return 1;
}

static int get_cond ( char **f, char *value, DCELL val)
{
    char *ff;

    ff = *f;
    if (val == 1.)
    {
	while (*ff)
	    if (*ff++ == '$')
		break;
    }

    while (*ff)
	if (*ff == '$')
	{
	    ff++;
	    break;
	}
	else
	    *value++ = *ff++;

    if (val != 1.)
    {
	while (*ff)
	    if (*ff++ == '$')
		break;
    }
    *value = 0;
    *f = ff;

    return 0;
}


/*!
 * \brief set a category label
 *
 * The <b>label</b> is copied into the <b>cats</b> structure
 * for category <b>n.</b>
 *
 *  \param n
 *  \param label
 *  \param cats
 *  \return int
 */

int G_set_cat (
    CELL num ,
    char *label ,
    struct Categories *pcats )
{
    CELL tmp=num;
    return G_set_c_raster_cat(&tmp, &tmp, label, pcats);
}


/*!
 * \brief 
 *
 * Adds the label for range <em>rast1</em> through <em>rast2</em> in
 * category structure <em>pcats</em>.
 *
 *  \param rast1
 *  \param rast2
 *  \param pcats
 *  \return int
 */

int G_set_c_raster_cat (
    CELL *rast1,CELL *rast2,
    char *label ,
    struct Categories *pcats )
{
    return G_set_raster_cat (rast1, rast2, label, pcats, CELL_TYPE);
}


/*!
 * \brief 
 *
 * Adds the label for range <em>rast1</em> through <em>rast2</em>
 * in category structure <em>pcats</em>.
 *
 *  \param rast1
 *  \param rast2
 *  \param pcats
 *  \return int
 */

int G_set_f_raster_cat (
    FCELL *rast1,FCELL *rast2,
    char *label ,
    struct Categories *pcats)
{
    return G_set_raster_cat (rast1, rast2, label, pcats, FCELL_TYPE);
}


/*!
 * \brief 
 *
 * Adds the label for range <em>rast1</em> through <em>rast2</em>
 * in category structure <em>pcats</em>.
 *
 *  \param rast1
 *  \param rast2
 *  \param pcats
 *  \return int
 */

int G_set_d_raster_cat (
    DCELL *rast1,DCELL *rast2,
    char *label ,
    struct Categories *pcats )
{
    long len;
    DCELL dtmp1, dtmp2;
    int i;
    char *descr;

/* DEBUG fprintf(stderr,"G_set_d_raster_cat(rast1 = %p,rast2 = %p,label = '%s',pcats = %p)\n",
rast1,rast2,label,pcats); */
    if(G_is_d_null_value(rast1)) return 0;
    if(G_is_d_null_value(rast2)) return 0;
    /* DEBUG fprintf (stderr, "G_set_d_raster_cat(): adding quant rule: %f %f %d %d\n", *rast1, *rast2, pcats->ncats, pcats->ncats); */
    /* the set_cat() functions are used in many places to reset the labels
       for the range (or cat) with existing label. In this case we don't
       want to store both rules with identical range even though the result
       of get_cat() will be correct, since it will use rule added later.
       we don't want to overuse memory and we don't want rules which are
       not used to be writen out in cats file. So we first look if
       the label for this range has been sen, and if it has, overwrite it */

    for(i=0; i< pcats->ncats; i++)
    {
       descr = G_get_ith_d_raster_cat(pcats, i, &dtmp1, &dtmp2);
       if((dtmp1==*rast1 && dtmp2==*rast2) 
	||(dtmp1==*rast2 && dtmp2==*rast1))
       {
	  if(pcats->labels[i] != NULL) G_free(pcats->labels[i]);
	  pcats->labels[i] = G_store(label);
          G_newlines_to_spaces (pcats->labels[i]);
          G_strip (pcats->labels[i]);
	  return 1;
       }
    }
    /* when rule for this range does not exist */
    /* DEBUG fprintf (stderr, "G_set_d_raster_cat(): New rule: adding %d %p\n", i, pcats->labels); */
    G_quant_add_rule(&pcats->q, *rast1, *rast2, pcats->ncats, pcats->ncats);
    pcats->ncats++;
    if(pcats->nalloc < pcats->ncats)
    {
    /* DEBUG fprintf (stderr, "G_set_d_raster_cat(): need more space nalloc = %d ncats = %d\n", pcats->nalloc,pcats->ncats); */
	len = (pcats->nalloc  + 256) * sizeof(char *);
    /* DEBUG fprintf (stderr, "G_set_d_raster_cat(): allocating %d labels(%d)\n", pcats->nalloc + 256,(int)len); */
	if (len != (int) len) /* make sure len doesn't overflow int */
	{
	   pcats->ncats--;
	   return -1;
	}
/* DEBUG fprintf(stderr,"G_set_d_raster_cat(): pcats->nalloc = %d, pcats->labels = (%p), len = %d\n",pcats->nalloc,pcats->labels,(int)len); */
	if(pcats->nalloc) {
/* DEBUG fprintf(stderr,"G_set_d_raster_cat(): Realloc-ing pcats->labels (%p)\n",pcats->labels); */
	   pcats->labels = (char**) G_realloc((char*) pcats->labels, (int) len);
        } else {
/* DEBUG fprintf(stderr,"G_set_d_raster_cat(): alloc-ing new labels pointer array\n"); */
	   pcats->labels = (char**) G_malloc((int) len);
	}
/* fflush(stderr); */
    /* DEBUG fprintf (stderr, "G_set_d_raster_cats(): allocating %d marks(%d)\n", pcats->nalloc + 256,(int)len); */
	len = (pcats->nalloc  + 256) * sizeof(int);
	if (len != (int) len) /* make sure len doesn't overflow int */
	{
	   pcats->ncats--;
	   return -1;
        }
	if(pcats->nalloc)
	   pcats->marks = (int*) G_realloc((char *) pcats->marks, (int) len);
        else
	   pcats->marks = (int*) G_malloc((int) len);
	pcats->nalloc += 256;
    }
    /* DEBUG fprintf(stderr,"G_set_d_raster_cats(): store new label\n"); */
    pcats->labels[pcats->ncats - 1] = G_store(label) ;
    G_newlines_to_spaces (pcats->labels[pcats->ncats - 1]);
    G_strip (pcats->labels[pcats->ncats - 1]);
    /* DEBUG
    fprintf (stderr, "%d %s\n", pcats->ncats - 1, pcats->labels[pcats->ncats - 1]);
    */
    /* updates cats.num = max cat values. This is really just used in old
       raster programs, and I am doing it for backwards cmpatibility (Olga) */
    if ((CELL) *rast1 > pcats->num)
		pcats->num = (CELL) *rast1;
    if ((CELL) *rast2 > pcats->num)
		pcats->num = (CELL) *rast2;
/* DEBUG fprintf(stderr,"G_set_d_raster_cat(): done\n"); */
/* DEBUG fflush(stderr); */
    return 1;
}


/*!
 * \brief 
 *
 * Adds the label for range <em>rast1</em> through <em>rast2</em> in category structure <em>pcats</em>.
 *
 *  \param rast1
 *  \param rast2
 *  \param pcats
 *  \param data_type
 *  \return int
 */

int G_set_raster_cat (
    void *rast1,void *rast2,
    char *label ,
    struct Categories *pcats ,
    RASTER_MAP_TYPE data_type)
{
    DCELL val1, val2;

    val1 = G_get_raster_value_d(rast1, data_type);
    val2 = G_get_raster_value_d(rast2, data_type);
    return G_set_d_raster_cat (&val1, &val2, label, pcats);
}


/*!
 * \brief write raster category file
 *
 * Writes the category file for the raster map <b>name</b> in
 * the current mapset from the <b>cats</b> structure.
 * Returns 1 if successful. Otherwise, -1 is returned (no diagnostic is
 * printed).
 *
 *  \param name
 *  \param cats
 *  \return int
 */

int G_write_cats ( char *name , struct Categories *cats )
{
    return G__write_cats ("cats", name, cats);
}


/*!
 * \brief 
 *
 * Same as existing G_write_cats()
 *
 *  \param name
 *  \param pcats
 *  \return int
 */

int G_write_raster_cats ( char *name , struct Categories *cats )
{
    return G__write_cats ("cats", name, cats);
}


/*!
 * \brief write
 *       vector category file
 *
 * Writes the category file for the vector map
 * <b>name</b> in the current mapset from the <b>cats</b> structure.
 * Returns 1 if successful. Otherwise, -1 is returned (no diagnostic is 
 * printed).
 *
 *  \param name
 *  \param cats
 *  \return int
 */

int G_write_vector_cats ( char *name , struct Categories *cats )
{
    return G__write_cats ("dig_cats", name, cats);
}

int G__write_cats( char *element, char *name, struct Categories *cats)
{
    FILE *fd ;
    int i, fp_map;
    char *descr;
    DCELL val1, val2;
    char str1[100], str2[100];

/* DEBUG fprintf(stderr,"G__write_cats(*element = '%s', name = '%s', cats = %p\n",element,name,cats); */
    if (!(fd = G_fopen_new (element, name)))
	return -1;

/* write # cats - note # indicate 3.0 or later */
    fprintf(fd,"# %ld categories\n", (long) cats->num);

/* title */
    fprintf(fd,"%s\n", cats->title!=NULL?cats->title:"") ;

/* write format and coefficients */
    fprintf(fd,"%s\n", cats->fmt!=NULL?cats->fmt:"") ;
    fprintf(fd,"%.2f %.2f %.2f %.2f\n",
	    cats->m1, cats->a1, cats->m2, cats->a2) ;

    /* if the map is integer or if this is a vector map, sort labels */
    if(strncmp(element, "dig", 3)== 0)
       fp_map = 0;
    else
       fp_map = G_raster_map_is_fp(name, G_mapset());
/* DEBUG fprintf(stderr,"G__write_cats(): fp_map = %d\n",fp_map); */
    if(!fp_map) G_sort_cats (cats);

    /* write the cat numbers:label */
    for (i = 0; i < G_quant_nof_rules(&cats->q); i++)
    {
	descr = G_get_ith_d_raster_cat(cats, i, &val1, &val2);
	if ((cats->fmt && cats->fmt[0])
	||  (descr && descr[0]))
	{
	    if(val1 == val2)
	    {
	       sprintf(str1, "%.10f", val1);
	       G_trim_decimal (str1);
	       fprintf(fd,"%s:%s\n", str1, 
		       descr!=NULL?descr:"");
            }
	    else
	    {
	       sprintf(str1, "%.10f", val1);
	       G_trim_decimal (str1);
	       sprintf(str2, "%.10f", val2);
	       G_trim_decimal (str2);
	       fprintf(fd,"%s:%s:%s\n", str1, str2,  
		       descr!=NULL?descr:"");
            }
	}
    }
    fclose (fd) ;
/* DEBUG fprintf(stderr,"G__write_cats(): done\n"); */
    return(1) ;
}


/*!
 * \brief 
 *
 * Returns i-th description and i-th data range
 * from the list of category descriptions with corresponding data ranges. end
 * points of data interval in <em>rast1</em> and <em>rast2</em>.
 *
 *  \param pcats
 *  \param i
 *  \param rast1
 *  \param rast2
 *  \return char * 
 */

char *G_get_ith_d_raster_cat (
     const struct Categories *pcats,
     int i,
     DCELL *rast1,DCELL *rast2)
{
     int index;
     if(i > pcats->ncats) 
     {
	 G_set_d_null_value(rast1, 1);
	 G_set_d_null_value(rast2, 1);
	 return "";
     }
     G_quant_get_ith_rule(&pcats->q, i, rast1, rast2, &index, &index);
     return pcats->labels[index];
}


/*!
 * \brief 
 *
 * Returns i-th description and i-th data range
 * from the list of category descriptions with corresponding data ranges. end
 * points of data interval in <em>rast1</em> and <em>rast2</em>.
 *
 *  \param pcats
 *  \param i
 *  \param rast1
 *  \param rast2
 *  \return char * 
 */

char *G_get_ith_f_raster_cat (
     const struct Categories *pcats,
     int i,
     void *rast1,void *rast2)
{
     RASTER_MAP_TYPE data_type = FCELL_TYPE;  
     char *tmp;
     DCELL val1, val2;
     tmp = G_get_ith_d_raster_cat (pcats, i, &val1, &val2);
     G_set_raster_value_d(rast1, val1, data_type);
     G_set_raster_value_d(rast2, val2, data_type);
     return tmp;
}


/*!
 * \brief 
 *
 * Returns i-th description and i-th data range
 * from the list of category descriptions with corresponding data ranges. end
 * points of data interval in <em>rast1</em> and <em>rast2</em>.
 *
 *  \param pcats
 *  \param i
 *  \param rast1
 *  \param rast2
 *  \return char * 
 */

char *G_get_ith_c_raster_cat (
     const struct Categories *pcats,
     int i,
     void *rast1,void *rast2)
{
     RASTER_MAP_TYPE data_type = CELL_TYPE;  
     char *tmp;
     DCELL val1, val2;
     tmp = G_get_ith_d_raster_cat (pcats, i, &val1, &val2);
     G_set_raster_value_d(rast1, val1, data_type);
     G_set_raster_value_d(rast2, val2, data_type);
     return tmp;
}


/*!
 * \brief 
 *
 * Returns i-th
 * description and i-th data range from the list of category descriptions with
 * corresponding data ranges. Stores end points of data interval in <em>rast1</em>
 * and <em>rast2</em> (after converting them to <em>data_type</em>.
 *
 *  \param pcats
 *  \param i
 *  \param rast1
 *  \param rast2
 *  \param data_type
 *  \return char * 
 */

char *
G_get_ith_raster_cat (const struct Categories *pcats, int i, void *rast1, void *rast2, RASTER_MAP_TYPE data_type)  
{
     char *tmp;
     DCELL val1, val2;
     tmp = G_get_ith_d_raster_cat (pcats, i, &val1, &val2);
     G_set_raster_value_d(rast1, val1, data_type);
     G_set_raster_value_d(rast2, val2, data_type);
     return tmp;
}


/*!
 * \brief initialize category structure
 *
 * To construct a new category file, the
 * structure must first be initialized.  This routine initializes the
 * <b>cats</b> structure, and copies the <b>title</b> into the structure.
 * The number of categories is set initially to <b>n.</b>
 * For example:
  \code
    struct Categories cats;
    G_init_cats ( (CELL)0, "", &cats);
  \endcode
 *
 *  \param n
 *  \param title
 *  \param cats
 *  \return int
 */

int G_init_cats (
    CELL num,
    const char *title,
    struct Categories *pcats)
{
    G_init_raster_cats (title, pcats);
    pcats->num = num;
      return 0;
}


/*!
 * \brief 
 *
 * Same as existing G_init_raster_cats() only ncats argument is
 * missign. ncats has no meaning in new Categories structure and only stores
 * (int) largets data value for backwards compatibility.
 *
 *  \param title
 *  \param pcats
 *  \return int
 */

int 
G_init_raster_cats (const char *title, struct Categories *pcats)
{
    G_set_raster_cats_title (title, pcats);
    pcats->labels = NULL;
    pcats->nalloc = 0;
    pcats->ncats = 0;
    pcats->num = 0;
    pcats->fmt = NULL;
    pcats->m1 = 0.0;
    pcats->a1 = 0.0;
    pcats->m2 = 0.0;
    pcats->a2 = 0.0;
    pcats->last_marked_rule = -1;
    G_quant_init(&pcats->q);
      return 0;
}


/*!
 * \brief set title in category structure
 *
 * The <b>title</b> is copied into the
 * <b>cats</b> structure.
 *
 *  \param title
 *  \param cats
 *  \return int
 */

int 
G_set_cats_title (const char *title, struct Categories *pcats)
{
    G_set_raster_cats_title (title, pcats);
      return 0;
}


/*!
 * \brief 
 *
 * Same as existing G_set_cats_title()
 *
 *  \param title
 *  \param pcats
 *  \return int
 */

int 
G_set_raster_cats_title (const char *title, struct Categories *pcats)
{
    if (title == NULL) title="";
    pcats->title = G_store (title);
    G_newlines_to_spaces (pcats->title);
    G_strip (pcats->title);
      return 0;
}

int G_set_cats_fmt (const char *fmt, double m1, double a1, double m2, double a2, struct Categories *pcats)
{
    G_set_raster_cats_fmt (fmt, m1, a1, m2, a2, pcats);
    return 0;
}


/*!
 * \brief 
 *
 * Same as existing G_set_cats_fmt()
 *
 *  \param fmt
 *  \param m1
 *  \param a1
 *  \param m2
 *  \param a2
 *  \param pcats
 *  \return int
 */

int G_set_raster_cats_fmt (const char *fmt, double m1, double a1, double m2, double a2, struct Categories *pcats)
{
    pcats->m1 = m1;
    pcats->a1 = a1;
    pcats->m2 = m2;
    pcats->a2 = a2;

    pcats->fmt = G_store (fmt);
    G_newlines_to_spaces (pcats->fmt);
    G_strip(pcats->fmt);
    return 0;
}


/*!
 * \brief free category structure memory
 *
 * Frees memory allocated by<i>G_read_cats, G_init_cats</i>
 * and<i>G_set_cat.</i>
 *
 *  \param cats
 *  \return int
 */

int G_free_cats (struct Categories *pcats)
{
    G_free_raster_cats (pcats);
    return 0;
}


/*!
 * \brief 
 *
 * Same as existing G_free_cats()
 *
 *  \param pcats
 *  \return int
 */

int G_free_raster_cats (struct Categories *pcats)
{
    int i;

    if (pcats->title != NULL)
    {
	G_free (pcats->title);
	pcats->title = NULL;
    }
    if (pcats->fmt != NULL)
    {
	G_free (pcats->fmt);
	pcats->fmt = NULL;
    }
    if (pcats->ncats > 0)
    {
	for (i = 0; i < pcats->ncats; i++)
	    if (pcats->labels[i] != NULL)
		G_free (pcats->labels[i]);
	G_free (pcats->labels);
	G_free (pcats->marks);
	pcats->labels = NULL;
    }
    G_quant_free (&pcats->q);
    pcats->ncats = 0;
    pcats->nalloc = 0;
    return 0;
}


/*!
 * \brief 
 *
 * Allocates NEW space for quant rules and labels n
 * <em>pcats_to</em> and copies all info from <em>pcats_from</em> cats to <em>pcats_to</em> cats.
 * returns:
 * 0 if successful
 * -1 on fail
 *
 *  \param pcats_to
 *  \param pcats_from
 *  \return int
 */

int 
G_copy_raster_cats (struct Categories *pcats_to, const struct Categories *pcats_from)
{
   int i;
   char *descr;
   DCELL d1, d2;
 
   G_init_raster_cats(pcats_from->title, pcats_to);
   for(i = 0; i < pcats_from->ncats; i++)
   {
      descr = G_get_ith_d_raster_cat (pcats_from, i, &d1, &d2);
      G_set_d_raster_cat(&d1, &d2, descr, pcats_to);
   }
    return 0;
}

int G_number_of_raster_cats (struct Categories *pcats)
{
  return pcats->ncats;
}

static struct Categories save_cats;

int G_sort_cats (struct Categories *pcats)
{
   int *indexes, i , ncats;
   char *descr;
   DCELL d1, d2;

   if (pcats->ncats <= 1) return -1;

   ncats = pcats->ncats;
/* DEBUG fprintf(stderr,"G_sort_cats(): Copying to save cats buffer\n"); */
   G_copy_raster_cats(&save_cats, pcats);
   G_free_raster_cats(pcats);

   indexes = (int *) G_malloc(sizeof(int) * ncats);
   for(i = 0; i < ncats; i++)
       indexes[i] = i;

   qsort (indexes, ncats, sizeof (int), cmp);
   G_init_raster_cats(save_cats.title, pcats);
   for(i = 0; i < ncats; i++)
   {
      descr = G_get_ith_d_raster_cat (&save_cats, indexes[i], &d1, &d2);
/* DEBUG fprintf(stderr,"G_sort_cats(): Write sorted cats, pcats = %p pcats->labels = %p\n",pcats,pcats->labels); */
      G_set_d_raster_cat(&d1, &d2, descr, pcats);
/* DEBUG fflush(stderr); */
   }
   G_free_raster_cats(&save_cats);
/* DEBUG fprintf(stderr,"G_sort_cats(): Done\n"); */
/* fflush(stderr); */

   return 0;
}

static int cmp (const void *aa, const void *bb)
{
    const int *a = aa, *b = bb;
    DCELL min_rast1, min_rast2, max_rast1, max_rast2; 
    CELL index;
    G_quant_get_ith_rule(&(save_cats.q), *a,
        &min_rast1, &max_rast1, &index, &index);
    G_quant_get_ith_rule(&(save_cats.q), *b,
        &min_rast2, &max_rast2, &index, &index);
    if(min_rast1 < min_rast2)
	return -1;
    if(min_rast1 > min_rast2)
	return 1;
    return 0;
}


