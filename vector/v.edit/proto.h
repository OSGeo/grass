#ifndef _V_EDIT_PROTO
#define _V_EDIT_PROTO

/* args.c */
int parser(int, char *[], struct GParams *, enum mode *);

/* close.c */
int close_lines(struct Map_info *, int, double);

/* select.c */
int print_selected(struct ilist *);
struct ilist *select_lines(struct Map_info *, int, enum mode,
                           struct SelectParams *, struct ilist *);
int sel_by_cat(struct Map_info *, struct cat_list *, int, int, char *,
               struct ilist *);
int sel_by_coordinates(struct Map_info *, int, struct line_pnts *, double,
                       struct ilist *);
int sel_by_bbox(struct Map_info *, int, double, double, double, double,
                struct ilist *);
int sel_by_polygon(struct Map_info *, int, struct line_pnts *, struct ilist *);
int sel_by_id(struct Map_info *, int, char *, struct ilist *);
int sel_by_where(struct Map_info *, int, int, char *, struct ilist *);
int reverse_selection(struct Map_info *, int, struct ilist **);
int sel_by_query(struct Map_info *, int, int, double, const char *,
                 struct ilist *);
int str_to_coordinates(const char *, struct line_pnts *);
int str_to_bbox(const char *, struct line_pnts *);
int str_to_polygon(const char *, struct line_pnts *);

/* snap.c */
int snap_lines(struct Map_info *, struct ilist *, double);
int snap_line(struct Map_info *, int, int, double);

/* max_distance.c */
double max_distance(double);
void coord2bbox(double, double, double, struct line_pnts *);

/* batch.c */
int batch_edit(struct Map_info *, struct Map_info **, int, const char *, char,
               struct SelectParams *);

#endif /* _V_EDIT_PROTO */
