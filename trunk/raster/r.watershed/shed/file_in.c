#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"


int ar_file_in(char *file_name, OUTPUT * output)
{
    FILE *ar_file;
    int bas_num, down_bas, i1, i2, bas_alloc;
    char s1[7], s2[5], s3[3];
    double northing, easting, str_slope, str_length;

    if ((ar_file = fopen(file_name, "r")) == NULL) {
	G_fatal_error(_("unable to open ARMSED file"));
    }
    output->num_basins = 0;
    bas_alloc = INCR;
    output->basin_facts = (B_FACTS *) G_malloc(INCR * sizeof(B_FACTS));
    while (!feof(ar_file)) {
	fscanf(ar_file, "%d %s %s %d %s %d %d %lf %lf %lf %lf",
	       &bas_num, s1, s2, &down_bas, s3, &i1, &i2,
	       &easting, &northing, &str_slope, &str_length);
	if (output->num_basins >= bas_alloc) {
	    bas_alloc += INCR;
	    output->basin_facts =
		(B_FACTS *) G_realloc(output->basin_facts,
				      bas_alloc * sizeof(B_FACTS));
	}
	output->basin_facts[output->num_basins].num_cells = 0;
	output->basin_facts[output->num_basins].str_length = str_length;
	output->basin_facts[output->num_basins].str_slope = str_slope;
	output->basin_facts[output->num_basins].easting = easting;
	output->basin_facts[output->num_basins].northing = northing;
	output->basin_facts[output->num_basins].down_basin = down_bas / 2 - 1;
	output->basin_facts[output->num_basins].valid = 1;
	output->num_basins++;
    }
    fclose(ar_file);

    return 0;
}
