#include <grass/raster.h>

#define         SHORT           short
#define         TRUE    1
#define         FALSE   0

#define MELEMENT        struct Melement
MELEMENT {
    short x, y;			/* grid coordinates */
    int value;
    MELEMENT *next, *prior;	/* next and prior element in row list */
};

#define NEIGHBOR        struct neighbor
NEIGHBOR {
    double distance;
    MELEMENT *Mptr,		/* pointer to data in linked lists of input */
    **searchptr;		/* row search pointer that identified this
				   neighbor */
    NEIGHBOR *next;
};

/* structure for search pointers which access a row list of MELEMENTs */
/* if latitude-longitude, ealive and walive prevent search collisions on a
   circular, doubly-linked list; else, list is linear (NULL terminated) and
   pointers to MELEMENT are set NULL to indicate end of search in a direction */
#define EW              struct ew
EW {
    MELEMENT *east,		/* next eastward search in this row */
     *west,			/* next westward search in this row */
     *start;			/* starting point of east and west search in this row */
    short ealive, walive;	/* used only for latitude-longitude,
				   TRUE if search is active in this direction */
    EW *next;
};

/* dist.c */
int G_begin_geodesic_distance_l(short, double, double);
double LL_set_geodesic_distance_lat(double);
double set_sdlmr(double);
int LL_set_geodesic_distance(double *, int, int);
double LL_geodesic_distance(double);
int free_dist_params(void);

/* ll.c */
int extend_west(EW *);
int extend_east(EW *);
double distance_LL(SHORT, SHORT, MELEMENT *);
int LL_lookup_tables(SHORT, SHORT);

/* main.c */
int lookup_and_function_ptrs(SHORT, SHORT);
int interpolate(MELEMENT[], SHORT, SHORT, SHORT, int, int, int);
int make_neighbors_list(EW *, EW *, EW *, SHORT, SHORT, NEIGHBOR *, int);
int search(EW **, NEIGHBOR *, SHORT, SHORT, int, SHORT *, EW *, SHORT);
int exhaust(EW **, NEIGHBOR *, SHORT, SHORT);
EW *next_row(EW *, EW *, SHORT *, SHORT);
double triangulate(MELEMENT *, SHORT, SHORT);
int add_neighbor(MELEMENT **, NEIGHBOR *, double, int);
int replace_neighbor(MELEMENT **, NEIGHBOR *, double);
int sort_neighbors(NEIGHBOR *, double);
int free_row_lists(MELEMENT *, SHORT);
MELEMENT *row_lists(SHORT, SHORT, SHORT *, int *, int, CELL *);
int lookup_tables(SHORT, SHORT);
