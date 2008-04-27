/*!
 * \file snap.c
 *
 * \brief Vector library - Clean vector map (snap lines)
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the 
 * GNU General Public License (>=v2). 
 * Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Radim Blazek
 *
 * \date 2001
 */

#include <math.h> 
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/* function prototypes */
static int sort_new(const void *pa, const void *pb);

/* Vertex */
typedef struct {
    double x, y;
    int    anchor; /* 0 - anchor, do not snap this point, that means snap others to this */
                   /* >0  - index of anchor to which snap this point */
                   /* -1  - init value */
} XPNT;

typedef struct {
    int    anchor; 
    double along;
} NEW;

/* This function is called by  RTreeSearch() to add selected node/line/area/isle to thelist */
int add_item(int id, struct ilist *list)
{
    dig_list_add ( list, id );
    return 1;
}

/*!
 * \brief Snap selected lines to existing vertex in threshold.
 *
 * Snap selected lines to existing vertices.
 * 
 * \warning Lines are not necessarily snapped to nearest vertex, but to vertex in threshold! 
 *
 * Lines showing how vertices were snapped may be optionaly written to error map. 
 * Input map must be opened on level 2 for update at least on GV_BUILD_BASE.
 *
 * \param[in] Map input map where vertices will be snapped
 * \param[in] List_lines list of lines to snap
 * \param[in] thresh threshold in which snap vertices
 * \param[out] Err vector map where lines representing snap are written or NULL
 * \param[out] msgout file pointer where messages will be written or NULL
 *
 * \return void
*/

/* As mentioned above, lines are not necessarily snapped to nearest vertex! For example:
     |                    
     | 1         line 3 is snapped to line 1,
     |           then line 2 is not snapped to common node at lines 1 and 3,
                 because it is already outside of threshold
----------- 3   

     |
     | 2
     |	
*/
void 
Vect_snap_lines_list (struct Map_info *Map, struct ilist *List_lines, double thresh, struct Map_info *Err, FILE *msgout)
{
    struct line_pnts *Points, *NPoints;
    struct line_cats *Cats;
    int    line, ltype, line_idx;
    double thresh2;
    int    printed;

    struct Node *RTree;
    int    point;   /* index in points array */
    int    nanchors, ntosnap; /* number of anchors and number of points to be snapped */
    int    nsnapped, ncreated; /* number of snapped verices, number of new vertices (on segments) */
    int    apoints, npoints, nvertices; /* number of allocated points, registered points, vertices */
    XPNT   *XPnts;  /* Array of points */
    NEW    *New = NULL;    /* Array of new points */
    int    anew = 0, nnew;   /* allocated new points , number of new points */
    struct Rect rect;
    struct ilist *List;
    int *Index = NULL;  /* indexes of anchors for vertices */
    int aindex = 0;     /* allocated Index */
    int width  = 26;    /* fprintf width */

    if (List_lines -> n_values < 1)
	return;

    Points = Vect_new_line_struct ();
    NPoints = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    List = Vect_new_list();
    RTree = RTreeNewIndex();
    
    thresh2 = thresh * thresh;
    
    /* Go through all lines in vector, and add each point to structure of points */
    apoints = 0;
    point = 1; /* index starts from 1 !*/
    nvertices = 0;
    XPnts = NULL;
    printed = 0;

    if ( msgout ) fprintf (msgout, "%s...", _("Registering points")); 

    for ( line_idx = 0; line_idx < List_lines -> n_values; line_idx++ ){ 
	int v;
	
	line = List_lines -> value[line_idx];

	G_debug (3, "line =  %d", line);
	if ( !Vect_line_alive ( Map, line ) ) continue;

	ltype = Vect_read_line (Map, Points, Cats, line);

	for ( v = 0; v <  Points->n_points; v++ ){ 
	    G_debug (3, "  vertex v = %d", v);
	    nvertices++;

            /* Box */
            rect.boundary[0] = Points->x[v];  rect.boundary[3] = Points->x[v];	    
            rect.boundary[1] = Points->y[v];  rect.boundary[4] = Points->y[v];	    
            rect.boundary[2] = 0;  rect.boundary[5] = 0;	    

	    /* Already registered ? */
	    Vect_reset_list ( List );
	    RTreeSearch(RTree, &rect, (void *)add_item, List);
	    G_debug (3, "List : nvalues =  %d", List->n_values);

	    if ( List->n_values == 0 ) { /* Not found */
		/* Add to tree and to structure */
	        RTreeInsertRect( &rect, point, &RTree, 0);
	        if ( (point - 1) == apoints ) {
		    apoints += 10000;
		    XPnts = (XPNT *) G_realloc ( XPnts, (apoints + 1) * sizeof (XPNT) );
		}
		XPnts[point].x = Points->x[v];
		XPnts[point].y = Points->y[v];
		XPnts[point].anchor = -1;
                point++;		    
            }
	}
	if ( msgout && printed > 1000 ) {
	    fprintf (msgout, "\r%s... %d", _("Registering points"), point - 1); 
	    fflush ( msgout );
	    printed = 0;
	}
	printed++;
    }
    npoints = point - 1;
    if ( msgout ) {
	fprintf (msgout, "\r                                               \r" ); 
	fprintf ( msgout, "%-*s: %4d\n", width, _("All vertices"), nvertices ); 
	fprintf ( msgout, "%-*s: %4d\n", width, _("Registered points"),
		  npoints ); 
    }

    /* Go through all registered points and if not yet marked mark it as anchor and assign this anchor
     * to all not yet marked points in threshold */
    nanchors = ntosnap = 0;
    for ( point = 1; point <= npoints; point++ ) { 
	int i;
	G_debug (3, "  point = %d", point);
	
	if ( XPnts[point].anchor >= 0 ) continue;

	XPnts[point].anchor = 0; /* make it anchor */
	nanchors++;

	/* Find points in threshold */
	rect.boundary[0] = XPnts[point].x - thresh;  
	rect.boundary[3] = XPnts[point].x + thresh;	    
	rect.boundary[1] = XPnts[point].y - thresh;  
	rect.boundary[4] = XPnts[point].y + thresh;
	rect.boundary[2] = 0;  rect.boundary[5] = 0;	    

	Vect_reset_list ( List );
	RTreeSearch(RTree, &rect, (void *)add_item, List);
	G_debug (4, "  %d points in threshold box", List->n_values);
	
	for ( i = 0; i < List->n_values; i++ ) {
	    int    pointb;
	    double dx, dy, dist2;

	    pointb = List->value[i];
	    if ( pointb == point ) continue;

	    dx = XPnts[pointb].x - XPnts[point].x;
	    dy = XPnts[pointb].y - XPnts[point].y;
	    dist2 = dx * dx + dy * dy;

	    if ( dist2 <= thresh2 ) {
		XPnts[pointb].anchor = point;
		ntosnap++;
	    }
	}
    }
    if ( msgout ) {
	fprintf ( msgout, "%-*s: %4d\n", width, _("Nodes marked as anchor"), nanchors ); 
	fprintf ( msgout, "%-*s: %4d\n", width, _("Nodes marked to be snapped"), ntosnap ); 
    }

    /* Go through all lines and: 
     *   1) for all vertices: if not anchor snap it to its anchor
     *   2) for all segments: snap it to all anchors in threshold (except anchors of vertices of course) */
    
    printed = 0;
    nsnapped = ncreated = 0;
    if ( msgout ) fprintf (msgout, "%-*s: %4d", width, _("Snaps"), nsnapped + ncreated ); 
    
    for ( line_idx = 0; line_idx < List_lines -> n_values; line_idx++ ){ 
	int v, spoint, anchor;
	int changed = 0;
	
	line = List_lines -> value[line_idx];

	G_debug (3, "line =  %d", line);
	if ( !Vect_line_alive ( Map, line ) ) continue;

	ltype = Vect_read_line (Map, Points, Cats, line);
	
	if ( Points->n_points >= aindex ) {
	    aindex = Points->n_points;
	    Index = (int *) G_realloc ( Index, aindex * sizeof(int) );
	}

	/* Snap all vertices */
	for ( v = 0; v <  Points->n_points; v++ ){ 
            /* Box */
            rect.boundary[0] = Points->x[v];  rect.boundary[3] = Points->x[v];	    
            rect.boundary[1] = Points->y[v];  rect.boundary[4] = Points->y[v];	    
            rect.boundary[2] = 0;  rect.boundary[5] = 0;	    

	    /* Find point ( should always find one point )*/
	    Vect_reset_list ( List );
	    
	    RTreeSearch(RTree, &rect, (void *)add_item, List);

	    spoint = List->value[0];
	    anchor = XPnts[spoint].anchor;

	    if ( anchor > 0 ) { /* to be snapped */
		Points->x[v] = XPnts[anchor].x;
		Points->y[v] = XPnts[anchor].y;
                nsnapped++;		    
		changed = 1;
	        Index[v] = anchor; /* point on new location */
            } else {
	        Index[v] = spoint; /* old point */
	    }
	}
	
	/* New points */
	Vect_reset_line (NPoints);

	/* Snap all segments to anchors in threshold */
	for ( v = 0; v < Points->n_points - 1; v++ ){ 
	    int    i;
	    double x1, x2, y1, y2, xmin, xmax, ymin, ymax;
            
	    G_debug (3, "  segment = %d end anchors : %d  %d", v, Index[v], Index[v+1]);
	    
	    x1 = Points->x[v];
	    x2 = Points->x[v+1];
	    y1 = Points->y[v];
	    y2 = Points->y[v+1];

	    Vect_append_point ( NPoints, Points->x[v], Points->y[v], Points->z[v] );

	    /* Box */
	    if ( x1 <= x2 ) { xmin = x1; xmax = x2; } else { xmin = x2; xmax = x1; }
	    if ( y1 <= y2 ) { ymin = y1; ymax = y2; } else { ymin = y2; ymax = y1; }
	    
            rect.boundary[0] = xmin - thresh;  
	    rect.boundary[3] = xmax + thresh;	    
            rect.boundary[1] = ymin - thresh;  
	    rect.boundary[4] = ymax + thresh;	    
            rect.boundary[2] = 0;  rect.boundary[5] = 0;	    

	    /* Find points */
	    Vect_reset_list ( List );
	    RTreeSearch(RTree, &rect, (void *)add_item, List);
	
	    G_debug (3, "  %d points in box", List->n_values);

	    /* Snap to anchor in threshold different from end points */
	    nnew = 0;
	    for ( i = 0; i < List->n_values; i++ ) {
		double dist2, along;
		
	        spoint = List->value[i];
	        G_debug (4, "    spoint = %d anchor = %d", spoint, XPnts[spoint].anchor);

		if ( spoint == Index[v] || spoint == Index[v+1] ) continue; /* end point */
		if ( XPnts[spoint].anchor > 0 ) continue; /* point is not anchor */

		/* Check the distance */
		dist2 = dig_distance2_point_to_line ( XPnts[spoint].x, XPnts[spoint].y, 0,
			x1, y1, 0, x2, y2, 0, 0, NULL, NULL, NULL, &along, NULL );
	            
		G_debug (4, "      distance = %lf", sqrt(dist2));

		if ( dist2 <= thresh2 ) {
	            G_debug (4, "      anchor in thresh, along = %lf", along);

		    if ( nnew == anew ) {
			anew += 100;
			New = (NEW *) G_realloc ( New, anew * sizeof (NEW) );
		    }
		    New[nnew].anchor = spoint;
		    New[nnew].along = along;
		    nnew++;		    
		}
	    }
	    G_debug (3, "  nnew = %d", nnew);
	    /* insert new vertices */
	    if ( nnew > 0 ) {
		/* sort by distance along the segment */
		qsort ( New, sizeof(char)*nnew, sizeof ( NEW), sort_new);
		
		for ( i = 0; i < nnew; i++ ) {
		    anchor = New[i].anchor;
		    /* Vect_line_insert_point ( Points, ++v, XPnts[anchor].x, XPnts[anchor].y, 0); */
	            Vect_append_point ( NPoints, XPnts[anchor].x, XPnts[anchor].y, 0 );
		    ncreated++;
		}
		changed = 1;
	    }
	}
	
        /* append end point */
	v = Points->n_points-1; 
        Vect_append_point ( NPoints, Points->x[v], Points->y[v], Points->z[v] );

	if ( changed ) { /* rewrite the line */
	    Vect_line_prune ( NPoints );  /* remove duplicates */
	    if ( NPoints->n_points > 1 || ltype & GV_LINES ) {
	        Vect_rewrite_line ( Map, line, ltype, NPoints, Cats );  
	    }
	    else {
		Vect_delete_line ( Map, line);
	    }
	    if (Err) {
		Vect_write_line ( Err, ltype, Points, Cats);
	    }
	}

	if ( msgout && printed > 1000 ) {
  	    fprintf (msgout, "\r%s: %5d  (line = %d)", _("Snaps"), nsnapped + ncreated, line ); 
	    fflush ( msgout );
	    printed = 0;
	}
	printed++;
    } /* for each line */

    if ( msgout ) {
	fprintf ( msgout, "\r%-*s: %4d\n", width, _("Snapped vertices"), nsnapped ); 
	fprintf ( msgout, "%-*s: %4d\n", width, _("New vertices"), ncreated ); 
    }
    
    Vect_destroy_line_struct ( Points );
    Vect_destroy_line_struct ( NPoints );
    Vect_destroy_cats_struct ( Cats );
    G_free ( XPnts );
    G_free (Index);
    G_free ( New );
    RTreeDestroyNode ( RTree);
}

/* for qsort */
static int sort_new(const void *pa, const void *pb)
{
    NEW *p1 = (NEW *) pa;
    NEW *p2 = (NEW *) pb;

    if ( p1->along < p2->along ) return -1;
    if ( p1->along > p2->along ) return 1;
    return 1;
}

/*!
 * \brief Snap lines in vector map to existing vertex in threshold.
 *
 * For details see Vect_snap_lines_list()
 *
 * \param[in] Map input map where vertices will be snapped
 * \param[in] type type of lines to snap
 * \param[in] thresh threshold in which snap vertices
 * \param[out] Err vector map where lines representing snap are written or NULL
 * \param[out] msgout file pointer where messages will be written or NULL
 *
 * \return void
*/
void 
Vect_snap_lines (struct Map_info *Map, int type, double thresh, struct Map_info *Err, FILE *msgout )
{
    int line, nlines, ltype;
    
    struct ilist* List;
    
    List   = Vect_new_list();
    
    nlines = Vect_get_num_lines (Map);
    
    for (line = 1; line <= nlines; line++) {
	G_debug (3, "line =  %d", line);
	
	if (!Vect_line_alive (Map, line))
	    continue;
	
	ltype = Vect_read_line (Map, NULL, NULL, line);
	
	if (!(ltype & type))
	    continue;
	
	Vect_list_append (List, line);
    }
    
    Vect_snap_lines_list (Map, List, thresh, Err, msgout);

    Vect_destroy_list (List);

    return;
}
