/*!
 * \file lib/raster/cats.c
 *
 * \brief Raster Library - Raster categories management
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
 * Note: while both the format and coefficient lines must be present
 *       a blank line for the fmt will effectively suppress automatic
 *       label generation
 *
 * Note: quant rules of Categories structures are heavily dependent
 * on the fact that rules are stored in the same order they are entered.
 * since i-th rule and i-th label are entered at the same time, we
 * know that i-th rule maps fp range to i, thus we know for sure
 * that cats.labels[i] corresponds to i-th quant rule
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static void get_cond(char **, char *, DCELL);
static int get_fmt(char **, char *, int *);
static int cmp(const void *, const void *);

static void write_cats(const char *element, const char *name,
                       struct Categories *cats);
static CELL read_cats(const char *element, const char *name, const char *mapset,
                      struct Categories *pcats, int full);

static struct Categories save_cats;

/*!
 * \brief Read raster category file
 *
 * The category file for raster map <i>name</i> in <i>mapset</i> is
 * read into the <i>cats</i> structure. If there is an error reading
 * the category file, a diagnostic message is printed and -1 is
 * returned. Otherwise, 0 is returned.
 *
 * \param name raster map name
 * \param mapset mapset name
 * \param[out] pcats pointer to Cats structure
 *
 * \return -1 on error
 * \return 0 on success
 */
int Rast_read_cats(const char *name, const char *mapset,
                   struct Categories *pcats)
{
    switch (read_cats("cats", name, mapset, pcats, 1)) {
    case -2:
        G_warning(_("Category support for <%s@%s> missing"), name, mapset);
        break;
    case -1:
        G_warning(_("Category support for <%s@%s> invalid"), name, mapset);
        break;
    default:
        return 0;
    }

    return -1;
}

/*!
 * \brief Read vector category file
 *
 * <b>Note:</b> This function works with <b>old</b> vector format.
 *
 * \todo: To be moved to the vector library
 *
 * The category  file for vector  map <i>name</i> in  <i>mapset</i> is
 * read into the  <i>cats</i> structure. If there is  an error reading
 * the  category file,  a  diagnostic  message is  printed  and -1  is
 * returned. Otherwise, 0 is returned.
 *
 * \param name vector map name
 * \param mapset mapset name
 * \param[out] pcats pointer to Cats structure
 *
 * \return -1 on error
 * \return 0 on success
 */
int Rast_read_vector_cats(const char *name, const char *mapset,
                          struct Categories *pcats)
{
    switch (read_cats("dig_cats", name, mapset, pcats, 1)) {
    case -2:
        G_warning(_("Category support for vector map <%s@%s> missing"), name,
                  mapset);
        break;
    case -1:
        G_warning(_("Category support for vector map <%s@%s> invalid"), name,
                  mapset);
        break;
    default:
        return 0;
    }

    return -1;
}

/*!
   \brief Get the max category number

   Return the max category number of a raster map
   of type CELL.

   \param name raster map name
   \param mapset mapset name

   \return -1 on error
   \return number of cats
 */
CELL Rast_get_max_c_cat(const char *name, const char *mapset)
{
    struct Range range;
    CELL min, max;

    /* return the max category number */
    if (Rast_read_range(name, mapset, &range) < 0)
        return -1;
    Rast_get_range_min_max(&range, &min, &max);
    if (Rast_is_c_null_value(&max))
        max = 0;
    return max;
}

static CELL read_cats(const char *element, const char *name, const char *mapset,
                      struct Categories *pcats, int full)
{
    FILE *fd;
    char buff[1024];
    CELL cat1, cat2;
    DCELL val1, val2;
    int old = 0, fp_map;
    long num = -1;

    if (strncmp(element, "dig", 3) == 0)
        fp_map = 0;
    else
        fp_map = Rast_map_is_fp(name, mapset);

    if (!(fd = G_fopen_old(element, name, mapset)))
        return -2;

    /* Read the number of categories */
    if (G_getl(buff, sizeof buff, fd) == 0)
        goto error;

    if (sscanf(buff, "# %ld", &num) == 1)
        old = 0;
    else if (sscanf(buff, "%ld", &num) == 1)
        old = 1;

    if (!full) {
        fclose(fd);
        if (num < 0)
            return 0; /* coorect */
        return (CELL)num;
    }

    /* Read the title for the file */
    if (G_getl(buff, sizeof buff, fd) == 0)
        goto error;
    G_strip(buff);
    /*    G_ascii_check(buff) ; */

    Rast_init_cats(buff, pcats);
    if (num >= 0)
        pcats->num = num;

    if (!old) {
        char fmt[256];
        float m1, a1, m2, a2;

        if (G_getl(fmt, sizeof fmt, fd) == 0)
            goto error;
        /* next line contains equation coefficients */
        if (G_getl(buff, sizeof buff, fd) == 0)
            goto error;
        if (sscanf(buff, "%f %f %f %f", &m1, &a1, &m2, &a2) != 4)
            goto error;
        Rast_set_cats_fmt(fmt, m1, a1, m2, a2, pcats);
    }

    /* Read all category names */
    for (cat1 = 0;; cat1++) {
        char label[1024];

        if (G_getl(buff, sizeof buff, fd) == 0)
            break;
        if (old)
            Rast_set_c_cat(&cat1, &cat1, buff, pcats);
        else {
            *label = 0;
            if (sscanf(buff, "%1s", label) != 1)
                continue;
            if (*label == '#')
                continue;
            *label = 0;
            /* for fp maps try to read a range of data */
            if (fp_map &&
                sscanf(buff, "%lf:%lf:%[^\n]", &val1, &val2, label) == 3)
                Rast_set_cat(&val1, &val2, label, pcats, DCELL_TYPE);
            else if (!fp_map &&
                     sscanf(buff, "%d:%d:%[^\n]", &cat1, &cat2, label) == 3)
                Rast_set_cat(&cat1, &cat2, label, pcats, CELL_TYPE);
            else if (sscanf(buff, "%d:%[^\n]", &cat1, label) >= 1)
                Rast_set_cat(&cat1, &cat1, label, pcats, CELL_TYPE);
            else if (sscanf(buff, "%lf:%[^\n]", &val1, label) >= 1)
                Rast_set_cat(&val1, &val1, label, pcats, DCELL_TYPE);
            else
                goto error;
        }
    }

    fclose(fd);
    return 0;
error:
    fclose(fd);
    return -1;
}

/*!
 * \brief Get title from category structure struct
 *
 * \todo Remove from GIS Library, replace by Rast_get_c_cats_title().
 *
 * Map layers store a one-line title in the category structure as
 * well. This routine returns a pointer to the title contained in the
 * <i>cats</i> structure. A legal pointer is always returned. If the
 * map layer does not have a title, then a pointer to the empty string
 * "" is returned.
 *
 * \param pcats pointer to Categories structure
 *
 * \return title
 * \return "" if missing
 */
char *Rast_get_cats_title(const struct Categories *pcats)
{
    return pcats->title ? pcats->title : "";
}

/*!
 * \brief Get a raster category label (CELL)
 *
 * This routine looks up category <i>rast</i> in the <i>pcats</i>
 * structure and returns a pointer to a string which is the label for
 * the category. A legal pointer is always returned. If the category
 * does not exist in <i>pcats</i>, then a pointer to the empty string
 * "" is returned.
 *
 * <b>Warning:</b> The pointer that is returned points to a hidden
 * static buffer. Successive calls to Rast_get_c_cat() overwrite this
 * buffer.
 *
 * \param rast cell value
 * \param pcats pointer to Categories structure
 *
 * \return pointer to category label
 * \return "" if category is not found
 */
char *Rast_get_c_cat(CELL *rast, struct Categories *pcats)
{
    return Rast_get_cat(rast, pcats, CELL_TYPE);
}

/*!
 * \brief Get a raster category label (FCELL)
 *
 * This routine looks up category <i>rast</i> in the <i>pcats</i>
 * structure and returns a pointer to a string which is the label for
 * the category. A legal pointer is always returned. If the category
 * does not exist in <i>pcats</i>, then a pointer to the empty string
 * "" is returned.
 *
 * <b>Warning:</b> The pointer that is returned points to a hidden
 * static buffer. Successive calls to Rast_get_c_cat() overwrite this
 * buffer.
 *
 * \param rast cell value
 * \param pcats pointer to Categories structure
 *
 * \return pointer to category label
 * \return "" if category is not found
 */
char *Rast_get_f_cat(FCELL *rast, struct Categories *pcats)
{
    return Rast_get_cat(rast, pcats, FCELL_TYPE);
}

/*!
 * \brief Get a raster category label (DCELL)
 *
 * This routine looks up category <i>rast</i> in the <i>pcats</i>
 * structure and returns a pointer to a string which is the label for
 * the category. A legal pointer is always returned. If the category
 * does not exist in <i>pcats</i>, then a pointer to the empty string
 * "" is returned.
 *
 * <b>Warning:</b> The pointer that is returned points to a hidden
 * static buffer. Successive calls to Rast_get_c_cat() overwrite this
 * buffer.
 *
 * \param rast cell value
 * \param pcats pointer to Categories structure
 *
 * \return pointer to category label
 * \return "" if category is not found
 */
char *Rast_get_d_cat(DCELL *rast, struct Categories *pcats)
{
    return Rast_get_cat(rast, pcats, DCELL_TYPE);
}

/*!
 * \brief Get a raster category label
 *
 * This routine looks up category <i>rast</i> in the <i>pcats</i>
 * structure and returns a pointer to a string which is the label for
 * the category. A legal pointer is always returned. If the category
 * does not exist in <i>pcats</i>, then a pointer to the empty string
 * "" is returned.
 *
 * <b>Warning:</b> The pointer that is returned points to a hidden
 * static buffer. Successive calls to Rast_get_c_cat() overwrite this
 * buffer.
 *
 * \param rast cell value
 * \param pcats pointer to Categories structure
 * \param data_type map type (CELL, FCELL, DCELL)
 *
 * \return pointer to category label
 * \return "" if category is not found
 */
char *Rast_get_cat(void *rast, struct Categories *pcats,
                   RASTER_MAP_TYPE data_type)
{
    static char label[1024];
    char *f, *l, *v;
    CELL i;
    DCELL val;
    float a[2];
    char fmt[30], value_str[30];

    if (Rast_is_null_value(rast, data_type)) {
        sprintf(label, "no data");
        return label;
    }

    /* first search the list of labels */
    *label = 0;
    val = Rast_get_d_value(rast, data_type);
    i = Rast_quant_get_cell_value(&pcats->q, val);

    G_debug(5, "Rast_get_cat(): val %lf found i %d", val, i);

    if (!Rast_is_c_null_value(&i) && i < pcats->ncats) {
        if (pcats->labels[i] != NULL)
            return pcats->labels[i];
        return label;
    }

    /* generate the label */
    if ((f = pcats->fmt) == NULL)
        return label;

    a[0] = (float)val * pcats->m1 + pcats->a1;
    a[1] = (float)val * pcats->m2 + pcats->a2;

    l = label;
    while (*f) {
        if (*f == '$') {
            f++;
            if (*f == '$')
                *l++ = *f++;
            else if (*f == '?') {
                f++;
                get_cond(&f, v = value_str, val);
                while (*v)
                    *l++ = *v++;
            }
            else if (get_fmt(&f, fmt, &i)) {
                sprintf(v = value_str, fmt, a[i]);
                while (*v)
                    *l++ = *v++;
            }
            else
                *l++ = '$';
        }
        else {
            *l++ = *f++;
        }
    }
    *l = 0;
    return label;
}

/*!
 * \brief Sets marks for all categories to 0.
 *
 * This initializes Categories structure for subsequent calls to
 * Rast_mark_cats() for each row of data, where non-zero mark for
 * i-th label means that some of the cells in rast_row are labeled
 * with i-th label and fall into i-th data range. These marks help
 * determine from the Categories structure which labels were used and
 * which weren't.
 *
 * \param pcats pointer to Categories structure
 */
void Rast_unmark_cats(struct Categories *pcats)
{
    int i;

    for (i = 0; i < pcats->ncats; i++)
        pcats->marks[i] = 0;
}

/*!
 * \brief Looks up the category label for each raster value (CELL).
 *
 * Looks up the category label for each raster value in the
 * <i>rast_row</i> and updates the marks for labels found.
 *
 * <b>Note:</b> Non-zero mark for i-th label stores the number of of
 * raster cells read so far which are labeled with i-th label and fall
 * into i-th data range.
 *
 * \param rast_row raster row to update stats
 * \param ncols number of columns
 * \param pcats pointer to Categories structure
 *
 */
void Rast_mark_c_cats(const CELL *rast_row, int ncols, struct Categories *pcats)
{
    Rast_mark_cats(rast_row, ncols, pcats, CELL_TYPE);
}

/*!
 * \brief Looks up the category label for each raster value (FCELL).
 *
 * Looks up the category label for each raster value in the
 * <i>rast_row</i> and updates the marks for labels found.
 *
 * <b>Note:</b> Non-zero mark for i-th label stores the number of of
 * raster cells read so far which are labeled with i-th label and fall
 * into i-th data range.
 *
 * \param rast_row raster row to update stats
 * \param ncols number of columns
 * \param pcats pointer to Categories structure
 *
 */
void Rast_mark_f_cats(const FCELL *rast_row, int ncols,
                      struct Categories *pcats)
{
    Rast_mark_cats(rast_row, ncols, pcats, FCELL_TYPE);
}

/*!
 * \brief Looks up the category label for each raster value (DCELL).
 *
 * Looks up the category label for each raster value in the
 * <i>rast_row</i> and updates the marks for labels found.
 *
 * <b>Note:</b> Non-zero mark for i-th label stores the number of of
 * raster cells read so far which are labeled with i-th label and fall
 * into i-th data range.
 *
 * \param rast_row raster row to update stats
 * \param ncols number of columns
 * \param pcats pointer to Categories structure
 *
 */
void Rast_mark_d_cats(const DCELL *rast_row, int ncols,
                      struct Categories *pcats)
{
    Rast_mark_cats(rast_row, ncols, pcats, DCELL_TYPE);
}

/*!
 * \brief Looks up the category label for each raster value (DCELL).
 *
 * Looks up the category label for each raster value in the
 * <i>rast_row</i> and updates the marks for labels found.
 *
 * <b>Note:</b> Non-zero mark for i-th label stores the number of of
 * raster cells read so far which are labeled with i-th label and fall
 * into i-th data range.
 *
 * \param rast_row raster row to update stats
 * \param ncols number of columns
 * \param pcats pointer to Categories structure
 * \param data_type map type
 *
 * \return -1 on error
 * \return 1 on success
 */
int Rast_mark_cats(const void *rast_row, int ncols, struct Categories *pcats,
                   RASTER_MAP_TYPE data_type)
{
    size_t size = Rast_cell_size(data_type);
    CELL i;

    while (ncols-- > 0) {
        i = Rast_quant_get_cell_value(&pcats->q,
                                      Rast_get_d_value(rast_row, data_type));
        if (Rast_is_c_null_value(&i))
            continue;
        if (i > pcats->ncats)
            return -1;
        pcats->marks[i]++;
        rast_row = G_incr_void_ptr(rast_row, size);
    }
    return 1;
}

/*!
 * \brief Rewind raster categories
 *
 * After call to this function Rast_get_next_marked_cat() returns
 * the first marked cat label.
 *
 * \param pcats pointer to Categories structure
 */
void Rast_rewind_cats(struct Categories *pcats)
{
    pcats->last_marked_rule = -1;
}

/*!
   \brief Get next marked raster categories (DCELL)

   \param pcats pointer to Categories structure
   \param rast1, rast2 cell values (raster range)
   \param[out] count count

   \return NULL if not found
   \return description if found
 */
char *Rast_get_next_marked_d_cat(struct Categories *pcats, DCELL *rast1,
                                 DCELL *rast2, long *count)
{
    char *descr = NULL;
    int found, i;

    found = 0;
    /* pcats->ncats should be == Rast_quant_nof_rules(&pcats->q) */

    G_debug(3, "last marked %d nrules %d\n", pcats->last_marked_rule,
            Rast_quant_nof_rules(&pcats->q));

    for (i = pcats->last_marked_rule + 1; i < Rast_quant_nof_rules(&pcats->q);
         i++) {
        descr = Rast_get_ith_d_cat(pcats, i, rast1, rast2);
        G_debug(5, "%d %d", i, pcats->marks[i]);
        if (pcats->marks[i]) {
            found = 1;
            break;
        }
    }

    if (!found)
        return NULL;

    *count = pcats->marks[i];
    pcats->last_marked_rule = i;
    return descr;
}

/*!
   \brief Get next marked raster categories (CELL)

   \param pcats pointer to Categories structure
   \param rast1, rast2 cell values (raster range)
   \param[out] count count

   \return NULL if not found
   \return description if found
 */
char *Rast_get_next_marked_c_cat(struct Categories *pcats, CELL *rast1,
                                 CELL *rast2, long *count)
{
    return Rast_get_next_marked_cat(pcats, rast1, rast2, count, CELL_TYPE);
}

/*!
   \brief Get next marked raster categories (FCELL)

   \param pcats pointer to Categories structure
   \param rast1, rast2 cell values (raster range)
   \param[out] count count

   \return NULL if not found
   \return description if found
 */
char *Rast_get_next_marked_f_cat(struct Categories *pcats, FCELL *rast1,
                                 FCELL *rast2, long *count)
{
    return Rast_get_next_marked_cat(pcats, rast1, rast2, count, FCELL_TYPE);
}

/*!
   \brief Get next marked raster categories

   \param pcats pointer to Categories structure
   \param rast1, rast2 cell values (raster range)
   \param[out] count count
   \param data_type map type

   \return NULL if not found
   \return description if found
 */
char *Rast_get_next_marked_cat(struct Categories *pcats, void *rast1,
                               void *rast2, long *count,
                               RASTER_MAP_TYPE data_type)
{
    DCELL val1, val2;
    char *lab;

    lab = Rast_get_next_marked_d_cat(pcats, &val1, &val2, count);
    Rast_set_d_value(rast1, val1, data_type);
    Rast_set_d_value(rast2, val2, data_type);
    return lab;
}

static int get_fmt(char **f, char *fmt, int *i)
{
    char *ff;

    ff = *f;
    if (*ff == 0)
        return 0;
    if (*ff == '$') {
        *f = ff + 1;
        return 0;
    }
    switch (*ff++) {
    case '1':
        *i = 0;
        break;
    case '2':
        *i = 1;
        break;
    default:
        return 0;
    }
    *fmt++ = '%';
    *fmt++ = '.';
    if (*ff++ != '.') {
        *f = ff - 1;
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

static void get_cond(char **f, char *value, DCELL val)
{
    char *ff;

    ff = *f;
    if (val == 1.) {
        while (*ff)
            if (*ff++ == '$')
                break;
    }

    while (*ff)
        if (*ff == '$') {
            ff++;
            break;
        }
        else
            *value++ = *ff++;

    if (val != 1.) {
        while (*ff)
            if (*ff++ == '$')
                break;
    }
    *value = 0;
    *f = ff;
}

/*!
 * \brief Set a raster category label (CELL)
 *
 * Adds the label for range <i>rast1</i> through <i>rast2</i> in
 * category structure <i>pcats</i>.
 *
 * \param rast1, rast2 raster values (range)
 * \param label category label
 * \param pcats pointer to Categories structure
 *
 * \return -1 on error
 * \return 0 if null value detected
 * \return 1 on success
 */
int Rast_set_c_cat(const CELL *rast1, const CELL *rast2, const char *label,
                   struct Categories *pcats)
{
    return Rast_set_cat(rast1, rast2, label, pcats, CELL_TYPE);
}

/*!
 * \brief Set a raster category label (FCELL)
 *
 * Adds the label for range <i>rast1</i> through <i>rast2</i> in
 * category structure <i>pcats</i>.
 *
 * \param rast1, rast2 raster values (range)
 * \param label category label
 * \param pcats pointer to Categories structure
 *
 * \return
 */
int Rast_set_f_cat(const FCELL *rast1, const FCELL *rast2, const char *label,
                   struct Categories *pcats)
{
    return Rast_set_cat(rast1, rast2, label, pcats, FCELL_TYPE);
}

/*!
 * \brief Set a raster category label (DCELL)
 *
 * Adds the label for range <i>rast1</i> through <i>rast2</i> in
 * category structure <i>pcats</i>.
 *
 * \param rast1, rast2 raster values (range)
 * \param label category label
 * \param pcats pointer to Categories structure
 *
 * \return -1 on error
 * \return 0 if null value detected
 * \return 1 on success
 */
int Rast_set_d_cat(const DCELL *rast1, const DCELL *rast2, const char *label,
                   struct Categories *pcats)
{
    long len;
    DCELL dtmp1, dtmp2;
    int i;

    /* DEBUG fprintf(stderr,"Rast_set_d_cat(rast1 = %p,rast2 = %p,label =
       '%s',pcats = %p)\n", rast1,rast2,label,pcats); */
    if (Rast_is_d_null_value(rast1))
        return 0;
    if (Rast_is_d_null_value(rast2))
        return 0;
    /* DEBUG fprintf (stderr, "Rast_set_d_cat(): adding quant rule: %f %f %d
     * %d\n", *rast1, *rast2, pcats->ncats, pcats->ncats); */
    /* the set_cat() functions are used in many places to reset the labels
       for the range (or cat) with existing label. In this case we don't
       want to store both rules with identical range even though the result
       of get_cat() will be correct, since it will use rule added later.
       we don't want to overuse memory and we don't want rules which are
       not used to be written out in cats file. So we first look if
       the label for this range has been sen, and if it has, overwrite it */

    for (i = 0; i < pcats->ncats; i++) {
        Rast_get_ith_d_cat(pcats, i, &dtmp1, &dtmp2);
        if ((dtmp1 == *rast1 && dtmp2 == *rast2) ||
            (dtmp1 == *rast2 && dtmp2 == *rast1)) {
            if (pcats->labels[i] != NULL)
                G_free(pcats->labels[i]);
            pcats->labels[i] = G_store(label);
            G_newlines_to_spaces(pcats->labels[i]);
            G_strip(pcats->labels[i]);
            return 1;
        }
    }
    /* when rule for this range does not exist */
    /* DEBUG fprintf (stderr, "Rast_set_d_cat(): New rule: adding %d %p\n", i,
     * pcats->labels); */
    Rast_quant_add_rule(&pcats->q, *rast1, *rast2, pcats->ncats, pcats->ncats);
    pcats->ncats++;
    if (pcats->nalloc < pcats->ncats) {
        /* DEBUG fprintf (stderr, "Rast_set_d_cat(): need more space nalloc = %d
         * ncats = %d\n", pcats->nalloc,pcats->ncats); */
        len = (pcats->nalloc + 256) * sizeof(char *);
        /* DEBUG fprintf (stderr, "Rast_set_d_cat(): allocating %d
         * labels(%d)\n", pcats->nalloc + 256,(int)len); */
        if (len != (int)len) { /* make sure len doesn't overflow int */
            pcats->ncats--;
            return -1;
        }
        /* DEBUG fprintf(stderr,"Rast_set_d_cat(): pcats->nalloc = %d,
         * pcats->labels = (%p), len =
         * %d\n",pcats->nalloc,pcats->labels,(int)len); */
        if (pcats->nalloc) {
            /* DEBUG fprintf(stderr,"Rast_set_d_cat(): Realloc-ing pcats->labels
             * (%p)\n",pcats->labels); */
            pcats->labels = (char **)G_realloc((char *)pcats->labels, (int)len);
        }
        else {
            /* DEBUG fprintf(stderr,"Rast_set_d_cat(): alloc-ing new labels
             * pointer array\n"); */
            pcats->labels = (char **)G_malloc((int)len);
        }
        /* fflush(stderr); */
        /* DEBUG fprintf (stderr, "Rast_set_d_cats(): allocating %d
         * marks(%d)\n", pcats->nalloc + 256,(int)len); */
        len = (pcats->nalloc + 256) * sizeof(int);
        if (len != (int)len) { /* make sure len doesn't overflow int */
            pcats->ncats--;
            return -1;
        }
        if (pcats->nalloc)
            pcats->marks = (int *)G_realloc((char *)pcats->marks, (int)len);
        else
            pcats->marks = (int *)G_malloc((int)len);
        pcats->nalloc += 256;
    }
    /* DEBUG fprintf(stderr,"Rast_set_d_cats(): store new label\n"); */
    pcats->labels[pcats->ncats - 1] = G_store(label);
    G_newlines_to_spaces(pcats->labels[pcats->ncats - 1]);
    G_strip(pcats->labels[pcats->ncats - 1]);
    /* DEBUG
       fprintf (stderr, "%d %s\n", pcats->ncats - 1, pcats->labels[pcats->ncats
       - 1]);
     */
    /* updates cats.num = max cat values. This is really just used in old
       raster programs, and I am doing it for backwards cmpatibility (Olga) */
    if ((CELL)*rast1 > pcats->num)
        pcats->num = (CELL)*rast1;
    if ((CELL)*rast2 > pcats->num)
        pcats->num = (CELL)*rast2;
    /* DEBUG fprintf(stderr,"Rast_set_d_cat(): done\n"); */
    /* DEBUG fflush(stderr); */
    return 1;
}

/*!
 * \brief Set a raster category label
 *
 * Adds the label for range <i>rast1</i> through <i>rast2</i> in
 * category structure <i>pcats</i>.
 *
 * \param rast1, rast2 raster values (range)
 * \param label category label
 * \param pcats pointer to Categories structure
 * \param data_type map type
 *
 * \return -1 on error
 * \return 0 if null value detected
 * \return 1 on success
 */

int Rast_set_cat(const void *rast1, const void *rast2, const char *label,
                 struct Categories *pcats, RASTER_MAP_TYPE data_type)
{
    DCELL val1, val2;

    val1 = Rast_get_d_value(rast1, data_type);
    val2 = Rast_get_d_value(rast2, data_type);
    return Rast_set_d_cat(&val1, &val2, label, pcats);
}

/*!
 * \brief Write raster category file
 *
 * \todo To be removed, replaced by Rast_write_cats().
 *
 * Writes the category file for the raster map <i>name</i> in the
 * current mapset from the <i>cats</i> structure.
 *
 * \param name map name
 * \param cats pointer to Categories structure
 *
 * \return void
 */
void Rast_write_cats(const char *name, struct Categories *cats)
{
    write_cats("cats", name, cats);
}

/*!
 * \brief Write vector category file
 *
 * <b>Note:</b> Used for only old vector format!
 *
 * \todo Move to the vector library.
 *
 * \param name map name
 * \param cats pointer to Categories structure
 *
 * \return void
 */
void Rast_write_vector_cats(const char *name, struct Categories *cats)
{
    write_cats("dig_cats", name, cats);
}

static void write_cats(const char *element, const char *name,
                       struct Categories *cats)
{
    FILE *fd;
    int i, fp_map;
    char *descr;
    DCELL val1, val2;
    char str1[100], str2[100];

    fd = G_fopen_new(element, name);
    if (!fd)
        G_fatal_error(_("Unable to open %s file for map <%s>"), element, name);

    /* write # cats - note # indicate 3.0 or later */
    fprintf(fd, "# %ld categories\n", (long)cats->num);

    /* title */
    fprintf(fd, "%s\n", cats->title != NULL ? cats->title : "");

    /* write format and coefficients */
    fprintf(fd, "%s\n", cats->fmt != NULL ? cats->fmt : "");
    fprintf(fd, "%.2f %.2f %.2f %.2f\n", cats->m1, cats->a1, cats->m2,
            cats->a2);

    /* if the map is integer or if this is a vector map, sort labels */
    if (strncmp(element, "dig", 3) == 0)
        fp_map = 0;
    else
        fp_map = Rast_map_is_fp(name, G_mapset());
    if (!fp_map)
        Rast_sort_cats(cats);

    /* write the cat numbers:label */
    for (i = 0; i < Rast_quant_nof_rules(&cats->q); i++) {
        descr = Rast_get_ith_d_cat(cats, i, &val1, &val2);
        if ((cats->fmt && cats->fmt[0]) || (descr && descr[0])) {
            if (val1 == val2) {
                sprintf(str1, "%.10f", val1);
                G_trim_decimal(str1);
                fprintf(fd, "%s:%s\n", str1, descr != NULL ? descr : "");
            }
            else {
                sprintf(str1, "%.10f", val1);
                G_trim_decimal(str1);
                sprintf(str2, "%.10f", val2);
                G_trim_decimal(str2);
                fprintf(fd, "%s:%s:%s\n", str1, str2,
                        descr != NULL ? descr : "");
            }
        }
    }
    fclose(fd);
}

/*!
 * \brief Get category description (DCELL)
 *
 * Returns i-th description and i-th data range from the list of
 * category descriptions with corresponding data ranges. end points of
 * data interval in <i>rast1</i> and <i>rast2</i>.
 *
 * \param pcats pointer to Categories structure
 * \param i index
 * \param rast1, rast2 raster values (range)
 *
 * \return "" on error
 * \return pointer to category description
 */
char *Rast_get_ith_d_cat(const struct Categories *pcats, int i, DCELL *rast1,
                         DCELL *rast2)
{
    int index;

    if (i > pcats->ncats) {
        Rast_set_d_null_value(rast1, 1);
        Rast_set_d_null_value(rast2, 1);
        return "";
    }
    Rast_quant_get_ith_rule(&pcats->q, i, rast1, rast2, &index, &index);
    return pcats->labels[index];
}

/*!
 * \brief Get category description (FCELL)
 *
 * Returns i-th description and i-th data range from the list of
 * category descriptions with corresponding data ranges. end points of
 * data interval in <i>rast1</i> and <i>rast2</i>.
 *
 * \param pcats pointer to Categories structure
 * \param i index
 * \param rast1, rast2 raster values (range)
 *
 * \return "" on error
 * \return pointer to category description
 */
char *Rast_get_ith_f_cat(const struct Categories *pcats, int i, void *rast1,
                         void *rast2)
{
    RASTER_MAP_TYPE data_type = FCELL_TYPE;
    char *tmp;
    DCELL val1, val2;

    tmp = Rast_get_ith_d_cat(pcats, i, &val1, &val2);
    Rast_set_d_value(rast1, val1, data_type);
    Rast_set_d_value(rast2, val2, data_type);
    return tmp;
}

/*!
 * \brief Get category description (CELL)
 *
 * Returns i-th description and i-th data range from the list of
 * category descriptions with corresponding data ranges. end points of
 * data interval in <i>rast1</i> and <i>rast2</i>.
 *
 * \param pcats pointer to Categories structure
 * \param i index
 * \param rast1, rast2 raster values (range)
 *
 * \return "" on error
 * \return pointer to category description
 */
char *Rast_get_ith_c_cat(const struct Categories *pcats, int i, void *rast1,
                         void *rast2)
{
    RASTER_MAP_TYPE data_type = CELL_TYPE;
    char *tmp;
    DCELL val1, val2;

    tmp = Rast_get_ith_d_cat(pcats, i, &val1, &val2);
    Rast_set_d_value(rast1, val1, data_type);
    Rast_set_d_value(rast2, val2, data_type);
    return tmp;
}

/*!
 * \brief Get category description
 *
 * Returns i-th description and i-th data range from the list of
 * category descriptions with corresponding data ranges. end points of
 * data interval in <i>rast1</i> and <i>rast2</i>.
 *
 * \param pcats pointer to Categories structure
 * \param i index
 * \param rast1, rast2 raster values (range)
 * \param data_type map type
 *
 * \return "" on error
 * \return pointer to category description
 */
char *Rast_get_ith_cat(const struct Categories *pcats, int i, void *rast1,
                       void *rast2, RASTER_MAP_TYPE data_type)
{
    char *tmp;
    DCELL val1, val2;

    tmp = Rast_get_ith_d_cat(pcats, i, &val1, &val2);
    Rast_set_d_value(rast1, val1, data_type);
    Rast_set_d_value(rast2, val2, data_type);
    return tmp;
}

/*!
 * \brief Initialize category structure
 *
 * To construct a new category file, the structure must first be
 * initialized. This routine initializes the <i>cats</i> structure,
 * and copies the <i>title</i> into the structure. The number of
 * categories is set initially to <i>n</i>.
 *
 * For example:
 \code
 struct Categories cats;
 Rast_init_cats ("", &cats);
 \endcode
 *
 * \todo Eliminate pcats->num. Num has no meaning in new Categories
 * structure and only stores (int) largest data value for backwards
 * compatibility.
 *
 * \param title title
 * \param pcats pointer to Categories structure
 */
void Rast_init_cats(const char *title, struct Categories *pcats)
{
    Rast_set_cats_title(title, pcats);
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
    Rast_quant_init(&pcats->q);
}

/*!
 * \brief Set title in category structure
 *
 * \todo To be removed, replaced by Rast_set_cats_title().
 *
 * The <i>title</i> is copied into the <i>pcats</i> structure.
 *
 * \param title title
 * \param pcats pointer to Categories structure
 */
void Rast_set_cats_title(const char *title, struct Categories *pcats)
{
    if (title == NULL)
        title = "";
    pcats->title = G_store(title);
    G_newlines_to_spaces(pcats->title);
    G_strip(pcats->title);
}

/*!
   \brief Set category fmt (?)

   \param fmt
   \param m1
   \param a1
   \param m2
   \param a2
   \param pcats pointer to Categories structure
 */
void Rast_set_cats_fmt(const char *fmt, double m1, double a1, double m2,
                       double a2, struct Categories *pcats)
{
    pcats->m1 = m1;
    pcats->a1 = a1;
    pcats->m2 = m2;
    pcats->a2 = a2;

    pcats->fmt = G_store(fmt);
    G_newlines_to_spaces(pcats->fmt);
    G_strip(pcats->fmt);
}

/*!
 * \brief Free category structure memory
 *
 * \todo To be removed, replaced by Rast_free_cats().
 *
 * Frees memory allocated by Rast_read_cats(), Rast_init_cats() and
 * Rast_set_c_cat().
 *
 * \param pcats pointer to Categories structure
 */
void Rast_free_cats(struct Categories *pcats)
{
    int i;

    if (pcats->title != NULL) {
        G_free(pcats->title);
        pcats->title = NULL;
    }
    if (pcats->fmt != NULL) {
        G_free(pcats->fmt);
        pcats->fmt = NULL;
    }
    if (pcats->ncats > 0) {
        for (i = 0; i < pcats->ncats; i++)
            if (pcats->labels[i] != NULL)
                G_free(pcats->labels[i]);
        G_free(pcats->labels);
        G_free(pcats->marks);
        pcats->labels = NULL;
    }
    Rast_quant_free(&pcats->q);
    pcats->ncats = 0;
    pcats->nalloc = 0;
}

/*!
 * \brief Copy raster categories
 *
 * Allocates NEW space for quant rules and labels n <i>pcats_to</i>
 * and copies all info from <i>pcats_from</i> cats to
 * <i>pcats_to</i> cats.
 *
 * \param pcats_to pointer to destination Categories structure
 * \param pcats_from pointer to source Categories structure
 */
void Rast_copy_cats(struct Categories *pcats_to,
                    const struct Categories *pcats_from)
{
    int i;
    char *descr;
    DCELL d1, d2;

    Rast_init_cats(pcats_from->title, pcats_to);
    for (i = 0; i < pcats_from->ncats; i++) {
        descr = Rast_get_ith_d_cat(pcats_from, i, &d1, &d2);
        Rast_set_d_cat(&d1, &d2, descr, pcats_to);
    }
}

/*!
   \brief Get number of raster categories

   \param pcats pointer to Categories structure

   \return number of categories
 */
int Rast_number_of_cats(struct Categories *pcats)
{
    return pcats->ncats;
}

/*!
   \brief Sort categories

   \param pcats pointer to Categories structure

   \return -1 on error (nothing to sort)
   \return 0 on success
 */
int Rast_sort_cats(struct Categories *pcats)
{
    int *indexes, i, ncats;
    char *descr;
    DCELL d1, d2;

    if (pcats->ncats <= 1)
        return -1;

    ncats = pcats->ncats;
    G_debug(3, "Rast_sort_cats(): Copying to save cats buffer");
    Rast_copy_cats(&save_cats, pcats);
    Rast_free_cats(pcats);

    indexes = (int *)G_malloc(sizeof(int) * ncats);
    for (i = 0; i < ncats; i++)
        indexes[i] = i;

    qsort(indexes, ncats, sizeof(int), cmp);
    Rast_init_cats(save_cats.title, pcats);
    for (i = 0; i < ncats; i++) {
        descr = Rast_get_ith_d_cat(&save_cats, indexes[i], &d1, &d2);
        G_debug(4, "  Write sorted cats, pcats = %p pcats->labels = %p",
                (void *)pcats, (void *)pcats->labels);
        Rast_set_d_cat(&d1, &d2, descr, pcats);
    }
    Rast_free_cats(&save_cats);
    G_free(indexes);

    return 0;
}

static int cmp(const void *aa, const void *bb)
{
    const int *a = aa, *b = bb;
    DCELL min_rast1, min_rast2, max_rast1, max_rast2;
    CELL index;

    Rast_quant_get_ith_rule(&(save_cats.q), *a, &min_rast1, &max_rast1, &index,
                            &index);
    Rast_quant_get_ith_rule(&(save_cats.q), *b, &min_rast2, &max_rast2, &index,
                            &index);
    if (min_rast1 < min_rast2)
        return -1;
    if (min_rast1 > min_rast2)
        return 1;
    return 0;
}
