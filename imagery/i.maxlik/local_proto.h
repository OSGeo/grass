/* classify.c */
int classify(CELL *, CELL *, int);

/* hist.c */
int make_history(char *, char *, char *, char *);

/* invert.c */
int invert_signatures(void);
int invert(struct One_Sig *, int, int *, int *, double *);

/* open.c */
int open_files(void);
