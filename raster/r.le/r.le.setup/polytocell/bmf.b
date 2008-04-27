
#include "ply_to_cll.h"
#include "gis.h"
#include <stdio.h>
#define ROW_SHIFT	-1

char *gets() ;

#define MAXLINE	 90
#define FGET	    gets(buffer)
#define READLINE	if (FGET==NULL) quit() ;\
					sscanf (buffer,"%d%d%d%d",\
					&cur_row, &col_b, &col_e, &cat) 

main(argc, argv)
	int argc;
	char *argv[] ;
{
	struct Cell_head wind ;
	char buffer[MAXLINE] ;
	CELL *record ;
	CELL *col_ptr ;
	int cat ;
	int cur_row, col_b, col_e ;
	int num_rows, num_cols ;
	int atrow, atcol ;
	int newmap ;

	if (argc != 2)
		exit(-1) ;

	G_gisinit("bmif_to_cell") ;

	READLINE ;
	num_rows = col_b ;
	num_cols = col_e ;

	if ((num_rows*num_cols) <= 0)
	{
		fprintf(stderr,
		  "    READ ERROR:  bmif_to_cell receiving bad header info\n") ;
		quit() ;
	}

	G_get_window(&wind) ;
	wind.rows = num_rows ;
	wind.cols = num_cols ;
	wind.ew_res= 1.0 ;
	wind.ns_res= 1.0 ;
	wind.north = (double)wind.rows ;
	wind.south = 0.0 ;
	wind.east  = (double)wind.cols ;
	wind.west  = 0.0 ;
	G_set_window(&wind) ;

	record = G_allocate_cell_buf() ;

	if ( (newmap = G_open_cell_new(argv[1],"") ) == -1)
	{
	    fprintf("bmif_to_cell error: can't open raster map %s\n", argv[1]) ;
	    quit() ;
	}

	READLINE ;

	/* Loop for all data rows */
	for(atrow=0; atrow<num_rows; atrow++)
	{
	/* zero the output buffer array */
		col_ptr = record ;
		for(atcol=0; atcol<num_cols; atcol++)
			*(col_ptr++) = 0 ;
		
	/* If we've hit the end of the file, write out some zero rows and quit */
		if (cur_row > num_rows)
		{
			while (atrow < num_rows)
			{
				G_put_map_row(newmap, record) ; 
				atrow++ ;
			}
			G_close_cell(newmap) ;
			exit(0) ;
		}

	/* write out enough rows to get to current row */
		while (atrow < cur_row + ROW_SHIFT)
		{
			G_put_map_row(newmap, record) ; 
			atrow++ ;
		}

		do
		{
			col_ptr = record + col_b ;
			for(atcol=col_b; atcol<=col_e; atcol++)
				*(col_ptr++) = (CELL)cat ;
			READLINE ;
		} while (cur_row == (atrow - ROW_SHIFT)) ;

		G_put_map_row(newmap, record) ; 
	}
	fprintf(stderr, "Close: %d\n", G_close_cell(newmap)) ;
}

quit()
{
	fprintf(stderr,"    You drew a region outside the mask; restart REGIONS setup\n") ;
	exit(-1) ;
}
