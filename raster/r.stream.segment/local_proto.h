#include "io.h"
#include "local_vars.h"

int free_attributes(int number_of_streams);
int convert_border_dir(int r, int c, int dir);

int create_sectors(STREAM *cur_stream, int seg_length, int seg_skip,
		   double seg_treshold);
int calc_tangents(STREAM *cur_stream, int seg_length, int seg_skip,
		  int number_streams);

int create_sector_vector(char *out_vector, int number_of_streams, int radians);
int create_segment_vector(char *out_vector, int number_of_streams,
			  int radians);


int ram_build_streamlines(CELL **streams, CELL **dirs, FCELL **elevation,
			  int number_of_streams);
int ram_fill_streams(CELL **unique_streams, int number_of_streams);
int ram_find_contributing_cell(int r, int c, CELL **dirs, FCELL **elevation);
int ram_identify_next_stream(CELL **streams, int number_of_streams);
int ram_number_of_streams(CELL **streams, CELL **dirs, int *ordered);

int seg_build_streamlines(SEGMENT *streams, SEGMENT *dirs,
			  SEGMENT *elevation, int number_of_streams);
int seg_fill_streams(SEGMENT *unique_streams, int number_of_streams);
int seg_find_contributing_cell(int r, int c, SEGMENT *dirs,
			       SEGMENT *elevation);
int seg_identify_next_stream(SEGMENT *streams, int number_of_streams);
int seg_number_of_streams(SEGMENT *streams, SEGMENT *dirs, int *ordered);
