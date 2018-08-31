/* bufs.c */
extern int allocate_bufs(void);
extern int rotate_bufs(void);

/* gather */
extern void circle_mask(void);
extern void weights_mask(void);
extern int gather(DCELL *, int);
extern int gather_w(DCELL *, DCELL(*)[2], int);

/* readcell.c */
extern int readcell(int, int, int, int);

/* divr_cats.c */
extern int divr_cats(void);

/* intr_cats.c */
extern int intr_cats(void);

/* null_cats.c */
extern int null_cats(const char *);

/* read_weights.c */
extern void read_weights(const char *);
extern void gaussian_weights(double);
