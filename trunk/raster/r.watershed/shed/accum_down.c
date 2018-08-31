#include "watershed.h"

int accum_down(OUTPUT * output)
{
    double new_length;
    int m, b, num_basins, down_basin;
    B_FACTS *basin_facts, *basin_info, *down_basin_info;
    CAT *down_cat, *cat;
    MAP *map;

    num_basins = output->num_basins;
    basin_facts = output->basin_facts;
    for (b = num_basins - 1; b >= 0; b--) {
	basin_facts[b].accum_length = basin_facts[b].str_length;
	basin_facts[b].accum_slope = basin_facts[b].str_slope;
    }
    for (b = num_basins - 1; b >= 0; b--) {
	basin_info = &(basin_facts[b]);
	down_basin = basin_info->down_basin;
	if (down_basin >= 0) {
	    down_basin_info = &(basin_facts[down_basin]);
	    down_basin_info->num_cells += basin_info->num_cells;
	    new_length =
		basin_info->accum_length + down_basin_info->str_length;
	    if (new_length > down_basin_info->accum_length) {
		down_basin_info->accum_length = new_length;
		down_basin_info->accum_slope =
		    (down_basin_info->str_slope *
		     down_basin_info->str_length +
		     basin_info->accum_slope * basin_info->accum_length) /
		    new_length;
	    }
	    /* accum map layer information */
	    for (m = 0; m < output->num_maps; m++) {
		map = &(output->maps[m]);
		map->basins[down_basin].sum_values +=
		    map->basins[b].sum_values;
		if (output->maps[m].do_cats != 0) {
		    cat = &(map->basins[b].first_cat);
		    down_cat = &(map->basins[down_basin].first_cat);
		    while (cat != NULL) {
			insert_cat(down_cat, cat->cat_val, cat->num_cat);
			cat = cat->nxt;
		    }
		}
	    }
	}
    }
    for (b = num_basins - 1; b >= 0; b--) {
	basin_facts[b].str_length = basin_facts[b].accum_length;
	basin_facts[b].str_slope = basin_facts[b].accum_slope;
    }

    return 0;
}
