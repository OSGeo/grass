#define DELETED -2

#define le 0
#define re 1

struct Freenode
{
    struct Freenode *nextfree;
};
struct Freelist
{
    struct Freenode *head;
    int nodesize;
};


struct Point
{
    double x, y, z;
};

/* structure used both for sites and for vertices */
struct Site
{
    struct Point coord;
    int sitenbr;
    int refcnt;
};

struct Edge
{
    double a, b, c;
    struct Site *ep[2];
    struct Site *reg[2];
    int edgenbr;
};

struct Halfedge
{
    struct Halfedge *ELleft, *ELright;
    struct Edge *ELedge;
    int ELrefcnt;
    char ELpm;
    struct Site *vertex;
    double ystar;
    struct Halfedge *PQnext;
};

extern int triangulate, sorted, plot, debug, mode3d;
extern struct Site *sites;
extern int nsites;
extern int siteidx;
extern int sqrt_nsites;
extern int nvertices;
extern struct Freelist sfl;
extern struct Site *bottomsite;
extern int nedges;
extern struct Freelist efl;
extern double xmin, xmax, ymin, ymax, deltax, deltay;
extern struct Freelist hfl;
extern struct Halfedge *ELleftend, *ELrightend;
extern int ELhashsize;
extern struct Halfedge **ELhash;
extern int PQhashsize;
extern struct Halfedge *PQhash;
extern int PQcount;
extern int PQmin;

/* sw_edgelist.c */
int ELinitialize(void);
struct Halfedge *HEcreate(struct Edge *, int);
int ELinsert(struct Halfedge *, struct Halfedge *);
struct Halfedge *ELgethash(int);
struct Halfedge *ELleftbnd(struct Point *);
int ELdelete(struct Halfedge *);
struct Halfedge *ELright(struct Halfedge *);
struct Halfedge *ELleft(struct Halfedge *);
struct Site *leftreg(struct Halfedge *);
struct Site *rightreg(struct Halfedge *);

/* sw_geometry.c */
int geominit(void);
struct Edge *bisect(struct Site *, struct Site *);
struct Site *intersect(struct Halfedge *, struct Halfedge *);
int right_of(struct Halfedge *, struct Point *);
int endpoint(struct Edge *, int, struct Site *);
double dist(struct Site *, struct Site *);
int makevertex(struct Site *);
int deref(struct Site *);
int ref(struct Site *);
double d_ulp(double);

/* sw_heap.c */
int PQinsert(struct Halfedge *, struct Site *, double);
int PQdelete(struct Halfedge *);
int PQbucket(struct Halfedge *);
int PQempty(void);
struct Point PQ_min(void);
struct Halfedge *PQextractmin(void);
int PQinitialize(void);

/* sw_main.c */
int scomp(const void *, const void *);
struct Site *nextone(void);
int readsites(void);
struct Site *readone(void);

/* sw_memory.c */
int freeinit(struct Freelist *, int);
char *getfree(struct Freelist *);
int makefree(struct Freenode *, struct Freelist *);
char *myalloc(unsigned);

/* sw_output.c */
int openpl(void);
int line(int, int, int, int);
int circle(int, int, int);
int range(int, int, int, int);
int out_bisector(struct Edge *);
int out_ep(struct Edge *);
int out_vertex(struct Site *);
int out_site(struct Site *);
int out_triple(struct Site *, struct Site *, struct Site *);
int plotinit(void);
int clip_line(struct Edge *);

/* sw_voronoi.c */
int voronoi(int, struct Site *(*)(void));
