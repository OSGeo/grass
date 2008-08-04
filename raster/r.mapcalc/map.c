
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/btree.h>
#include <grass/rowio.h>
#include <grass/glocale.h>

#include "mapcalc.h"
#include "globals.h"
#include "globals2.h"

/****************************************************************************/

struct Cell_head current_region2;

void setup_region(void)
{
    G_get_window(&current_region2);

    rows = G_window_rows();
    columns = G_window_cols();
    depths = 1;
}

/****************************************************************************/

typedef struct map
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
    ROWIO rowio;
} map;

/****************************************************************************/

static map *maps;
static int num_maps;
static int max_maps;

static unsigned char *red, *grn, *blu;
static unsigned char *set;

static int min_row = INT_MAX;
static int max_row = -INT_MAX;
static int min_col = INT_MAX;
static int max_col = -INT_MAX;

static int max_rows_in_memory = 3;

static int read_row_type;

/****************************************************************************/

static int compare_ints(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

static void init_colors(map * m)
{
    if (!red)
	red = G_malloc(columns);
    if (!grn)
	grn = G_malloc(columns);
    if (!blu)
	blu = G_malloc(columns);
    if (!set)
	set = G_malloc(columns);

    if (G_read_colors((char *)m->name, (char *)m->mapset, &m->colors) < 0)
	G_fatal_error(_("Unable to read color file for raster map <%s@%s>"),
		      m->name, m->mapset);

    m->have_colors = 1;
}

static void init_cats(map * m)
{
    if (G_read_cats((char *)m->name, (char *)m->mapset, &m->cats) < 0)
	G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		      m->name, m->mapset);

    if (!btree_create(&m->btree, compare_ints, 1))
	G_fatal_error(_("Unable to create btree for raster map <%s@%s>"),
		      m->name, m->mapset);

    m->have_cats = 1;
}

static void translate_from_colors(map * m, DCELL * rast, CELL * cell,
				  int ncols, int mod)
{
    int i;

    G_lookup_d_raster_colors(rast, red, grn, blu, set, ncols, &m->colors);

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
 * btree structure must already be intialized
 * pcats structure must already contain category labels
 */

#define SHIFT 6
#define NCATS (1<<SHIFT)

static void translate_from_cats(map * m, CELL * cell, DCELL * xcell,
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
		if ((label = G_get_cat((CELL) (i + key), pcats)) == NULL
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
}

static void set_read_row_type(int res_type)
{
    read_row_type = res_type;
}

static int read_row(int fd, void *buf, int row, int dummy)
{
    if (G_get_raster_row(fd, (DCELL *) buf, row, read_row_type) < 0)
	G_fatal_error(_("Unable to read raster map row %d"), row);

    return 0;
}

static void setup_map(map * m)
{
    int nrows = m->max_row - m->min_row + 1;
    int size = (sizeof(CELL) > sizeof(double))
	? sizeof(CELL)
	: sizeof(double);

    if (nrows > 1 && nrows <= max_rows_in_memory) {
	if (rowio_setup(&m->rowio, m->fd, nrows,
			columns * size, read_row, NULL) < 0)
	    G_fatal_error(_("Rowio_setup failed"));
	m->use_rowio = 1;
    }
    else
	m->use_rowio = 0;
}

static void read_map(map * m, void *buf, int res_type, int row, int col)
{
    CELL *ibuf = buf;
    FCELL *fbuf = buf;
    DCELL *dbuf = buf;
    void *bp;

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

    set_read_row_type(res_type);

    if (m->use_rowio) {
	bp = rowio_get(&m->rowio, row);
	if (!bp)
	    G_fatal_error(_("Rowio_get failed"));

	G_copy(buf, bp, columns * G_raster_size(res_type));
    }
    else
	read_row(m->fd, buf, row, 0);

    if (col)
	column_shift(buf, res_type, col);
}

static void close_map(map * m)
{
    if (m->fd < 0)
	return;

    if (G_close_cell(m->fd) < 0)
	G_fatal_error(_("Unable to close raster map <%s@%s>"),
		      m->name, m->mapset);

    if (m->have_cats) {
	btree_free(&m->btree);
	G_free_cats(&m->cats);
	m->have_cats = 0;
    }

    if (m->have_colors) {
	G_free_colors(&m->colors);
	m->have_colors = 0;
    }

    if (m->use_rowio) {
	rowio_release(&m->rowio);
	m->use_rowio = 0;
    }
}

/****************************************************************************/

int map_type(const char *name, int mod)
{
    char *mapset, *tmpname;
    int result;

    switch (mod) {
    case 'M':
	tmpname = G_store((char *)name);
	mapset = G_find_cell2(tmpname, "");
	result = mapset ? G_raster_map_type(tmpname, mapset) : -1;
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
    char *mapset;
    int use_cats = 0;
    int use_colors = 0;
    map *m;

    if (row < min_row)
	min_row = row;
    if (row > max_row)
	max_row = row;
    if (col < min_col)
	min_col = col;
    if (col > max_col)
	max_col = col;

    mapset = G_find_cell2((char *)name, "");
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
	maps = G_realloc(maps, max_maps * sizeof(map));
    }

    m = &maps[num_maps];

    m->name = name;
    m->mapset = mapset;
    m->have_cats = 0;
    m->have_colors = 0;
    m->use_rowio = 0;
    m->min_row = row;
    m->max_row = row;

    if (use_cats)
	init_cats(m);
    if (use_colors)
	init_colors(m);

    m->fd = G_open_cell_old((char *)name, mapset);

    if (m->fd < 0)
	G_fatal_error(_("Unable to open raster map <%s@%s>"), name, mapset);

    return num_maps++;
}

void setup_maps(void)
{
    int i;

    for (i = 0; i < num_maps; i++)
	setup_map(&maps[i]);
}

void get_map_row(int idx, int mod, int depth, int row, int col, void *buf,
		 int res_type)
{
    static CELL *ibuf;
    static DCELL *fbuf;
    map *m = &maps[idx];

    switch (mod) {
    case 'M':
	read_map(m, buf, res_type, row, col);
	break;
    case '@':
	if (!ibuf)
	    ibuf = G_malloc(columns * sizeof(CELL));
	read_map(m, ibuf, CELL_TYPE, row, col);
	translate_from_cats(m, ibuf, buf, columns);
	break;
    case 'r':
    case 'g':
    case 'b':
    case '#':
    case 'y':
    case 'i':
	if (!fbuf)
	    fbuf = G_malloc(columns * sizeof(DCELL));
	read_map(m, fbuf, DCELL_TYPE, row, col);
	translate_from_colors(m, fbuf, buf, columns, mod);
	break;
    default:
	G_fatal_error(_("Invalid map modifier: '%c'"), mod);
	break;
    }
}

void close_maps(void)
{
    int i;

    for (i = 0; i < num_maps; i++)
	close_map(&maps[i]);

    num_maps = 0;
}

/****************************************************************************/

int open_output_map(const char *name, int res_type)
{
    int fd;

    fd = G_open_raster_new((char *)name, res_type);
    if (fd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), name);

    return fd;
}

void put_map_row(int fd, void *buf, int res_type)
{
    if (G_put_raster_row(fd, buf, res_type) < 0)
	G_fatal_error(_("Failed writing raster map row"));
}

void close_output_map(int fd)
{
    if (G_close_cell(fd) < 0)
	G_fatal_error(_("Unable to close raster map"));
}

void unopen_output_map(int fd)
{
    G_unopen_cell(fd);
}

/****************************************************************************/

void copy_cats(const char *dst, int idx)
{
    const map *m = &maps[idx];
    struct Categories cats;

    if (G_read_cats((char *)m->name, (char *)m->mapset, &cats) < 0)
	return;

    G_write_cats((char *)dst, &cats);
    G_free_cats(&cats);
}

void copy_colors(const char *dst, int idx)
{
    const map *m = &maps[idx];
    struct Colors colr;

    if (G_read_colors((char *)m->name, (char *)m->mapset, &colr) <= 0)
	return;

    G_write_colors((char *)dst, G_mapset(), &colr);
    G_free_colors(&colr);
}

void copy_history(const char *dst, int idx)
{
    const map *m = &maps[idx];
    struct History hist;

    if (G_read_history((char *)m->name, (char *)m->mapset, &hist) < 0)
	return;

    G_write_history((char *)dst, &hist);
}

void create_history(const char *dst, expression * e)
{
    static int WIDTH = RECORD_LEN - 12;
    struct History hist;
    char *expr = format_expression(e);
    char *p = expr;
    int len = strlen(expr);
    int i;

    G_short_history((char *)dst, "raster", &hist);

    for (i = 0; i < MAXEDLINES; i++) {
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

	memcpy(hist.edhist[i], p, n);
	hist.edhist[i][n] = '\0';

	p += n;
	len -= n;
    }

    hist.edlinecnt = i;

    G_write_history((char *)dst, &hist);

    G_free(expr);
}

/****************************************************************************/
