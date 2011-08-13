/* scan_attr */
int scan_attr(const struct Map_info *, int, const char *, const char *,
	      struct Colors *, int *, int *);

/* scan_cats */
void scan_cats(const struct Map_info *, int, const char *,
	       struct Colors *, int *, int *);

/* write_rgb.c */
void write_rgb_values(const struct Map_info *, int, const char *,
		      struct Colors *);
