/* use option */
#define USE_ATTR 1
#define USE_CAT  2
#define USE_Z    3

/* scan_cats.c */
void scan_cats(const struct Map_info *, int, const char *, const char *,
	       const struct FPRange *, struct Colors *);

/* scan_attr.c */
int scan_attr(const struct Map_info *, int, const char *, const char *,
	      const char *, const struct FPRange *, struct Colors *,
	      struct Colors *, int);

/* scan_z.c */
void scan_z(struct Map_info *, int, const char *, const char *,
	    const struct FPRange *, struct Colors *, int);

/* make_colors.c */
void make_colors(struct Colors *, const char *, DCELL, DCELL, int);
void load_colors(struct Colors *, const char *, DCELL, DCELL, int);
void color_rules_to_cats(dbCatValArray *, int, struct Colors *,
			 struct Colors *, int, DCELL, DCELL);

/* write_rgb.c */
void write_rgb_values(const struct Map_info *, int, const char *,
		      struct Colors *);
/* read_rgb.c */
void rgb2colr(const struct Map_info *, int, const char *, struct Colors *);
