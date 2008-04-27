/* global variables */
#ifdef MAIN
int TOP, BOTTOM, LEFT, RIGHT;
#else
extern int TOP, BOTTOM, LEFT, RIGHT;
#endif

/* popup.c */
int popup(FILE *, int, int, char *);
int pick(int, int);
int draw_which(int);
int outline_box(int, int, int, int);
int do_text(char *, int, int, int, int, int);
