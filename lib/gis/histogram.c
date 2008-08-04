#include <grass/gis.h>
#include <grass/glocale.h>
#include <stdlib.h>


#define LIST struct Histogram_list

static FILE *fopen_histogram_new(const char *);
static int cmp(const void *, const void *);
static int cmp_count(const void *, const void *);


/*!
 * \brief initializes the histogram structure
 * 
 * initializes the histogram structure for calls to G_set_histogram()
 * and G_add_histogram()
 * \param  histogram
 * \return int
 */

int G_init_histogram(struct Histogram *histogram)
{
    histogram->num = 0;
    histogram->list = NULL;

    return 0;
}


/*!
 * \brief read the histogram information
 *
 *  Reads the histogram information associated with map layer "map"
 *  in mapset "mapset" into the structure "histogram".
 *
 *  note:   a warning message is printed if the file is missing or incorrect
 * \param name: name of map
 * \param mapset: mapset that map belongs to 
 * \param histogram: struct for histogram
 * \return 1  if successful,
 *               0  if no histogram file,
 *              -1  on fail
 */

int G_read_histogram(const char *name, const char *mapset,
		     struct Histogram *histogram)
{
    FILE *fd = NULL;
    long cat;
    long count;
    char buf[200];

    G_init_histogram(histogram);

    if (G_find_file2_misc("cell_misc", "histogram", name, mapset) == NULL) {
	G_warning(_("Histogram for [%s in %s] missing (run r.support)"), name,
		  mapset);

	return 0;
    }

    fd = G_fopen_old_misc("cell_misc", "histogram", name, mapset);
    if (!fd) {
	G_warning(_("Can't read histogram for [%s in %s]"), name, mapset);

	return -1;
    }

    while (fgets(buf, sizeof buf, fd)) {
	if (sscanf(buf, "%ld:%ld", &cat, &count) != 2) {
	    G_free_histogram(histogram);
	    fclose(fd);
	    G_warning(_("Invalid histogram file for [%s in %s]"), name,
		      mapset);

	    return -1;
	}
	G_extend_histogram((CELL) cat, count, histogram);
    }
    fclose(fd);

    if (histogram->num == 0) {
	G_warning(_("Invalid histogram file for [%s in %s]"), name, mapset);

	return -1;
    }

    G_sort_histogram(histogram);

    return 1;
}


/*!
 * \brief Writes the histogram information
 *
 *  Writes the histogram information associated with map layer "name"
 * \param name: name of map
 * \param histogram: struct for histogram
 * \return  1  if successful,
 *              -1  on fail
 */

int G_write_histogram(const char *name, const struct Histogram *histogram)
{
    FILE *fd;
    int n;
    LIST *list;

    fd = fopen_histogram_new(name);
    if (fd == NULL)
	return -1;

    list = histogram->list;
    for (n = 0; n < histogram->num; n++) {
	if (list[n].count)
	    fprintf(fd, "%ld:%ld\n", (long)list[n].cat, list[n].count);
    }
    fclose(fd);

    return 1;
}


/*!
 * \brief Writes the histogram based on cell statistics to file
 *
 * \param name: name of map
 * \param statf: cell statistics
 * \return 1 on success
 *        -1 on failure
 */

int G_write_histogram_cs(const char *name, struct Cell_stats *statf)
{
    FILE *fd;
    CELL cat;
    long count;

    fd = fopen_histogram_new(name);
    if (fd == NULL)
	return -1;

    G_rewind_cell_stats(statf);
    while (G_next_cell_stat(&cat, &count, statf)) {
	if (count > 0)
	    fprintf(fd, "%ld:%ld\n", (long)cat, count);
    }
    fclose(fd);

    return 1;
}


/*!
 * \brief Creates histogram based on cell statistics
 *
 * \param statf: cell statistics
 * \param histogram: raster histogram
 * \return 0 
 */
int G_make_histogram_cs(struct Cell_stats *statf, struct Histogram *histogram)
{
    CELL cat;
    long count;

    G_init_histogram(histogram);
    G_rewind_cell_stats(statf);
    while (G_next_cell_stat(&cat, &count, statf))
	G_add_histogram(cat, count, histogram);

    G_sort_histogram(histogram);

    return 0;
}


/*!
 * \brief Sorts the histogram in ascending order by counts then category
 *
 *  Sorts the histogram in ascending order by counts then category.
 *  No combining is done.
 * \param histogram: struct for histogram
 * \return  1  if successful,
 *              -1  on fail
 */
int G_get_histogram_num(const struct Histogram *histogram)
{
    return histogram->num;
}


/*!
 * \brief Returns cat for the nth element in the histogram
 *
 *  Returns cat for the nth element in the histogram
 * \param histogram: struct for histogram
 * \return CELL
 */
CELL G_get_histogram_cat(int n, const struct Histogram * histogram)
{
    if (n < 0 || n >= histogram->num)
	return 0;

    return histogram->list[n].cat;
}


/*!
 * \brief Returns count for the nth element in the histogram
 *
 *  Returns count for the nth element in the histogram
 * \param n: nth element
 * \param histogram: struct for histogram
 * \return count
 */
long G_get_histogram_count(int n, const struct Histogram *histogram)
{
    if (n < 0 || n >= histogram->num)
	return 0;

    return histogram->list[n].count;
}


/*!
 * \brief Frees memory allocated for the histogram
 *
 * frees the memory allocated for the histogram
 * \param histogram: struct for histogram
 * \return 
 */
int G_free_histogram(struct Histogram *histogram)
{
    if (histogram->num > 0)
	G_free(histogram->list);
    histogram->num = 0;
    histogram->list = NULL;

    return 1;
}

/*!
 * \brief Sorts the histogram
 *
 *  Sorts the histogram in ascending order by category,
 *  combining (by adding) elements that have the same category.
 * \param histogram: struct for histogram
 * \return  0  if successful,
 *              1  on fail
 */
int G_sort_histogram(struct Histogram *histogram)
{
    int a, b, n;
    LIST *list;

    /* if histogram only has 1 entry, nothing to do */
    if ((n = histogram->num) <= 1)
	return 1;

    list = histogram->list;

    /* quick check to see if sorting needed */
    for (a = 1; a < n; a++)
	if (list[a - 1].cat >= list[a].cat)
	    break;
    if (a >= n)
	return 1;

    /* sort */
    qsort(list, n, sizeof(LIST), &cmp);

    /* sum duplicate entries */
    for (a = 0, b = 1; b < n; b++) {
	if (list[a].cat != list[b].cat) {
	    a++;
	    list[a].count = list[b].count;
	    list[a].cat = list[b].cat;
	}
	else {
	    list[a].count += list[b].count;
	}
    }
    histogram->num = a + 1;

    return 0;
}


static int cmp(const void *aa, const void *bb)
{
    const LIST *a = aa, *b = bb;

    if (a->cat < b->cat)
	return -1;

    if (a->cat > b->cat)
	return 1;

    return 0;
}

/*!
 * \brief Sorts the histogram by counts
 *
 *  Sorts the histogram in ascending order by counts then category.
 *  No combining is done.
 * \param histogram: struct for histogram
 * \return  0  if successful,
 *              1  on fail
 */
int G_sort_histogram_by_count(struct Histogram *histogram)
{
    int n;
    LIST *list;

    /* if histogram only has 1 entry, nothing to do */
    if ((n = histogram->num) <= 1)
	return 1;

    list = histogram->list;

    /* sort */
    qsort(list, n, sizeof(LIST), &cmp_count);

    return 0;
}


static int cmp_count(const void *aa, const void *bb)
{
    const LIST *a = aa, *b = bb;

    if (a->count < b->count)
	return -1;

    if (a->count > b->count)
	return 1;

    if (a->cat < b->cat)
	return -1;

    if (a->cat > b->cat)
	return 1;

    return 0;
}

static FILE *fopen_histogram_new(const char *name)
{
    FILE *fd;

    fd = G_fopen_new_misc("cell_misc", "histogram", name);
    if (fd == NULL)
	G_warning(_("can't create histogram for [%s in %s]"), name,
		  G_mapset());

    return fd;
}


/*!
 * \brief Removes the histogram
 *
 *  Removes the histogram information associated with map layer "name"
 * \param name: name of map
 * \return 0
 */

int G_remove_histogram(const char *name)
{
    G_remove_misc("cell_misc", "histogram", name);

    return 0;
}


/*!
 * \brief adds count to the histogram value for cat
 *
 *  adds count to the histogram value for cat
 * \param cat: category
 * \param count
 * \param histogram: struct for histogram
 * \return 0  if successful,
 *              1  on fail
 */
int G_add_histogram(CELL cat, long count, struct Histogram *histogram)
{
    int i;

    for (i = 0; i < histogram->num; i++) {
	if (histogram->list[i].cat == cat) {
	    histogram->list[i].count += count;
	    return 1;
	}
    }
    G_extend_histogram(cat, count, histogram);

    return 0;
}


/*!
 * \brief sets the histogram value for cat to count
 *
 *  sets the histogram value for cat to count
 * \param cat: category
 * \param count
 * \param histogram: struct for histogram
 * \return 0  if successful,
 *              1  on fail
 */
int G_set_histogram(CELL cat, long count, struct Histogram *histogram)
{
    int i;

    for (i = 0; i < histogram->num; i++) {
	if (histogram->list[i].cat == cat) {
	    histogram->list[i].count = count;
	    return 1;
	}
    }
    G_extend_histogram(cat, count, histogram);

    return 0;
}


/*!
 * \brief Extends histogram struct to accomodate a new value
 *
 * \param cat: category
 * \param count
 * \param histogram: struct for histogram
 * \return 
 */
int G_extend_histogram(CELL cat, long count, struct Histogram *histogram)
{
    histogram->num++;
    histogram->list =
	(LIST *) G_realloc((char *)histogram->list,
			   histogram->num * sizeof(LIST));
    histogram->list[histogram->num - 1].cat = cat;
    histogram->list[histogram->num - 1].count = count;

    return 0;
}


/*!
 * \brief Zero out histogram struct
 *
 * \param histogram: struct for histogram
 * \return 
 */
int G_zero_histogram(struct Histogram *histogram)
{
    int i;

    for (i = 0; i < histogram->num; i++)
	histogram->list[i].count = 0;

    return 0;
}
