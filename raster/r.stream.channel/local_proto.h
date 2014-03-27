#include "io.h"
#include "local_vars.h"

int free_attributes(int number_of_streams);


int ram_build_streamlines(CELL **streams, CELL **dirs, FCELL **elevation,
			  int number_of_streams);
int ram_number_of_streams(CELL **streams, CELL **dirs);
int ram_find_contributing_cell(int r, int c, CELL **dirs, FCELL **elevation);

int ram_set_null_output(DCELL **output);
int ram_calculate_identifiers(CELL **identifier, int number_of_streams,
			      int downstream);
int ram_calculate_difference(DCELL **output, int number_of_streams,
			     int downstream);
int ram_calculate_drop(DCELL **output, int number_of_streams, int downstream);
int ram_calculate_local_distance(DCELL **output, int number_of_streams,
				 int downstream);
int ram_calculate_distance(DCELL **output, int number_of_streams,
			   int downstream);
int ram_calculate_cell(DCELL **output, int number_of_streams, int downstream);
int ram_calculate_local_gradient(DCELL **output, int number_of_streams,
				 int downstream);
int ram_calculate_gradient(DCELL **output, int number_of_streams,
			   int downstream);
int ram_calculate_curvature(DCELL **output, int number_of_streams,
			    int downstream);



int seg_build_streamlines(SEGMENT *streams, SEGMENT *dirs,
			  SEGMENT *elevation, int number_of_streams);
int seg_number_of_streams(SEGMENT *streams, SEGMENT *dirs);

int seg_find_contributing_cell(int r, int c, SEGMENT *dirs,
			       SEGMENT *elevation);

int seg_set_null_output(SEGMENT *output);
int seg_calculate_identifiers(SEGMENT *identifier, int number_of_streams,
			      int downstream);
int seg_calculate_difference(SEGMENT *output, int number_of_streams,
			     int downstream);
int seg_calculate_drop(SEGMENT *output, int number_of_streams,
		       int downstream);
int seg_calculate_local_distance(SEGMENT *output, int number_of_streams,
				 int downstream);
int seg_calculate_distance(SEGMENT *output, int number_of_streams,
			   int downstream);
int seg_calculate_cell(SEGMENT *output, int number_of_streams,
		       int downstream);
int seg_calculate_local_gradient(SEGMENT *output, int number_of_streams,
				 int downstream);
int seg_calculate_gradient(SEGMENT *output, int number_of_streams,
			   int downstream);
int seg_calculate_curvature(SEGMENT *output, int number_of_streams,
			    int downstream);
