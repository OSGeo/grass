#include "io.h"
#include "local_vars.h"

/* stream init */
int stream_init(int min_index_of_stream, int max_index_of_stream);
int stream_sample_map(char* input_map_name, int number_of_streams, int what);

/* stream topology */
int ram_stream_topology(CELL** streams, CELL** dirs, int number_of_streams);
int ram_stream_geometry(CELL** streams, CELL** dirs);
int seg_stream_topology(SEGMENT* streams, SEGMENT* dirs, int number_of_streams);
int seg_stream_geometry(SEGMENT* streams, SEGMENT* dirs);

/* stream order */
int strahler(int* strahler);
int shreve(int* shreve);
int horton(const int* strahler, int* horton, int number_of_streams);
int hack(int* hack, int* topo_dim, int number_of_streams);

/* stream raster close */
int ram_close_raster_order(CELL** streams, int number_of_streams, int zerofill);
int seg_close_raster_order(SEGMENT* streams, int number_of_streams, int zerofill);

/* stream vector */
int ram_create_vector(CELL** streams, CELL** dirs, char* out_vector, int number_of_streams);
int seg_create_vector(SEGMENT* streams, SEGMENT* dirs, char* out_vector, int number_of_streams);
int stream_add_table (int number_of_streams);
