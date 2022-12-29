/* do_labels.c */
int initialize_options(void);
int do_labels(FILE *, int);
int show_it(void);
int scan_ref(char *);

/* color.c */
void set_RGBA_from_components(RGBA_Color *, const unsigned char,
			      const unsigned char, const unsigned char);
int set_RGBA_from_str(RGBA_Color *, const char *);
void unset_RGBA(RGBA_Color *);
int RGBA_has_color(const RGBA_Color *);
void set_color_from_RGBA(const RGBA_Color *);
