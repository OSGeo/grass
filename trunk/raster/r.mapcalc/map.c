
#include <grass/config.h>

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster.h>
#include <grass/btree.h>
#include <grass/glocale.h>
#include <grass/calc.h>

#include "mapcalc.h"
#include "globals.h"
#include "globals2.h"

/****************************************************************************/

static void prepare_region_from_maps(expression **, int, int);
int columns;
struct Cell_head current_region2;

void setup_region(void)
{
    G_get_window(&current_region2);

    rows = Rast_window_rows();
    columns = Rast_window_cols();
    calc_init(columns);
    depths = 1;
}

/****************************************************************************/

struct sub_cache
{
    int row;
    char *valid;
    void **buf;
};

struct row_cache
{
    int fd;
    int nrows;
    struct sub_cache *sub[3];
};

struct map
{
    const char *name;
    const char *mapset;
    int have_cats;
    int have_colors;
    int use_rowio;
    int min_row, max_row;
    int fd;
    struct Categories cats;
    struct Colors colors;
    BTREE btree;
    struct row_cache cache;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t mutex;
#endif
};

/****************************************************************************/

static struct map *maps;
static int num_maps;
static int max_maps;
static int masking;

static int min_row = INT_MAX;
static int max_row = -INT_MAX;
static int min_col = INT_MAX;
static int max_col = -INT_MAX;

static int max_rows_in_memory = 8;

#ifdef HAVE_PTHREAD_H
static pthread_mutex_t cats_mutex;
static pthread_mutex_t mask_mutex;
#endif

/****************************************************************************/

static void read_row(int fd, void *buf, int row, int res_type)
{
#ifdef HAVE_PTHREAD_H
    if (masking)
	pthread_mutex_lock(&mask_mutex);
#endif
    Rast_get_row(fd, buf, row, res_type);
#ifdef HAVE_PTHREAD_H
    if (masking)
	pthread_mutex_unlock(&mask_mutex);
#endif
}

static void cache_sub_init(struct row_cache *cache, int data_type)
{
    struct sub_cache *sub = G_malloc(sizeof(struct sub_cache));
    int i;

    sub->row = -cache->nrows;
    sub->valid = G_calloc(cache->nrows, 1);
    sub->buf = G_malloc(cache->nrows * sizeof(void *));
    for (i = 0; i < cache->nrows; i++)
	sub->buf[i] = Rast_allocate_buf(data_type);

    cache->sub[data_type] = sub;
}

static void cache_setup(struct row_cache *cache, int fd, int nrows)
{
    cache->fd = fd;
    cache->nrows = nrows;
    cache->sub[CELL_TYPE] = NULL;
    cache->sub[FCELL_TYPE] = NULL;
    cache->sub[DCELL_TYPE] = NULL;
}

static void cache_release(struct row_cache *cache)
{
    int t;

    for (t = 0; t < 3; t++) {
	struct sub_cache *sub = cache->sub[t];
	int i;

	if (!sub)
	    continue;

	for (i = 0; i < cache->nrows; i++)
	    G_free(sub->buf[i]);

	G_free(sub->buf);
	G_free(sub->valid);

	G_free(sub);
    }
}

static void *cache_get_raw(struct row_cache *cache, int row, int data_type)
{
    struct sub_cache *sub;
    void **tmp;
    char *vtmp;
    int i, j;
    int newrow;

    if (!cache->sub[data_type])
	cache_sub_init(cache, data_type);
    sub = cache->sub[data_type];

    i = row - sub->row;

    if (i >= 0 && i < cache->nrows) {
	if (!sub->valid[i]) {
	    read_row(cache->fd, sub->buf[i], row, data_type);
	    sub->valid[i] = 1;
	}
	return sub->buf[i];
    }

    if (i <= -cache->nrows || i >= cache->nrows * 2 - 1) {
	memset(sub->valid, 0, cache->nrows);
	sub->row = row;
	read_row(cache->fd, sub->buf[0], row, data_type);
	sub->valid[0] = 1;
	return sub->buf[0];
    }

    tmp = G_alloca(cache->nrows * sizeof(void *));
    memcpy(tmp, sub->buf, cache->nrows * sizeof(void *));
    vtmp = G_alloca(cache->nrows);
    memcpy(vtmp, sub->valid, cache->nrows);

    i = (i < 0)
	? 0
	: cache->nrows - 1;
    newrow = row - i;

    for (j = 0; j < cache->nrows; j++) {
	int r = newrow + j;
	int k = r - sub->row;
	int l = (k + cache->nrows) % cache->nrows;

	sub->buf[j] = tmp[l];
	sub->valid[j] = k >= 0 && k < cache->nrows && vtmp[l];
    }

    sub->row = newrow;
    G_freea(tmp);
    G_freea(vtmp);

    read_row(cache->fd, sub->buf[i], row, data_type);
    sub->valid[i] = 1;

    return sub->buf[i];
}

static void cache_get(struct row_cache *cache, void *buf, int row, int res_type)
{
    void *p = cache_get_raw(cache, row, res_type);
    memcpy(buf, p, columns * Rast_cell_size(res_type));
}

/****************************************************************************/

static int compare_ints(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

static void init_colors(struct map *m)
{
    if (Rast_read_colors((char *)m->name, (char *)m->mapset, &m->colors) < 0)
	G_fatal_error(_("Unable to read color file for raster map <%s@%s>"),
		      m->name, m->mapset);

    m->have_colors = 1;
}

static void init_cats(struct map *m)
{
    if (Rast_read_cats((char *)m->name, (char *)m->mapset, &m->cats) < 0)
	G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		      m->name, m->mapset);

    if (!btree_create(&m->btree, compare_ints, 1))
	G_fatal_error(_("Unable to create btree for raster map <%s@%s>"),
		      m->name, m->mapset);

    m->have_cats = 1;
}

static void translate_from_colors(struct map *m, DCELL *rast, CELL *cell,
				  int ncols, int mod)
{
    unsigned char *red = G_alloca(columns);
    unsigned char *grn = G_alloca(columns);
    unsigned char *blu = G_alloca(columns);
    unsigned char *set = G_alloca(columns);
    int i;

    Rast_lookup_d_colors(rast, red, grn, blu, set, ncols, &m->colors);

    switch (mod) {
    case 'r':
	for (i = 0; i < ncols; i++)
	    cell[i] = red[i];
	break;
    case 'g':
	for (i = 0; i < ncols; i++)
	    cell[i] = grn[i];
	break;
    case 'b':
	for (i = 0; i < ncols; i++)
	    cell[i] = blu[i];
	break;
    case '#':			/* grey (backwards compatible) */
	/* old weightings: R=0.177, G=0.813, B=0.011 */
	for (i = 0; i < ncols; i++)
	    cell[i] =
		(181 * red[i] + 833 * grn[i] + 11 * blu[i] + 512) / 1024;
	break;
    case 'y':			/* grey (NTSC) */
	/* NTSC weightings: R=0.299, G=0.587, B=0.114 */
	for (i = 0; i < ncols; i++)
	    cell[i] =
		(306 * red[i] + 601 * grn[i] + 117 * blu[i] + 512) / 1024;
	break;
    case 'i':			/* grey (equal weight) */
	for (i = 0; i < ncols; i++)
	    cell[i] = (red[i] + grn[i] + blu[i]) / 3;
	break;
    case 'M':
    case '@':
    default:
	G_fatal_error(_("Invalid map modifier: '%c'"), mod);
	break;
    }

    G_freea(red);
    G_freea(grn);
    G_freea(blu);
    G_freea(set);
}

/* convert cell values to double based on the values in the
 * category file.
 *
 * This requires performing sscanf() of the category label
 * and only do it it for new categories. Must maintain
 * some kind of maps of already scaned values.
 *
 * This maps is a hybrid tree, where the data in each node
 * of the tree is an array of, for example, 64 values, and
 * the key of the tree is the category represented by the
 * first index of the data
 *
 * To speed things up a little, use shifts instead of divide or multiply
 * to compute the key and the index
 *
 * This uses the BTREE library to manage the tree itself
 * btree structure must already be initialized
 * pcats structure must already contain category labels
 */

#define SHIFT 6
#define NCATS (1<<SHIFT)

static void translate_from_cats(struct map *m, CELL * cell, DCELL * xcell,
				int ncols)
{
    struct Categories *pcats;
    BTREE *btree;
    int i, idx;
    CELL cat, key;
    double vbuf[1 << SHIFT];
    double *values;
    void *ptr;
    char *label;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&cats_mutex);
#endif

    btree = &m->btree;
    pcats = &m->cats;

    for (; ncols-- > 0; cell++, xcell++) {
	cat = *cell;
	if (IS_NULL_C(cell)) {
	    SET_NULL_D(xcell);
	    continue;
	}

	/* compute key as cat/NCATS * NCATS, adjusting down for negatives
	 * and idx so that key+idx == cat
	 */
	if (cat < 0)
	    key = -(((-cat - 1) >> SHIFT) << SHIFT) - NCATS;
	else
	    key = (cat >> SHIFT) << SHIFT;
	idx = cat - key;

	/* If key not already in the tree, sscanf() all cats for this key
	 * and put them into the tree
	 */
	if (!btree_find(btree, &key, &ptr)) {
	    values = vbuf;
	    for (i = 0; i < NCATS; i++) {
		CELL cat = i + key;
		if ((label = Rast_get_c_cat(&cat, pcats)) == NULL
		    || sscanf(label, "%lf", values) != 1)
		    SET_NULL_D(values);
		values++;
	    }

	    values = vbuf;
	    btree_update(btree, &key, sizeof(key), vbuf, sizeof(vbuf));
	}
	else
	    values = ptr;

	/* and finally lookup the translated value */
	if (IS_NULL_D(&values[idx]))
	    SET_NULL_D(xcell);
	else
	    *xcell = values[idx];
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&cats_mutex);
#endif
}

static void setup_map(struct map *m)
{
    int nrows = m->max_row - m->min_row + 1;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&m->mutex, NULL);
#endif

    if (nrows > 1 && nrows <= max_rows_in_memory) {
	cache_setup(&m->cache, m->fd, nrows);
	m->use_rowio = 1;
    }
    else
	m->use_rowio = 0;
}

static void read_map(struct map *m, void *buf, int res_type, int row, int col)
{
    CELL *ibuf = buf;
    FCELL *fbuf = buf;
    DCELL *dbuf = buf;

    if (row < 0 || row >= rows) {
	int i;

	switch (res_type) {
	case CELL_TYPE:
	    for (i = 0; i < columns; i++)
		SET_NULL_C(&ibuf[i]);
	    break;
	case FCELL_TYPE:
	    for (i = 0; i < columns; i++)
		SET_NULL_F(&fbuf[i]);
	    break;
	case DCELL_TYPE:
	    for (i = 0; i < columns; i++)
		SET_NULL_D(&dbuf[i]);
	    break;
	default:
	    G_fatal_error(_("Unknown type: %d"), res_type);
	    break;
	}

	return;
    }

    if (m->use_rowio)
	cache_get(&m->cache, buf, row, res_type);
    else
	read_row(m->fd, buf, row, res_type);

    if (col)
	column_shift(buf, res_type, col);
}

static void close_map(struct map *m)
{
    if (m->fd < 0)
	return;

    Rast_close(m->fd);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&m->mutex);
#endif

    if (m->have_cats) {
	btree_free(&m->btree);
	Rast_free_cats(&m->cats);
	m->have_cats = 0;
    }

    if (m->have_colors) {
	Rast_free_colors(&m->colors);
	m->have_colors = 0;
    }

    if (m->use_rowio) {
	cache_release(&m->cache);
	m->use_rowio = 0;
    }
}

/****************************************************************************/

int map_type(const char *name, int mod)
{
    const char *mapset;
    char *tmpname;
    int result;

    switch (mod) {
    case 'M':
	tmpname = G_store((char *)name);
	mapset = G_find_raster2(tmpname, "");
	result = mapset ? Rast_map_type(tmpname, mapset) : -1;
	G_free(tmpname);
	return result;
    case '@':
	return DCELL_TYPE;
    case 'r':
    case 'g':
    case 'b':
    case '#':
    case 'y':
    case 'i':
	return CELL_TYPE;
    default:
	G_fatal_error(_("Invalid map modifier: '%c'"), mod);
	return -1;
    }
}

int open_map(const char *name, int mod, int row, int col)
{
    int i;
    const char *mapset;
    int use_cats = 0;
    int use_colors = 0;
    struct map *m;

    if (row < min_row)
	min_row = row;
    if (row > max_row)
	max_row = row;
    if (col < min_col)
	min_col = col;
    if (col > max_col)
	max_col = col;

    mapset = G_find_raster2(name, "");
    if (!mapset)
	G_fatal_error(_("Raster map <%s> not found"), name);

    switch (mod) {
    case 'M':
	break;
    case '@':
	use_cats = 1;
	break;
    case 'r':
    case 'g':
    case 'b':
    case '#':
    case 'y':
    case 'i':
	use_colors = 1;
	break;
    default:
	G_fatal_error(_("Invalid map modifier: '%c'"), mod);
	break;
    }

    for (i = 0; i < num_maps; i++) {
	m = &maps[i];

	if (strcmp(m->name, name) != 0 || strcmp(m->mapset, mapset) != 0)
	    continue;

	if (row < m->min_row)
	    m->min_row = row;
	if (row > m->max_row)
	    m->max_row = row;

	if (use_cats && !m->have_cats)
	    init_cats(m);

	if (use_colors && !m->have_colors)
	    init_colors(m);

	return i;
    }


    if (num_maps >= max_maps) {
	max_maps += 10;
	maps = G_realloc(maps, max_maps * sizeof(struct map));
    }

    m = &maps[num_maps];

    m->name = name;
    m->mapset = mapset;
    m->have_cats = 0;
    m->have_colors = 0;
    m->use_rowio = 0;
    m->min_row = row;
    m->max_row = row;
    m->fd = -1;

    if (use_cats)
	init_cats(m);
    if (use_colors)
	init_colors(m);

    m->fd = Rast_open_old(name, mapset);

    return num_maps++;
}

void setup_maps(void)
{
    int i;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&cats_mutex, NULL);
    pthread_mutex_init(&mask_mutex, NULL);
    masking = Rast_maskfd() >= 0;
#endif

    for (i = 0; i < num_maps; i++)
	setup_map(&maps[i]);
}

void get_map_row(int idx, int mod, int depth, int row, int col, void *buf,
		 int res_type)
{
    CELL *ibuf;
    DCELL *fbuf;
    struct map *m = &maps[idx];

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&m->mutex);
#endif

    switch (mod) {
    case 'M':
	read_map(m, buf, res_type, row, col);
	break;
    case '@':
	ibuf = G_alloca(columns * sizeof(CELL));
	read_map(m, ibuf, CELL_TYPE, row, col);
	translate_from_cats(m, ibuf, buf, columns);
	G_freea(ibuf);
	break;
    case 'r':
    case 'g':
    case 'b':
    case '#':
    case 'y':
    case 'i':
	fbuf = G_alloca(columns * sizeof(DCELL));
	read_map(m, fbuf, DCELL_TYPE, row, col);
	translate_from_colors(m, fbuf, buf, columns, mod);
	G_freea(fbuf);
	break;
    default:
	G_fatal_error(_("Invalid map modifier: '%c'"), mod);
	break;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&m->mutex);
#endif
}

void close_maps(void)
{
    int i;

    for (i = 0; i < num_maps; i++)
	close_map(&maps[i]);

    num_maps = 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&cats_mutex);
    pthread_mutex_destroy(&mask_mutex);
#endif
}

void list_maps(FILE *fp, const char *sep)
{
    int i;

    for (i = 0; i < num_maps; i++) {
        const struct map *m = &maps[i];
        fprintf(fp, "%s%s@%s", i ? sep : "", m->name, m->mapset);
    }
}

/****************************************************************************/

int check_output_map(const char *name)
{
    return !!G_find_raster2(name, G_mapset());
}

int open_output_map(const char *name, int res_type)
{
    return Rast_open_new((char *)name, res_type);
}

void put_map_row(int fd, void *buf, int res_type)
{
    Rast_put_row(fd, buf, res_type);
}

void close_output_map(int fd)
{
    Rast_close(fd);
}

void unopen_output_map(int fd)
{
    Rast_unopen(fd);
}

/****************************************************************************/

void copy_cats(const char *dst, int idx)
{
    const struct map *m = &maps[idx];
    struct Categories cats;

    if (Rast_read_cats((char *)m->name, (char *)m->mapset, &cats) < 0)
	return;

    Rast_write_cats((char *)dst, &cats);
    Rast_free_cats(&cats);
}

void copy_colors(const char *dst, int idx)
{
    const struct map *m = &maps[idx];
    struct Colors colr;

    if (Rast_read_colors((char *)m->name, (char *)m->mapset, &colr) <= 0)
	return;

    Rast_write_colors((char *)dst, G_mapset(), &colr);
    Rast_free_colors(&colr);
}

void copy_history(const char *dst, int idx)
{
    const struct map *m = &maps[idx];
    struct History hist;

    if (Rast_read_history((char *)m->name, (char *)m->mapset, &hist) < 0)
	return;

    Rast_write_history((char *)dst, &hist);
}

void create_history(const char *dst, expression * e)
{
    int RECORD_LEN = 80;
    int WIDTH = RECORD_LEN - 12;
    struct History hist;
    char *expr = format_expression(e);
    char *p = expr;
    int len = strlen(expr);
    int i;

    Rast_short_history(dst, "raster", &hist);

    for (i = 0; ; i++) {
	char buf[RECORD_LEN];
	int n;

	if (!len)
	    break;

	if (len > WIDTH) {
	    for (n = WIDTH; n > 0 && p[n] != ' '; n--) ;

	    if (n <= 0)
		n = WIDTH;
	    else
		n++;
	}
	else
	    n = len;

	memcpy(buf, p, n);
	buf[n] = '\0';
	Rast_append_history(&hist, buf);

	p += n;
	len -= n;
    }

    if (seeded) {
	char buf[RECORD_LEN];
	sprintf(buf, "random seed = %ld", seed_value);
	Rast_append_history(&hist, buf);
    }

    Rast_write_history(dst, &hist);

    G_free(expr);
}

/****************************************************************************/

void prepare_region_from_maps_union(expression **map_list, int num_maps) {
    prepare_region_from_maps(map_list, 1, num_maps);
}

void prepare_region_from_maps_intersect(expression **map_list, int num_maps) {
    prepare_region_from_maps(map_list, 2, num_maps);
}

void prepare_region_from_maps(expression **map_list, int type, int num_maps) {
    /* \brief Setup the computational region from all raster maps
     *
     * This function computes the disjoint union or intersection extent
     * that is covered by all raster maps and sets the smallest
     * resolution that it can find.
     */

    int first = 0;
    struct Cell_head window, temp_window;
    int i;
    const char *mapset;

    for (i = 0; i < num_maps; i++) {
        expression *e = map_list[i];

        char rast_name[GNAME_MAX];

        strcpy(rast_name, e->data.map.name);
        mapset = G_find_raster2(rast_name, "");

        if (!mapset)
            G_fatal_error(_("Raster map <%s> not found"), rast_name);

        G_debug(1, "Setting region from raster map: %s@%s", rast_name, mapset);

        Rast_get_cellhd(rast_name, mapset, &temp_window);

        if (!first) {
            window = temp_window;
            first = 1;
        }
        else {
            if (type == 1) {
                /* Union: find the largest extent */
                window.north = (window.north > temp_window.north) ?
                    window.north : temp_window.north;
                window.south = (window.south < temp_window.south) ?
                    window.south : temp_window.south;
                window.east = (window.east > temp_window.east) ?
                    window.east : temp_window.east;
                window.west = (window.west < temp_window.west) ?
                    window.west : temp_window.west;
            }
            else {
                /* Intersect: Find the smallest extent */
                window.north = (window.north < temp_window.north) ?
                    window.north : temp_window.north;
                window.south = (window.south > temp_window.south) ?
                    window.south : temp_window.south;
                window.east = (window.east < temp_window.east) ?
                    window.east : temp_window.east;
                window.west = (window.west > temp_window.west) ?
                    window.west : temp_window.west;
            }
            /* Find the smallest resolution */
            window.ns_res = (window.ns_res < temp_window.ns_res) ?
                window.ns_res : temp_window.ns_res;
            window.ew_res = (window.ew_res < temp_window.ew_res) ?
                window.ew_res : temp_window.ew_res;
        }
    }

    /* Set the region only if a map was found in the expression */
    if (first == 1) {
        G_adjust_Cell_head3(&window, 0, 0, 0);
        G_debug(1, "Region was set to n %g s %g e %g w %g ns_res %g ew_res %g", window.north,
                  window.south, window.east, window.west, window.ns_res, window.ew_res);
        G_put_window(&window);
    }

}
/****************************************************************************/
