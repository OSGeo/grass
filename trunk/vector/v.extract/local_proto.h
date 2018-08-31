/* copy_tab.c */
void copy_tabs(const struct Map_info *, int, int,
	       struct Map_info *);

/* extract.c */
int cmp(const void *, const void *);
int extract_line(int, int *, struct Map_info *,
		 struct Map_info *, int, int, int, char* ,
		 int, int, int);
