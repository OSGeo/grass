/* openvect.c */
char *openvect(char *);

/* what.c */
int what(int, int, int, int, int, int, int);
int show_buttons(int);

/*flash.c */
void flash_area(struct Map_info *, plus_t, struct line_pnts *, int);
void flash_line(struct Map_info *, plus_t, struct line_pnts *, int);

/* attr.c */
int disp_attr(char *, char *, char *, char *, int);

extern char **vect;
extern int nvects;
extern struct Map_info *Map;
