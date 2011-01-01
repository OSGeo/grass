#ifndef GRASS_VEDIT_H
#define GRASS_VEDIT_H

#include <grass/gis.h>
#include <grass/vector.h>

#define NO_SNAP    0		/* snapping disabled */
#define SNAP       1		/* snapping enabled for nodes */
#define SNAPVERTEX 2		/* snapping enabled for vertex also */

#define QUERY_UNKNOWN -1
#define QUERY_LENGTH   0	/* select by line length */
#define QUERY_DANGLE   1	/* select dangles */

/* used by Vedit_render_map() */
#define TYPE_POINT           0x01
#define TYPE_LINE	     0x02
#define TYPE_BOUNDARYNO      0x04
#define TYPE_BOUNDARYTWO     0x08
#define TYPE_BOUNDARYONE     0x10
#define TYPE_CENTROIDIN      0x20
#define TYPE_CENTROIDOUT     0x40
#define TYPE_CENTROIDDUP     0x80
#define TYPE_NODEONE         0x100
#define TYPE_NODETWO         0x200
#define TYPE_VERTEX          0x400
#define TYPE_AREA            0x800
#define TYPE_ISLE            0x1000
#define TYPE_DIRECTION       0x2000

#define DRAW_POINT           0x01
#define DRAW_LINE	     0x02
#define DRAW_BOUNDARYNO      0x04
#define DRAW_BOUNDARYTWO     0x08
#define DRAW_BOUNDARYONE     0x10
#define DRAW_CENTROIDIN      0x20
#define DRAW_CENTROIDOUT     0x40
#define DRAW_CENTROIDDUP     0x80
#define DRAW_NODEONE         0x100
#define DRAW_NODETWO         0x200
#define DRAW_VERTEX          0x400
#define DRAW_AREA            0x800
#define DRAW_DIRECTION       0x1000

struct rpoint {
    /* screen coordinates */
    int x, y;
};

struct robject {
    /* object to be rendered */
    int            type;
    int            npoints;
    struct rpoint *point;   /* list of points */
};

struct robject_list {
    /* list of objects to be rendered */
    int              nitems;
    struct robject **item;
};

/* break.c */
int Vedit_split_lines(struct Map_info *, struct ilist *,
		      struct line_pnts *, double, struct ilist *);
int Vedit_connect_lines(struct Map_info *, struct ilist *, double);

/* cats.c */
int Vedit_modify_cats(struct Map_info *, struct ilist *,
		      int, int, struct cat_list *);

/* copy.c */
int Vedit_copy_lines(struct Map_info *, struct Map_info *, struct ilist *);

/* chtype.c */
int Vedit_chtype_lines(struct Map_info *, struct ilist *);

/* delete.c */

int Vedit_delete_lines(struct Map_info *, struct ilist *);

/* distance.c */
double Vedit_get_min_distance(struct line_pnts *, struct line_pnts *,
			      int, int *);

/* flip.c */
int Vedit_flip_lines(struct Map_info *, struct ilist *);

/* merge.c */
int Vedit_merge_lines(struct Map_info *, struct ilist *);

/* move.c */
int Vedit_move_lines(struct Map_info *, struct Map_info **, int,
		     struct ilist *, double, double, double, int, double);

/* render.c */
struct robject_list *Vedit_render_map(struct Map_info *, struct bound_box *, int,
				      double, double, int, int, double);

/* select.c */
int Vedit_select_by_query(struct Map_info *,
			  int, int, double, int, struct ilist *);

/* snap.c */
int Vedit_snap_point(struct Map_info *,
		     int, double *, double *, double *, double, int);
int Vedit_snap_line(struct Map_info *, struct Map_info **, int,
		    int, struct line_pnts *, double, int);
int Vedit_snap_lines(struct Map_info *, struct Map_info **, int,
		     struct ilist *, double, int);

/* vertex.c */
int Vedit_move_vertex(struct Map_info *, struct Map_info **, int,
		      struct ilist *,
		      struct line_pnts *, double, double,
		      double, double, double, int, int);
int Vedit_add_vertex(struct Map_info *Map, struct ilist *,
		     struct line_pnts *, double);
int Vedit_remove_vertex(struct Map_info *, struct ilist *,
			struct line_pnts *, double);

/* zbulk.c */
int Vedit_bulk_labeling(struct Map_info *, struct ilist *,
			double, double, double, double, double, double);

#endif /* GRASS_VEDIT_H */
