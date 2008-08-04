/* %W%  %G%  */
#include "ply_to_cll.h"
#define START_ROW	xy[incr].row
#define STOP_ROW	xy[incr+1].row
#define START_COL	xy[incr].col
#define STOP_COL	xy[incr+1].col

void save_area(struct element *xy, int num_points, int category)
{
    int incr;
    float first_cell, last_cell;

    for (incr = 0; incr < num_points;) {

	if (START_ROW != STOP_ROW) {
	    fprintf(stderr, "ERROR: start and end row not same\n");
	    for (incr = 0; incr < num_points; incr += 2) {
		fprintf(stderr, "%d: %d %f %d %f\n",
			incr, xy[incr].row, xy[incr].col,
			xy[incr + 1].row, xy[incr + 1].col);
	    }
	    incr++;
	    continue;
	}

	first_cell = START_COL + 1.;
	last_cell = STOP_COL;
	if (last_cell >= first_cell)
	    write_record(START_ROW, first_cell, last_cell, category);
	incr += 2;
    }
}
