/*********************************************************************
 *
 *	collect_ori.c	in ~/r.spread2
 *
 *	functin to collect the spread origins from the source map and
 *	put them into a min-heap; also marks the origin locations and
 *	the other locations to avoid redundant computation and to be
 *	able to terminate.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "cmd_line.h"
#include "costHa.h"
#include "local_proto.h"

#define DATA(map, r, c)		(map)[(r) * ncols + (c)]

/*#define DEBUG*/

/* Globals from cmd_line */
char *backdrop_layer;
char *base_layer;
char *dir_layer;
char *max_layer;
char *spotdist_layer;
char *mois_layer;
char *out_layer;
char *start_layer;
char *velocity_layer;
char *x_out_layer;
char *y_out_layer;

float comp_dens;
int display;
int init_time;
int least;
int spotting;
int time_lag;
int verbose;
int x_out;
int y_out;

void collect_ori (int start_fd)
{
	extern CELL 	*cell;
	extern CELL	*map_base, *map_x_out, *map_y_out, *map_visit;
	extern float	*map_out; 
	extern char	buf[];
	extern float	neg, zero;
	extern int	BARRIER;
	extern int	nrows, ncols;
	extern long	heap_len;
	extern struct costHa	*heap;
	int		row, col;

	for ( row=0 ; row<nrows ; row++ ) {
		if (verbose)
			G_percent (row, nrows, 2);
		if( G_get_map_row(start_fd, cell, row)< 0)
			exit(1);
		for ( col=0 ; col<ncols ; col++ ) {
			if (*(cell+col) > 0) {
                                /*Check if starting sources legally ?*/
		                if (DATA(map_base, row, col) <= 0) {
			                sprintf (buf, "can't start from a BARRIER at cell (%d,%d), request ignored\n", col, row); 
   	                		G_warning (buf);
		                	continue;
				}
			
				DATA(map_out, row, col) = (float)init_time;	
				insertHa((float)init_time, zero, row, col, heap, &heap_len); 
                                /*mark it to avoid redundant computing*/
                                DATA(map_visit, row, col) = 1;
				if (x_out)	DATA(map_x_out, row, col) = col;
				if (y_out)	DATA(map_y_out, row, col) = row; 
/*DEBUG printf("\norigin: row=%d col=%d\n", row, col);*/				
				if (display)	
					draw_a_burning_cell (row, col);
			} else {
                                DATA(map_out, row, col) = neg;
				DATA(map_visit, row, col) = BARRIER;
			}
		}
	}
	if (verbose)
		G_percent (row, nrows, 2);
#ifdef DEBUG
{int i;
printf ("\nheap_len=%d  ", heap_len);
for (i=1; i<=heap_len; i++) printf ("(%d,%d) ", heap[i].row, heap[i].col);
}
#endif
}
