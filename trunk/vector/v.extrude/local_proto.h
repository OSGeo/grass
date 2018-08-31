/* db.c */
int get_height(const struct field_info *, const char *,
               dbDriver *, int, double *);

/* extrude.c */
int extrude(struct Map_info *, struct Map_info *,
            const struct line_cats *, const struct line_pnts *,
            int, int, int, double, int, double,
            double, double,
            const struct Cell_head *, int, int);
