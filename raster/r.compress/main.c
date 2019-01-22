
/****************************************************************************
 *
 * MODULE:       r.compress
 *
 * AUTHOR(S):    James Westervelt - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Compress and decompress raster map files.
 *
 * COPYRIGHT:    (C) 2003-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/****************************************************************************
 * compress_cell converts straight grid_cell files into compressed grid_cell
 * files.  Compressed files have the following format:
 *
 * RLE:
 *  - Array of addresses pointing to the internal start of each row
 *    First byte of each row is the number of bytes per cell for that row
 *    Remainder of the row is a series of byte groups that describe the data:
 *        First byte: number of cells that contain the category given by second
 *        Next byte(s): category number. The number of bytes is determined
 *                      by the number of bytes in a cell 
 *
 * The normal G_open_cell(), and Rast_put_row() do the compression
 * This program must only check that the file is not a reclass file and
 * is not a GDAL-linked map or a GRASS VRT.
 *
 * The only trick is to preserve the support files
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static int process(char *, int);
static int pprint(char *, int);
static int doit(char *, int, RASTER_MAP_TYPE);

/* compressed null file, see lib/raster/open.c */
#define NULLC_FILE "nullcmpr"

int main(int argc, char *argv[])
{
    int stat;
    int n;
    char *name;
    struct GModule *module;
    struct Option *map;
    struct Flag *uncompress, *pflag, *gflag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("compression"));
    module->description = _("Compresses and decompresses raster maps.");

    map = G_define_standard_option(G_OPT_R_MAPS);
    map->description = _("Name of existing raster map(s)");

    uncompress = G_define_flag();
    uncompress->key = 'u';
    uncompress->description = _("Uncompress the map");

    pflag = G_define_flag();
    pflag->key = 'p';
    pflag->description = _("Print compression information and data type of input map(s)");

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description = _("Print compression information in shell script style");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (pflag->answer || gflag->answer) {
        for (n = 0; (name = map->answers[n]); n++)
            pprint(name, gflag->answer);
        exit(EXIT_SUCCESS);
    }

    stat = EXIT_FAILURE;
    for (n = 0; (name = map->answers[n]); n++)
	if (process(name, uncompress->answer))
	    stat = EXIT_SUCCESS;
    exit(stat);
}


static int process(char *name, int uncompress)
{
    struct Colors colr;
    struct History hist;
    struct Categories cats;
    struct Quant quant;
    int colr_ok;
    int hist_ok;
    int cats_ok;
    int quant_ok = 0;
    off_t newsize, oldsize, diff;
    RASTER_MAP_TYPE map_type;
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];
    const char *mapset;
    int data_fd;
    char *sizestr;

    mapset = G_find_raster(name, G_mapset());
    if (mapset == NULL) {
        G_warning(_("Raster map <%s> not found"), name);
        return 1;
    }
    if (strcmp(mapset, G_mapset())) {
	G_warning(_("Raster map <%s> is not in the current mapset"), name);
	return 1;
    }
    if (Rast_is_reclass(name, G_mapset(), rname, rmapset) > 0) {
	G_warning(uncompress
		  ?
		  _("<%s> is a reclass file of map <%s> in mapset <%s> - can't uncompress")
		  :
		  _("<%s> is a reclass file of map <%s> in mapset <%s> - can't compress"),
		  name, rname, rmapset);
	return 1;
    }
    if (G_find_file2_misc("cell_misc", "gdal", name, G_mapset())) {
	G_warning(_("<%s> is a GDAL-linked map - can't (un)compress"), name);
	return 1;
    }
    if (G_find_file2_misc("cell_misc", "vrt", name, G_mapset())) {
	G_warning(_("<%s> is a virtual raster map - can't (un)compress"), name);
	return 1;
    }

    map_type = Rast_map_type(name, G_mapset());

    colr_ok = Rast_read_colors(name, G_mapset(), &colr) > 0;
    hist_ok = Rast_read_history(name, G_mapset(), &hist) >= 0;
    cats_ok = Rast_read_cats(name, G_mapset(), &cats) >= 0;

    if (map_type != CELL_TYPE) {
	Rast_quant_init(&quant);
	quant_ok = Rast_read_quant(name, G_mapset(), &quant);
	G_suppress_warnings(0);

	data_fd = G_open_old("fcell", name, G_mapset());
    }
    else
	data_fd = G_open_old("cell", name, G_mapset());

    oldsize = lseek(data_fd, (off_t) 0, SEEK_END);
    close(data_fd);

    if (doit(name, uncompress, map_type))
	return 1;

    if (colr_ok) {
	Rast_write_colors(name, G_mapset(), &colr);
	Rast_free_colors(&colr);
    }
    if (hist_ok)
	Rast_write_history(name, &hist);
    if (cats_ok) {
	cats.num = Rast_get_max_c_cat(name, G_mapset());
	Rast_write_cats(name, &cats);
	Rast_free_cats(&cats);
    }
    if (map_type != CELL_TYPE && quant_ok)
	Rast_write_quant(name, G_mapset(), &quant);

    if (map_type != CELL_TYPE) {
	data_fd = G_open_old("fcell", name, G_mapset());
    }
    else
	data_fd = G_open_old("cell", name, G_mapset());

    newsize = lseek(data_fd, (off_t) 0, SEEK_END);
    close(data_fd);

    sizestr = "bytes";
    if (newsize > 100 * 1024 && oldsize > 100 * 1024) {
	newsize /= 1024;
	oldsize /= 1024;
	sizestr = "KB";
    }
    if (newsize > 100 * 1024 && oldsize > 100 * 1024) {
	newsize /= 1024;
	oldsize /= 1024;
	sizestr = "MB";
    }
    if (newsize > 100 * 1024 && oldsize > 100 * 1024) {
	newsize /= 1024;
	oldsize /= 1024;
	sizestr = "GB";
    }

    diff = newsize - oldsize;
    if (diff < 0)
	diff = -diff;
    if (sizeof(off_t) > sizeof(long) && diff > ULONG_MAX)
	diff = ULONG_MAX;

    if (newsize < oldsize)
	G_message(uncompress
		  ? _("DONE: uncompressed file is %lu %s (%.2f%) smaller")
		  : _("DONE: compressed file is %lu %s (%.2f%) smaller"),
		  (unsigned long)diff, sizestr, (double) 100.0 - 100.0 * newsize / oldsize);
    else if (newsize > oldsize)
	G_message(uncompress
		  ? _("DONE: uncompressed file is %lu %s (%.2f%) larger")
		  : _("DONE: compressed file is %lu %s (%.2f%) larger"),
		  (unsigned long)diff, sizestr, (double) 100.0 * newsize / oldsize - 100.0);
    else
	G_message(_("same size"));

    return 0;
}

static int doit(char *name, int uncompress, RASTER_MAP_TYPE map_type)
{
    struct Cell_head cellhd;
    int new, old, nrows, row;
    void *rast;
    char *cname;

    Rast_get_cellhd(name, G_mapset(), &cellhd);
    cname = getenv("GRASS_COMPRESSOR");
    if (cname && *cname) {
       if (G_compressor_number(cname) < 1)
           cname = G_compressor_name(G_default_compressor());
       else
           cname = G_compressor_name(G_compressor_number(cname));
    } else
           cname = G_compressor_name(G_default_compressor());

    /* check if already compressed/decompressed */
    if (uncompress) {
	if (cellhd.compressed == 0) {
	    G_warning(_("<%s> already uncompressed"), name);
	    return 1;
	}
	G_verbose_message(_("Uncompressing <%s>"), name);
    }
    else /* uncompress == 0 */ {
	if (cellhd.compressed > 0) {
	    /*
	    G_warning(_("[%s] already compressed"), name);
	    return 1;
	    */
	    G_message(_("Re-compressing <%s> with method %s..."), name, cname);
	}
	else
	    G_message(_("Compressing <%s> with method %s..."), name, cname);
    }

    Rast_set_window(&cellhd);

    old = Rast_open_old(name, G_mapset());

    if (uncompress) {
	if (map_type == CELL_TYPE) {
	    Rast_set_cell_format(cellhd.format);
	    new = Rast_open_c_new_uncompressed(name);
	}
	else {
	    Rast_set_fp_type(map_type);
	    new = Rast_open_fp_new_uncompressed(name);
	}
    }
    else
	new = Rast_open_new(name, map_type);

    nrows = Rast_window_rows();
    rast = Rast_allocate_buf(map_type);

    /* the null file is written automatically */
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_row_nomask(old, rast, row, map_type);
	Rast_put_row(new, rast, map_type);
    }
    G_free(rast);
    Rast_close(old);
    if (row < nrows) {
	Rast_unopen(new);
	return 1;
    }
    Rast_close(new);

    return 0;
}


static int pprint(char *name, int shell_style)
{
    struct Cell_head cellhd;
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];
    const char *mapset;
    RASTER_MAP_TYPE map_type;

    mapset = G_find_raster(name, G_mapset());
    if (mapset == NULL) {
        G_warning(_("Raster map <%s> not found"), name);
        return 1;
    }
    if (strcmp(mapset, G_mapset())) {
	G_warning(_("Raster map <%s> is not in the current mapset"), name);
	return 1;
    }
    if (G_find_file2_misc("cell_misc", "gdal", name, G_mapset())) {
        G_message(_("<%s> is a GDAL-linked map"), name);
        return 1;
    }
    if (G_find_file2_misc("cell_misc", "vrt", name, G_mapset())) {
        G_message(_("<%s> is a virtual raster map"), name);
        return 1;
    }
    if (Rast_is_reclass(name, G_mapset(), rname, rmapset) > 0) {
        G_message(_("<%s> is a reclass file of map <%s> in mapset <%s>"),
                  name, rname, rmapset);
        return 1;
    }

    Rast_get_cellhd(name, G_mapset(), &cellhd);
    map_type = Rast_map_type(name, G_mapset());

    /* Integer (CELL) compression:
     *    cellhd.compressed == 0: uncompressed
     *    cellhd.compressed == 1: RLE compressed
     *    cellhd.compressed == 2: DEFLATE compressed
     */

    /* No RLE compression for fp maps */
    if (cellhd.compressed == 1 && map_type != CELL_TYPE)
	cellhd.compressed = 2;

    if (!shell_style) {
	if (cellhd.compressed == 0) {
	    G_message(_("<%s> is uncompressed (method %i: %s). Data type: %s"), name, cellhd.compressed,
			"NONE",
			 (map_type == CELL_TYPE ? "CELL" :
			   (map_type == DCELL_TYPE ? "DCELL" :
			     (map_type == FCELL_TYPE ? "FCELL" : "??"))));
	}
	else if (cellhd.compressed > 0) {
	    G_message(_("<%s> is compressed (method %i: %s). Data type: %s"), name, cellhd.compressed,
			G_compressor_name(cellhd.compressed),
			 (map_type == CELL_TYPE ? "CELL" :
			   (map_type == DCELL_TYPE ? "DCELL" :
			     (map_type == FCELL_TYPE ? "FCELL" : "??"))));
	}

	if (G_find_file2_misc("cell_misc", NULLC_FILE, name, G_mapset())) {
	    G_message(_("<%s> has a compressed NULL file"), name);
	}
	else {
	    G_message(_("<%s> has an uncompressed NULL file"), name);
	}
    }
    else {
	fprintf(stdout, "%s|%s", name,
			(map_type == CELL_TYPE ? "CELL" :
			   (map_type == DCELL_TYPE ? "DCELL" :
			     (map_type == FCELL_TYPE ? "FCELL" : "??"))));
	if (cellhd.compressed == 0)
	    fprintf(stdout, "|NONE");
	else
	    fprintf(stdout, "|%s", G_compressor_name(cellhd.compressed));
	if (G_find_file2_misc("cell_misc", NULLC_FILE, name, G_mapset()))
	    fprintf(stdout, "|YES\n");
	else
	    fprintf(stdout, "|NO\n");
    }

    return 0;
}

