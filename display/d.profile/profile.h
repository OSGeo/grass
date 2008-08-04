#include <grass/gis.h>
#include <grass/config.h>

typedef struct _ucat
{
    RASTER_MAP_TYPE type;
    union
    {
	CELL c;
	FCELL f;
	DCELL d;
    } val;
} UCAT;

typedef union _raster_ptr
{
    void *v;
    CELL *c;
    FCELL *f;
    DCELL *d;
} RASTER_PTR;

typedef struct _raster_map_ptr
{
    RASTER_MAP_TYPE type;
    RASTER_PTR data;
} RASTER_MAP_PTR;

struct windows
{
    char *name;
    float bot, top, left, right;
};

struct ProfileNode
{
    double north, east, dist;
    UCAT cat;
    struct ProfileNode *next;
};

struct Profile
{
    struct Cell_head window;
    double n1, e1, n2, e2;
    struct ProfileNode *ptr;
    long int count;
    UCAT MinCat, MaxCat;
};

#ifdef MAIN
struct windows windows[] = {
    {"mou", 85, 100, 0, 50},
    {"sta", 85, 100, 50, 100},
    {"map", 0, 85, 0, 50},
    {"orig", 0, 100, 0, 1009}
};

struct windows profiles[] = {
    {"pro1", 64, 85, 50, 100},
    {"pro2", 43, 64, 50, 100},
    {"pro3", 22, 43, 50, 100},
    {"pro4", 0, 22, 50, 100}
};
#else
extern struct windows windows[];
extern struct windows profiles[];
#endif

#define MOU     windows[0]
#define STA     windows[1]
#define MAP     windows[2]
#define ORIG     windows[3]

/* DrawText.c */
int DrawText(int, int, int, char *);

/* DumpProfile.c */
int WriteProfile(char *, char *, char *, char, struct Profile *);

/* ExtractProf.c */
int ExtractProfile(struct Profile *, char *, char *);

/* InitProfile.c */
int InitProfile(struct Profile *, struct Cell_head, double, double, double,
		double);
/* PlotProfile.c */
int PlotProfile(struct Profile, char *, int, int);

/* Range.c */
int WindowRange(char *, char *, long *, long *);
int quick_range(char *, char *, long *, long *);
int slow_range(char *, char *, long *, long *);

/* What.c */
int What(char *, char *, struct Cell_head, double, double);

/* bnw_line.c */
int black_and_white_line(int, int, int, int);

/* show.c */
int show_cat(int, char *, int, char *);
int show_utm(double, double);
int show_mouse(void);

/* utils.c */
int is_null_value(RASTER_MAP_PTR *, int);
