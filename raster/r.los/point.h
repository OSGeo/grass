
/****************************************************************/
/*                                                              */
/*      point.h         in      ~/src/Glos                      */
/*                                                              */
/*      This header file defines the data structure of a        */
/*      point (structure containing various attributes of       */
/*      a grid cell).                                           */
/*                                                              */

/****************************************************************/

struct point
{

    double orientation;
    /* horizontal angle(degrees) measured from +ve x-axis   */

    double inclination;
    /* vertical angle(degrees) from the viewing point       */

    int x;			/* x-coor measured from viewing point location  */
    int y;			/* y-coor measured from viewing point location  */

    struct point *next;		/* pointer to next point in list */
    struct point *previous;	/* ptr to previous pt. in list  */

};

/* make_point.c */
struct point *make_point(double, double, int, int);

#ifdef GRASS_SEGMENT_H
/* delete.c */
struct point *delete(struct point *, struct point *, SEGMENT *, int, int);

/* make_list.c */
struct point *make_list(struct point *, int, int, SEGMENT *, int, int, int,
			int, int, double);
/* mark_pts.c */
int mark_visible_points(struct point *, SEGMENT *, int, int, double, double);

/* pts_elim.c */
struct point *hidden_point_elimination(struct point *, int, SEGMENT *,
				       SEGMENT *, SEGMENT *, int, int, int,
				       int, int, int, int, double);
/* segment.c */
struct point *segment(int, int, int, double, double,
		      int, int, int, int, SEGMENT *, SEGMENT *, SEGMENT *,
		      int, int, int, int, double);
/* 	
	For delayed deletion of points (see delete3.c).
	Initially set to NULL in main.c.
*/

#ifdef MAIN
struct point *DELAYED_DELETE;
#else
extern struct point *DELAYED_DELETE;
#endif

#endif

/****************************************************************/
