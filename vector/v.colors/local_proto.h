/* make_colors.c */
void make_colors(struct Colors *, const char *, DCELL, DCELL, int);

/* scan_attr.c */
int scan_attr(const struct Map_info *, int, const char *, const char *,
	      const struct FPRange *, struct Colors *, int *, int *);

/* scan_cats.c */
void scan_cats(const struct Map_info *, int, const char *,
	       const struct FPRange *, struct Colors *, int *, int *);

/* write_rgb.c */
void write_rgb_values(const struct Map_info *, int, const char *,
		      struct Colors *);

