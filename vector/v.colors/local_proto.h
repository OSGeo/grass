/* scan_attr */
int scan_attr(const struct Map_info *, int, const char *,
	       double *, double *);

/* scan_cats */
void scan_cats(const struct Map_info *, int, double *, double*);

/* write_rgb.c */
void write_rgb_values(const struct Map_info *, int, const char *,
		      struct Colors *);
