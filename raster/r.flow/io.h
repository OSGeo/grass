#include <grass/gis.h>

/********************************* I/O **********************************/

/*
 * read_input_files: loads input files completely into z, o arrays
 * globals r:  region, parm
 * globals rw: el, as
 * globals w:  density, string
 */

void read_input_files();

/*
 * open_output_files: opens continuously written files (length/vector)
 * globals r: parm, fl
 * globals w: string, el, as, ds, lgfd
 */

void open_output_files();

/*
 * close_files: closes continuously written/read files, outputs header info
 * globals r: parm, el, as, ds, lgfd, fl
 */

void close_files();

/*
 * write_density_file: dumps density matrix and colormap
 * globals r: density, parm, region, string
 */

void write_density_file();
