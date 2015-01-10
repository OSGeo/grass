#include "cell_ptrHa.h"
#include "costHa.h"
/* collect_ori.c */
void collect_ori(int, int);

/* deleteHa.c */
void deleteHa(float, int, int, struct costHa *, long *);

/* display.c */
void display_init(void);
void draw_a_cell(int, int, int);
void draw_a_burning_cell(int, int);
void display_close(void);

/* fixHa.c */
struct costHa *fixHa(long, struct costHa *, long);

/* get_minHa.c */
void get_minHa(struct costHa *, struct costHa *, long);

/* insert2Ha.c */
void insert2Ha(struct cell_ptrHa **, struct cell_ptrHa **, float, int, int);

/* insertHa.c */
void insertHa(float, float, int, int, struct costHa *, long *);

/* pick_dist.c */
int pick_dist(int);

/* pick_ignite.c */
int pick_ignite(int);

/* ram2out.c */
void ram2out(void);

/* replaceHa.c */
void replaceHa(float, float, int, int, struct costHa *, long *);

/* select_linksB.c */
void select_linksB(struct costHa *, int, float);

/* spot.c */
void spot(struct costHa *, int);

/* spread.c */
void spread(void);
int cumulative(struct costHa *, struct cell_ptrHa *, int, int, int, float *);
void update(struct costHa *, int, int, double, float);
