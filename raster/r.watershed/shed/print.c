#include "watershed.h"
#include "string.h"

int print_output(OUTPUT * output)
{
    double cell_size;
    int b, c;
    CAT *do_cat;
    char *cat_name, area[32];

    cell_size = output->window.ns_res * output->window.ew_res;
    for (c = output->num_basins - 1; c >= 0; c--) {
	if (output->basin_facts[c].valid == 1)
	    fprintf(output->out_file,
		    "\nValid Basin: %-5d flows into basin: %-5d at: E=%.1f N=%.1f\n",
		    (c + 1) * 2, (output->basin_facts[c].down_basin + 1) * 2,
		    output->basin_facts[c].easting,
		    output->basin_facts[c].northing);
	else
	    fprintf(output->out_file,
		    "\nInvalid basin: %-5d flows into basin: %-5d at: E=%.1f N=%.1f\n",
		    (c + 1) * 2, (output->basin_facts[c].down_basin + 1) * 2,
		    output->basin_facts[c].easting,
		    output->basin_facts[c].northing);
	fprintf(output->out_file,
		"    Str. length:%-.3f meters, %-.3f feet; Str. slope:%-.4f\n",
		output->basin_facts[c].str_length,
		(double)(output->basin_facts[c].str_length * METER_TO_FOOT),
		output->basin_facts[c].str_slope);
	switch (output->type_area) {
	case 1:
	    fprintf(output->out_file, "    Basin Area acres: %-16.4f",
		    output->basin_facts[c].num_cells * cell_size *
		    METERSQ_TO_ACRE);
	    break;
	case 2:
	    fprintf(output->out_file, "    Basin Area sq. meters: %-11.3f",
		    output->basin_facts[c].num_cells * cell_size);
	    break;
	case 3:
	    fprintf(output->out_file, "    Basin Area miles sq: %-16.5f",
		    output->basin_facts[c].num_cells * cell_size *
		    METERSQ_TO_MILESQ);
	    break;
	case 4:
	    fprintf(output->out_file, "    Basin Area hectareas: %-14.4f",
		    output->basin_facts[c].num_cells * cell_size *
		    METERSQ_TO_HECTACRE);
	    break;
	case 5:
	    fprintf(output->out_file, "    Basin Area kilometers: %-13.4f",
		    output->basin_facts[c].num_cells * cell_size *
		    METERSQ_TO_KILOSQ);
	    break;
	case 6:
	    fprintf(output->out_file, "    Basin Area in cells: %-16d",
		    output->basin_facts[c].num_cells);
	    break;
	}
	fprintf(output->out_file, "             Area       Percent Basin\n");
	for (b = 0; b < output->num_maps; b++) {
	    fprintf(output->out_file,
		    "<< %20s >> map layer, average category value: %.2f\n",
		    output->maps[b].name,
		    ((double)output->maps[b].basins[c].sum_values) /
		    output->basin_facts[c].num_cells);
	    do_cat = &(output->maps[b].basins[c].first_cat);
	    while ((output->maps[b].do_cats != 0) && do_cat) {
		cat_name =
		    Rast_get_c_cat(&(do_cat->cat_val), &(output->maps[b].cats));
		switch (output->type_area) {
		case 1:
		    sprintf(area, "%.3f acres",
			    METERSQ_TO_ACRE * cell_size * do_cat->num_cat);
		    break;
		case 2:
		    sprintf(area, "%.2f sq. meters",
			    cell_size * do_cat->num_cat);
		    break;
		case 3:
		    sprintf(area, "%.4f sq. miles",
			    METERSQ_TO_MILESQ * cell_size * do_cat->num_cat);
		    break;
		case 4:
		    sprintf(area, "%.3f hectacres",
			    METERSQ_TO_HECTACRE * cell_size *
			    do_cat->num_cat);
		    break;
		case 5:
		    sprintf(area, "%.3f sq. km.",
			    METERSQ_TO_KILOSQ * cell_size * do_cat->num_cat);
		    break;
		case 6:
		    sprintf(area, "%6d cells", do_cat->num_cat);
		    break;
		}
		fprintf(output->out_file, "%3d %-43s %16s %-.4f\n",
			do_cat->cat_val, cat_name, area,
			((double)do_cat->num_cat) /
			output->basin_facts[c].num_cells);
		do_cat = do_cat->nxt;
	    }
	}
    }

    return 0;
}
