/* copy_tab.c */
<<<<<<< HEAD
<<<<<<< HEAD
void copy_tabs(struct Map_info *, int, int, struct Map_info *);
=======
void copy_tabs(const struct Map_info *, int, int, struct Map_info *);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void copy_tabs(const struct Map_info *, int, int, struct Map_info *);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

/* extract.c */
int cmp(const void *, const void *);
int extract_line(int, int *, struct Map_info *, struct Map_info *, int, int,
                 int, char *, int, int, int);
