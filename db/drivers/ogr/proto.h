
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
/* error.c */
void init_error(void);
void append_error(const char *fmt, ...);
void report_error(void);

/* cursor.c */
cursor *alloc_cursor();
void free_cursor(cursor *);

/* describe.c */
int describe_table(OGRLayerH, dbTable **, cursor *);
