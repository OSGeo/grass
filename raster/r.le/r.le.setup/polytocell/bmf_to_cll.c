#include <stdio.h>
#include <stdlib.h>
#include "ply_to_cll.h"
#include <grass/gis.h>
#define ROW_SHIFT	-1

char *gets();

#define MAXLINE	 500
#define FGET	    gets(buffer)
#define READLINE	if (FGET==NULL) quit() ;\
					sscanf (buffer,"%d %*d:%d:%d:%ld",\
					&cur_row, &col_b, &col_e, &cat)
#define CONVERT	col_b = col_b / 100; col_e =  num_cols - (col_e / 100)

int quit();

/*
 *  sort cannot handle multiply numeric fields, negative numbers or decimals.
 *  get around these limitations by multipling by 100, sorting and then dividing
 *  by 100;  this gives us two decimal accuracy.  
 *  columns we want to sort in descending order.
 */

int main(argc, argv)
     int argc;
     char *argv[];
{
    struct Cell_head wind;
    struct Categories cats;
    char buffer[MAXLINE];
    CELL *record;
    CELL *col_ptr;
    long cat;
    int cur_row, col_b, col_e;
    int num_rows, num_cols;
    int atrow, atcol;
    int newmap;
    int stat;
    char *mapset;

    if (argc != 2)
	exit(EXIT_FAILURE);

    G_gisinit("bmif_to_cell");

    READLINE;
    num_rows = col_b;
    num_cols = col_e;

    if (Rast_get_cellhd(argv[1], mapset = G_mapset(), &wind)) {
	fprintf(stderr, "ERROR bmif_to_cell: can't read cellhd file for %s\n",
		argv[1]);
	quit();
    }

    if (num_rows != wind.rows || num_cols != wind.cols) {
	fprintf(stderr,
		"ERROR: bmif and cellhd rows and cols do not match\n");
	fprintf(stderr, "   bmif_to_cell:        rows: %d   cols: %d\n",
		num_rows, num_cols);
	fprintf(stderr, "   Cellhd for <%s>: rows: %d   cols: %d\n", argv[1],
		wind.rows, wind.cols);
	quit();
    }

    Rast_set_window(&wind);

    record = Rast_allocate_c_buf();

    if ((newmap = Rast_open_c_new(argv[1])) == -1) {
	fprintf(stderr, "ERROR bmif_to_cell: can't open raster map %s\n",
		argv[1]);
	quit();
    }

    READLINE;
    CONVERT;

    /* Loop for all data rows */
    for (atrow = 0; atrow < num_rows; atrow++) {

	/* zero the output buffer array */

	col_ptr = record;
	for (atcol = 0; atcol < num_cols; atcol++)
	    *(col_ptr++) = 0;

	/* If we've hit the end of the file, 
	   write out some zero rows and quit */

	if (cur_row > num_rows) {
	    while (atrow < num_rows) {
		Rast_put_c_row(newmap, record);
		atrow++;
	    }
	    Rast_close(newmap);
	    goto finish;
	}

	/* write out enough rows to get to current row */

	while (atrow < cur_row + ROW_SHIFT) {
	    Rast_put_c_row(newmap, record);
	    atrow++;
	}

	do {
	    col_ptr = record + col_b;
	    for (atcol = col_b; atcol <= col_e; atcol++)
		*(col_ptr++) = (CELL) cat;
	    READLINE;
	    CONVERT;
	}
	while (cur_row == (atrow - ROW_SHIFT));

	Rast_put_c_row(newmap, record);
    }
    fprintf(stderr, "Close: %d\n", Rast_close(newmap));

  finish:
    G_suppress_warnings(1);
    stat = Rast_read_vector_cats(argv[1], mapset, &cats);
    G_suppress_warnings(0);
    if (stat >= 0) {		/* if cats file existed */
	printf("Copying vector category file\n");
	stat = Rast_write_cats(argv[1], &cats);
    }
    exit(0);
}


int quit(void)
{
    fprintf(stderr,
	    "    You drew a region outside the mask; restart REGIONS setup\n");
    exit(EXIT_FAILURE);
}
