#ifndef GRASS_VEDITDEFS_H
#define GRASS_VEDITDEFS_H

/* break.c */
int Vedit_split_lines(struct Map_info *, struct ilist *,
		      struct line_pnts *, double, struct ilist *);
int Vedit_connect_lines(struct Map_info *, struct ilist *, double);

/* extend.c */
int Vedit_extend_lines(struct Map_info *, struct ilist *, int, int, double);

/* cats.c */
int Vedit_modify_cats(struct Map_info *, struct ilist *,
		      int, int, struct cat_list *);

/* copy.c */
int Vedit_copy_lines(struct Map_info *, struct Map_info *, struct ilist *);

/* chtype.c */
int Vedit_chtype_lines(struct Map_info *, struct ilist *);

/* delete.c */
int Vedit_delete_lines(struct Map_info *, struct ilist *);
int Vedit_delete_area_centroid(struct Map_info *, int);
int Vedit_delete_area(struct Map_info *, int);
int Vedit_delete_areas_cat(struct Map_info *, int, int);

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

#endif /* GRASS_VEDITDEFS_H */
