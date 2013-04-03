
/****************************************************************************
 *
 * MODULE:       r.tile
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      Retiles an existing raster map with user defined x and y tile size
 * COPYRIGHT:    (C) 2013 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static struct
{
    struct Option *rastin, *rastout, *width, *height, *overlap;
} parm;
static struct Cell_head dst_w, src_w, ovl_w;
static int xtiles, ytiles;
static RASTER_MAP_TYPE map_type;

static void write_support_files(int xtile, int ytile, int overlap);

int main(int argc, char *argv[])
{
    struct GModule *module;
    int infile;
    size_t xtile_size, cell_size;
    int ytile, xtile, y, overlap;
    int *outfiles;
    void *inbuf;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("tiling"));
    module->description =
	_("Splits a raster map into tiles.");

    parm.rastin = G_define_standard_option(G_OPT_R_INPUT);

    parm.rastout = G_define_option();
    parm.rastout->key = "output";
    parm.rastout->type = TYPE_STRING;
    parm.rastout->required = YES;
    parm.rastout->multiple = NO;
    parm.rastout->description = _("Output base name");

    parm.width = G_define_option();
    parm.width->key = "width";
    parm.width->type = TYPE_INTEGER;
    parm.width->required = YES;
    parm.width->multiple = NO;
    parm.width->description = _("Width of tiles");

    parm.height = G_define_option();
    parm.height->key = "height";
    parm.height->type = TYPE_INTEGER;
    parm.height->required = YES;
    parm.height->multiple = NO;
    parm.height->description = _("Height of tiles");

    parm.overlap = G_define_option();
    parm.overlap->key = "overlap";
    parm.overlap->type = TYPE_INTEGER;
    parm.overlap->required = NO;
    parm.overlap->multiple = NO;
    parm.overlap->description = _("Overlap of tiles");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&src_w);
    overlap = parm.overlap->answer ? atoi(parm.overlap->answer) : 0;

    /* set window to old map */
    Rast_get_cellhd(parm.rastin->answer, "", &src_w);
    dst_w = src_w;
    dst_w.cols = atoi(parm.width->answer);
    dst_w.rows = atoi(parm.height->answer);
    G_adjust_Cell_head(&dst_w, 1, 1);

    xtiles = (src_w.cols + dst_w.cols - 1) / dst_w.cols;
    ytiles = (src_w.rows + dst_w.rows - 1) / dst_w.rows;

    G_debug(1, "X: %d * %d, Y: %d * %d",
	    xtiles, dst_w.cols, ytiles, dst_w.rows);

    src_w.cols = xtiles * dst_w.cols + 2 * overlap;
    src_w.rows = ytiles * dst_w.rows + 2 * overlap;
    src_w.west = src_w.west - overlap * src_w.ew_res;
    src_w.east = src_w.west + (src_w.cols + 2 * overlap) * src_w.ew_res;
    src_w.north = src_w.north + overlap * src_w.ns_res;
    src_w.south = src_w.north - (src_w.rows + 2 * overlap) * src_w.ns_res;

    Rast_set_input_window(&src_w);

    /* set the output region */
    ovl_w = dst_w;
    ovl_w.cols = ovl_w.cols + 2 * overlap;
    ovl_w.rows = ovl_w.rows + 2 * overlap;

    G_adjust_Cell_head(&ovl_w, 1, 1);
    Rast_set_output_window(&ovl_w);

    infile = Rast_open_old(parm.rastin->answer, "");
    map_type = Rast_get_map_type(infile);
    cell_size = Rast_cell_size(map_type);

    inbuf = Rast_allocate_input_buf(map_type);

    outfiles = G_malloc(xtiles * sizeof(int));

    G_debug(1, "X: %d * %d, Y: %d * %d",
	    xtiles, dst_w.cols, ytiles, dst_w.rows);

    for (ytile = 0; ytile < ytiles; ytile++) {
	G_debug(1, "reading y tile: %d", ytile);
	for (xtile = 0; xtile < xtiles; xtile++) {
	    char name[GNAME_MAX];
	    sprintf(name, "%s-%03d-%03d", parm.rastout->answer, ytile, xtile);
	    outfiles[xtile] = Rast_open_new(name, map_type);
	}
	
	for (y = 0; y < ovl_w.rows; y++) {
	    int row = ytile * dst_w.rows + y;
	    G_debug(1, "reading row: %d", row);
	    Rast_get_row(infile, inbuf, row, map_type);
	    
	    for (xtile = 0; xtile < xtiles; xtile++) {
		int cells = xtile * dst_w.cols;
		void *ptr = G_incr_void_ptr(inbuf, cells * cell_size);
		Rast_put_row(outfiles[xtile], ptr, map_type);
	    }
	}

	for (xtile = 0; xtile < xtiles; xtile++) {
	    Rast_close(outfiles[xtile]);
	    write_support_files(xtile, ytile, overlap);
	}
    }

    Rast_close(infile);

    return EXIT_SUCCESS;
}

static void write_support_files(int xtile, int ytile, int overlap)
{
    char name[GNAME_MAX];
    struct Cell_head cellhd;
    char title[64];
    struct History history;
    struct Colors colors;
    struct Categories cats;

    sprintf(name, "%s-%03d-%03d", parm.rastout->answer, ytile, xtile);

    Rast_get_cellhd(name, G_mapset(), &cellhd);

    cellhd.north = src_w.north - ytile * dst_w.rows * src_w.ns_res;
    cellhd.south = cellhd.north - (dst_w.rows + 2 * overlap) * src_w.ns_res;
    cellhd.west = src_w.west + xtile * dst_w.cols * src_w.ew_res;
    cellhd.east = cellhd.west + (dst_w.cols + 2 * overlap) * src_w.ew_res;

    Rast_put_cellhd(name, &cellhd);

    /* copy cats from source map */
    if (Rast_read_cats(parm.rastin->answer, "", &cats) < 0)
	G_fatal_error(_("Unable to read cats for %s"),
		      parm.rastin->answer);
    Rast_write_cats(name, &cats);

    /* record map metadata/history info */
    sprintf(title, "Tile %d,%d of %s", xtile, ytile, parm.rastin->answer);
    Rast_put_cell_title(parm.rastout->answer, title);

    Rast_short_history(name, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, parm.rastin->answer);
    Rast_command_history(&history);
    Rast_write_history(name, &history);

    /* copy color table from source map */
    if (Rast_read_colors(parm.rastin->answer, "", &colors) < 0)
	G_fatal_error(_("Unable to read color table for %s"),
		      parm.rastin->answer);
    if (map_type != CELL_TYPE)
	Rast_mark_colors_as_fp(&colors);
    Rast_write_colors(name, G_mapset(), &colors);
}

