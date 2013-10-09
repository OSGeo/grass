
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/btree.h>
#include <grass/glocale.h>

#include "mapcalc.h"
#include "globals.h"
#include "globals3.h"

/****************************************************************************/

RASTER3D_Region current_region3;

void setup_region(void)
{
    Rast3d_init_defaults();
    Rast3d_get_window(&current_region3);

    rows = current_region3.rows;
    columns = current_region3.cols;
    depths = current_region3.depths;
}

/****************************************************************************/

typedef struct map
{
    const char *name;
    const char *mapset;
    int have_cats;
    int have_colors;
    int min_row, max_row;
    void *handle;
    int fd;
    struct Categories cats;
    struct Colors colors;
    BTREE btree;
} map;

/****************************************************************************/

static map *maps;
static int num_maps;
static int max_maps;

static void **omaps;
static int num_omaps;
static int max_omaps;

static unsigned char *red, *grn, *blu;
static unsigned char *set;

static int min_row = INT_MAX;
static int max_row = -INT_MAX;
static int min_col = INT_MAX;
static int max_col = -INT_MAX;

/****************************************************************************/

static void read_row(void *handle, char *buf, int type, int depth, int row)
{
    int i;

    switch (type) {
    case CELL_TYPE:
	for (i = 0; i < columns; i++) {
	    double x;

	    Rast3d_get_value(handle, i, row, depth, (char *)&x, DCELL_TYPE);
	    if (Rast3d_is_null_value_num(&x, DCELL_TYPE))
		SET_NULL_C(&((CELL *) buf)[i]);
	    else
		((CELL *) buf)[i] = (CELL) x;
	}
	break;
    case FCELL_TYPE:
	for (i = 0; i < columns; i++) {
	    float x;

	    Rast3d_get_value(handle, i, row, depth, (char *)&x, FCELL_TYPE);
	    if (Rast3d_is_null_value_num(&x, FCELL_TYPE))
		SET_NULL_F(&((FCELL *) buf)[i]);
	    else
		((FCELL *) buf)[i] = x;
	}
	break;
    case DCELL_TYPE:
	for (i = 0; i < columns; i++) {
	    double x;

	    Rast3d_get_value(handle, i, row, depth, (char *)&x, DCELL_TYPE);
	    if (Rast3d_is_null_value_num(&x, DCELL_TYPE))
		SET_NULL_D(&((DCELL *) buf)[i]);
	    else
		((DCELL *) buf)[i] = x;
	}
	break;
    }
}

static void write_row(void *handle, const char *buf, int type, int depth,
		      int row)
{
    int i;

    switch (type) {
    case CELL_TYPE:
	for (i = 0; i < columns; i++) {
	    double x;

	    if (IS_NULL_C(&((CELL *) buf)[i]))
		Rast3d_set_null_value(&x, 1, DCELL_TYPE);
	    else
		x = ((CELL *) buf)[i];

	    if (Rast3d_put_value(handle, i, row, depth, (char *)&x, DCELL_TYPE) <
		0)
		G_fatal_error(_("Error writing data"));
	}
	break;
    case FCELL_TYPE:
	for (i = 0; i < columns; i++) {
	    float x;

	    if (IS_NULL_F(&((FCELL *) buf)[i]))
		Rast3d_set_null_value(&x, 1, FCELL_TYPE);
	    else
		x = ((FCELL *) buf)[i];

	    if (Rast3d_put_value(handle, i, row, depth, (char *)&x, FCELL_TYPE) <
		0)
		G_fatal_error(_("Error writing data"));
	}
	break;
    case DCELL_TYPE:
	for (i = 0; i < columns; i++) {
	    double x;

	    if (IS_NULL_D(&((DCELL *) buf)[i]))
		Rast3d_set_null_value(&x, 1, DCELL_TYPE);
	    else
		x = ((DCELL *) buf)[i];

	    if (Rast3d_put_value(handle, i, row, depth, (char *)&x, DCELL_TYPE) <
		0)
		G_fatal_error(_("Error writing data"));
	}
	break;
    }
}

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

    if (Rast3d_read_colors((char *)m->name, (char *)m->mapset, &m->colors) < 0)
	G_fatal_error(_("Unable to read color file for raster map <%s@%s>"),
		      m->name, m->mapset);

    m->have_colors = 1;
}

static void init_cats(map * m)
{
    if (Rast3d_read_cats((char *)m->name, (char *)m->mapset, &m->cats) < 0)
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
	    int cat = i + key;
	    for (i = 0; i < NCATS; i++) {
		if ((label = Rast_get_c_cat((CELL *) &cat, pcats)) == NULL
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

static void setup_map(map * m)
{
}

static void read_map(map * m, void *buf, int res_type, int depth, int row,
		     int col)
{
    CELL *ibuf = buf;
    FCELL *fbuf = buf;
    DCELL *dbuf = buf;

    if (row < 0 || row >= rows || depth < 0 || depth >= depths) {
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

    read_row(m->handle, buf, res_type, depth, row);

    if (col)
	column_shift(buf, res_type, col);
}

static void close_map(map * m)
{
    if (!m->handle)
	return;

    if (!Rast3d_close(m->handle))
	G_fatal_error(_("Unable to close raster map <%s@%s>"),
		      m->name, m->mapset);

    if (m->have_cats) {
	btree_free(&m->btree);
	Rast_free_cats(&m->cats);
	m->have_cats = 0;
    }

    if (m->have_colors) {
	Rast_free_colors(&m->colors);
	m->have_colors = 0;
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
	mapset = G_find_raster3d(tmpname, "");
	if (mapset) {
	    void *handle;

	    setup_region();	/* TODO: setup_region should be called by evaluate() ? */
	    handle = Rast3d_open_cell_old(tmpname, mapset, &current_region3,
				     RASTER3D_TILE_SAME_AS_FILE, RASTER3D_NO_CACHE);
	    result = (Rast3d_file_type_map(handle) == FCELL_TYPE)
		? FCELL_TYPE : DCELL_TYPE;
	    Rast3d_close(handle);
	}
	else
	    result = -1;
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
    char *tmpname;
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

    tmpname = G_store((char *)name);
    mapset = G_find_raster3d(tmpname, "");
    G_free(tmpname);

    if (!mapset)
	G_fatal_error("open_map: map [%s] not found", name);

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
    m->min_row = row;
    m->max_row = row;

    if (use_cats)
	init_cats(m);
    if (use_colors)
	init_colors(m);

    m->handle = Rast3d_open_cell_old((char *)name, (char *)mapset,
				&current_region3, DCELL_TYPE,
				RASTER3D_USE_CACHE_DEFAULT);

    if (!m->handle)
	G_fatal_error(_("Unable to open raster map <%s>"), name);

    return num_maps++;
}

void setup_maps(void)
{
    int i;

    /* We need to reduce the number of worker threads to one, to
     * avoid that several threads access a single map for reading
     * at the same time. The raster3d library is not thread safe.
     * */
    putenv("WORKERS=1");

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
	read_map(m, buf, res_type, depth, row, col);
	break;
    case '@':
	if (!ibuf)
	    ibuf = G_malloc(columns * sizeof(CELL));
	read_map(m, ibuf, CELL_TYPE, depth, row, col);
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
	read_map(m, fbuf, DCELL_TYPE, depth, row, col);
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

int check_output_map(const char *name)
{
    return !!G_find_raster3d(name, G_mapset());
}

int open_output_map(const char *name, int res_type)
{
    void *handle;

    Rast3d_set_file_type(res_type == FCELL_TYPE ? FCELL_TYPE : DCELL_TYPE);

    handle = Rast3d_open_new_opt_tile_size((char *)name, RASTER3D_USE_CACHE_XYZ, &current_region3, res_type == FCELL_TYPE ? FCELL_TYPE : DCELL_TYPE, 32);

    if (!handle)
	G_fatal_error(_("Unable to create raster map <%s>"), name);


    if (num_omaps >= max_omaps) {
	max_omaps += 10;
	omaps = G_realloc(omaps, max_omaps * sizeof(void *));
    }

    omaps[num_omaps] = handle;

    return num_omaps++;
}

void put_map_row(int fd, void *buf, int res_type)
{
    void *handle = omaps[fd];

    write_row(handle, buf, res_type, current_depth, current_row);
}

void close_output_map(int fd)
{
    void *handle = omaps[fd];

    if (!Rast3d_close(handle))
	G_fatal_error(_("Unable to close output raster map"));
}

void unopen_output_map(int fd)
{
    close_output_map(fd);
}

/****************************************************************************/

void copy_cats(const char *dst, int idx)
{
    const map *m = &maps[idx];
    struct Categories cats;

    if (Rast3d_read_cats((char *)m->name, (char *)m->mapset, &cats) < 0)
	return;

    Rast3d_write_cats((char *)dst, &cats);
    Rast_free_cats(&cats);
}

void copy_colors(const char *dst, int idx)
{
    const map *m = &maps[idx];
    struct Colors colr;

    if (Rast3d_read_colors((char *)m->name, (char *)m->mapset, &colr) <= 0)
	return;

    Rast3d_write_colors((char *)dst, G_mapset(), &colr);
    Rast_free_colors(&colr);
}

void copy_history(const char *dst, int idx)
{
}

void create_history(const char *dst, expression * e)
{
}

/****************************************************************************/
