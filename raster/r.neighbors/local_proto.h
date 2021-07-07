#include <grass/gis.h>

/* bufs.c */
extern int allocate_bufs(int);
extern int rotate_bufs(int);

/* gather */
extern void circle_mask(void);
extern void weights_mask(void);
extern int gather(DCELL *, int, int);
extern int gather_w(DCELL *, DCELL(*)[2], int, int);

/* readcell.c */
extern int readcell(int, int, int, int, int);

/* divr_cats.c */
extern int divr_cats(void);

/* intr_cats.c */
extern int intr_cats(void);

/* null_cats.c */
extern int null_cats(const char *);

/* read_weights.c */
extern void read_weights(const char *);
extern double gaussian(double, double);
extern double exponential(double, double);
extern void compute_weights(const char *, double);

/* parallel.c */
extern int segment_open(void);
extern void segment_close(void);
