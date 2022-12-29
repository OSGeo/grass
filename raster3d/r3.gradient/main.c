/****************************************************************************
 * 
 * MODULE:       r3.gradient     
 * AUTHOR(S):    Anna Petrasova kratochanna <at> gmail <dot> com
 * PURPOSE:      Computes gradient of a 3D raster map
 * COPYRIGHT:    (C) 2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/raster3d.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "r3gradient_structs.h"

int main(int argc, char *argv[])
{
    struct Option *input_opt, *output_opt, *block_opt;
    struct GModule *module;
    RASTER3D_Region region;
    RASTER3D_Map *input;
    RASTER3D_Map *output[3];
    struct Gradient_block *blocks;
    int block_x, block_y, block_z;
    int index_x, index_y, index_z;
    int n_x, n_y, n_z;
    int start_x, start_y, start_z;
    int i, max_i, k, j, N;
    double step[3];
    int *bl_indices;
    int *bl_overlap;
    int r, c, d;
    DCELL value;

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("gradient"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Computes gradient of a 3D raster map "
	  "and outputs gradient components as three 3D raster maps.");

    input_opt = G_define_standard_option(G_OPT_R3_INPUT);

    /* TODO: define G_OPT_R3_OUTPUTS or use separate options for each map? */
    output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    output_opt->multiple = YES;
    output_opt->key_desc = "grad_x,grad_y,grad_z";
    output_opt->description = _("Name for output 3D raster map(s)");

    block_opt = G_define_option();
    block_opt->key = "blocksize";
    block_opt->multiple = TRUE;
    block_opt->answer = "30,30,20"; /* based on testing */
    block_opt->key_desc = "size_x,size_y,size_z";
    block_opt->description = _("Size of blocks");

    /* disabled - was there for openMP
    process_opt = G_define_option();
    process_opt->key = "nprocs";
    process_opt->type = TYPE_INTEGER;
    process_opt->required = NO;
    process_opt->description = _("Number of parallel processes");
    process_opt->options = "1-100";
    process_opt->answer = "1";
    */

    G_gisinit(argv[0]);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    N = 1;
    /* disabled - was there for openMP
    N = atoi(process_opt->answer);
#if defined(_OPENMP)
    omp_set_num_threads(N);
#endif
    */

    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    block_x = atoi(block_opt->answers[0]);
    block_y = atoi(block_opt->answers[1]);
    block_z = atoi(block_opt->answers[2]);

    if (block_x < 3 || block_y < 3 || block_z < 3)
	G_warning("block size too small, set to 3");

    block_x = block_x < 3 ? 3 : block_x;
    block_y = block_y < 3 ? 3 : block_y;
    block_z = block_z < 3 ? 3 : block_z;
    block_x = block_x > region.cols ? region.cols : block_x;
    block_y = block_y > region.rows ? region.rows : block_y;
    block_z = block_y > region.depths ? region.depths : block_z;

    step[0] = region.ew_res;
    step[1] = region.ns_res;
    step[2] = region.tb_res;

    input = Rast3d_open_cell_old(input_opt->answer,
				 G_find_raster3d(input_opt->answer, ""),
				 &region, RASTER3D_TILE_SAME_AS_FILE,
				 RASTER3D_USE_CACHE_DEFAULT);
    if (!input)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			   input_opt->answer);

    for (i = 0; i < 3; i++) {
	output[i] =
	    Rast3d_open_new_opt_tile_size(output_opt->answers[i],
					  RASTER3D_USE_CACHE_DEFAULT,
					  &region, DCELL_TYPE, 32);
	if (!output[i]) {
	    Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			       output_opt->answers[i]);
	}

    }

    blocks = G_calloc(N, sizeof(struct Gradient_block));
    if (!blocks)
	G_fatal_error(_("Failed to allocate memory for blocks"));
    for (i = 0; i < N; i++) {
	blocks[i].input.array = G_malloc(((block_x + 2) * (block_y + 2)
					  * (block_z + 2)) * sizeof(DCELL));
	blocks[i].dx.array = G_malloc(((block_x + 2) * (block_y + 2)
				       * (block_z + 2)) * sizeof(DCELL));
	blocks[i].dy.array = G_malloc(((block_x + 2) * (block_y + 2)
				       * (block_z + 2)) * sizeof(DCELL));
	blocks[i].dz.array = G_malloc(((block_x + 2) * (block_y + 2)
				       * (block_z + 2)) * sizeof(DCELL));
    }

    bl_indices = G_calloc(N * 3, sizeof(int));
    bl_overlap = G_calloc(N * 6, sizeof(int));

    max_i = (int)ceil(region.cols / (float)block_x) *
	(int)ceil(region.rows / (float)block_y) *
	(int)ceil(region.depths / (float)block_z);
    i = j = 0;
    index_z = 0;

    /* loop through the blocks */
    while (index_z < region.depths) {
	index_y = 0;
	while (index_y < region.rows) {
	    index_x = 0;
	    while (index_x < region.cols) {
		G_percent(i, max_i, 1);
		/* generally overlap is 1 on both sides */
		bl_overlap[j * 6 + 0] = bl_overlap[j * 6 + 2] =
		    bl_overlap[j * 6 + 4] = 1;
		bl_overlap[j * 6 + 1] = bl_overlap[j * 6 + 3] =
		    bl_overlap[j * 6 + 5] = 1;

		/* compute the starting index of the block and its size */
		start_x = fmax(index_x - 1, 0);
		n_x = fmin(index_x + block_x, region.cols - 1) - start_x + 1;
		start_y = fmax(index_y - 1, 0);
		n_y = fmin(index_y + block_y, region.rows - 1) - start_y + 1;
		start_z = fmax(index_z - 1, 0);
		n_z =
		    fmin(index_z + block_z, region.depths - 1) - start_z + 1;

		/* adjust offset on edges */
		/* start offset */
		if (index_x == 0)
		    bl_overlap[j * 6 + 0] = 0;
		if (index_y == 0)
		    bl_overlap[j * 6 + 2] = 0;
		if (index_z == 0)
		    bl_overlap[j * 6 + 4] = 0;
		/* end offset */
		if (index_x + block_x >= region.cols)
		    bl_overlap[j * 6 + 1] = 0;
		if (index_y + block_y >= region.rows)
		    bl_overlap[j * 6 + 3] = 0;
		if (index_z + block_z >= region.depths)
		    bl_overlap[j * 6 + 5] = 0;
		/* adjust offset when the end block would be too small */
		if (n_x <= 2) {
		    start_x -= 1;
		    n_x += 1;
		    bl_overlap[j * 6 + 0] = 2;
		}
		if (n_y <= 2) {
		    start_y -= 1;
		    n_y += 1;
		    bl_overlap[j * 6 + 2] = 2;
		}
		if (n_z <= 2) {
		    start_z -= 1;
		    n_z += 1;
		    bl_overlap[j * 6 + 4] = 2;
		}
		/* store indices for later writing */
		bl_indices[j * 3 + 0] = index_x;
		bl_indices[j * 3 + 1] = index_y;
		bl_indices[j * 3 + 2] = index_z;

		blocks[j].input.sx = n_x;
		blocks[j].input.sy = n_y;
		blocks[j].input.sz = n_z;
		blocks[j].dx.sx = blocks[j].dy.sx = blocks[j].dz.sx = n_x;
		blocks[j].dx.sy = blocks[j].dy.sy = blocks[j].dz.sy = n_y;
		blocks[j].dx.sz = blocks[j].dy.sz = blocks[j].dz.sz = n_z;

		/* read */
		Rast3d_get_block(input, start_x, start_y, start_z,
				 n_x, n_y, n_z, blocks[j].input.array,
				 DCELL_TYPE);
		if ((j + 1) == N || i == max_i - 1) {

		    /* compute gradient */
		    /* disabled openMP #pragma omp parallel for schedule (static) private (k) */
		    for (k = 0; k <= j; k++) {
			Rast3d_gradient_double(&(blocks[k].input), step,
					       &(blocks[k].dx), &(blocks[k].dy),
					       &(blocks[k].dz));
		    }

		    /* write */
		    for (k = 0; k <= j; k++) {
			for (c = 0;c < blocks[k].input.sx - bl_overlap[k * 6 + 0] -
			     bl_overlap[k * 6 + 1]; c++) {
			    for (r = 0; r < blocks[k].input.sy - bl_overlap[k * 6 + 2] -
				 bl_overlap[k * 6 + 3]; r++) {
				for (d = 0; d < blocks[k].input.sz - bl_overlap[k * 6 + 4] -
				     bl_overlap[k * 6 + 5]; d++) {
				    value = RASTER3D_ARRAY_ACCESS(&(blocks[k].dx),
					      c + bl_overlap[k * 6 + 0],
					      r + bl_overlap[k * 6 + 2],
					      d + bl_overlap[k * 6 + 4]);
				    Rast3d_put_value(output[0],
						     c + bl_indices[k * 3 + 0],
						     r + bl_indices[k * 3 + 1],
						     d + bl_indices[k * 3 + 2],
						     &value, DCELL_TYPE);

				    value = RASTER3D_ARRAY_ACCESS(&(blocks[k].dy),
					     c + bl_overlap[k * 6 + 0],
					     r + bl_overlap[k * 6 + 2],
					     d + bl_overlap[k * 6 + 4]);
				    Rast3d_put_value(output[1],
						     c + bl_indices[k * 3 + 0],
						     r + bl_indices[k * 3 + 1],
						     d + bl_indices[k * 3 + 2],
						     &value, DCELL_TYPE);

				    value = RASTER3D_ARRAY_ACCESS(&(blocks[k].dz),
					     c + bl_overlap[k * 6 + 0],
					     r + bl_overlap[k * 6 + 2],
					     d + bl_overlap[k * 6 + 4]);
				    Rast3d_put_value(output[2],
						     c + bl_indices[k * 3 + 0],
						     r + bl_indices[k * 3 + 1],
						     d + bl_indices[k * 3 + 2],
						     &value, DCELL_TYPE);
				}
			    }
			}
		    }
		    j = -1;
		}
		i++;
		j++;
		index_x += block_x;
	    }
	    index_y += block_y;
	}
	index_z += block_z;
    }
    G_percent(1, 1, 1);
    for (i = 0; i < N; i++) {
	G_free(blocks[i].input.array);
	G_free(blocks[i].dx.array);
	G_free(blocks[i].dy.array);
	G_free(blocks[i].dz.array);
    }
    G_free(blocks);
    G_free(bl_indices);
    G_free(bl_overlap);

    G_message(_("Writing gradient 3D raster maps..."));
    G_percent(0, 3, 1);
    Rast3d_close(output[0]);
    G_percent(1, 3, 1);
    Rast3d_close(output[1]);
    G_percent(2, 3, 1);
    Rast3d_close(output[2]);
    G_percent(1, 1, 1);

    exit(EXIT_SUCCESS);
}
