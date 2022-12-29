/* Global variables: */
/*    direction     indicates whether we should use fptr or bptr to */
/*                  move to the "next" point on the line */
/*    first_read    flag to indicate that we haven't read from input */
/*                  file yet */
/*    last_read     flag to indicate we have reached EOF on input */
/*    row_length    length of each row of the raster map (i.e., number of */
/*                  columns) */
/*    n_rows        number of rows in the raster map */
/*    row_count     number of the row just read in--used to prevent reading */
/*                  beyond end of the raster map */
/*    equivs        pointer to allocated equivalence table made by */
/*                  write_equiv() */
/*    areas         pointer to allocated array of area information passed */
/*                  from bound.c */
/*    total_areas   number of distinct areas found */

/* Entry points: */
/*    write_line    write a line out to the digit files */
/*    write_boundary  write a line out to the digit files */
/*    write_area    make table of area mappings and write dlg label file */

#define BACKWARD 1
#define FORWARD 2
#define OPEN 1
#define END 2
#define LOOP 3

#define SMOOTH 1
#define NO_SMOOTH 0

#define CATNUM 0
#define CATLABEL 1

extern int data_type;
extern int data_size;
extern struct Map_info Map;
extern int input_fd;		/*    input_fd     input raster map descriptor */
extern struct line_cats *Cats;
extern struct Cell_head cell_head;

extern int direction;
extern int first_read, last_read;
extern int input_fd;
extern int row_length, row_count, n_rows;
extern int total_areas;
extern int n_alloced_ptrs;

extern int smooth_flag;		/* this is 0 for no smoothing, 1 for smoothing of lines */
extern int value_flag;		/* use raster values as categories */

extern struct Categories RastCats;
extern int has_cats;		/* Category labels available */
extern struct field_info *Fi;
extern dbDriver *driver;
extern dbString sql, label;

struct COOR
{
    struct COOR *bptr, *fptr;	/* pointers to neighboring points */
    int row, col, node;		/* row, column of point; node flag */
    int val;			/* CELL value */
    double dval;		/* FCELL/DCELL value */
    double right, left;		/* areas to right and left of line */

};

struct line_hdr
{
    struct COOR *left;
    struct COOR *right;
    struct COOR *center;
};

#define NULPTR ((struct COOR *) NULL)

/* area_table - structure to store stuff associated with each */
/* area number */

struct area_table
{
    int free;			/* this entry is not taken yet */
    double cat;			/* category number for this area */
    int row;			/* row and column of point where the */
    int col;			/*   area is widest */
    int width;			/*   and width there */
};

/* equiv_table - structure in which to compile equivalences between area */
/* numbers */

struct equiv_table
{
    int mapped;			/* is this area number mapped? */
    int where;			/* if so, where */
    int count;			/* if not, number mapped here */
    int length;
    int *ptr;			/*   and pointer to them */
};


/* lines.c */
int alloc_lines_bufs(int);
int extract_lines(void);

/* lines_io.c */
int write_line(struct COOR *);

/* areas.c */
int alloc_areas_bufs(int);
int extract_areas(void);
int more_equivs(void);

/* areas_io.c */
int write_boundary(struct COOR *);
int write_area(struct area_table *, struct equiv_table *, int, int);

/* points.c */
int extract_points(int);

/* util.c */
struct COOR *move(struct COOR *);
struct COOR *find_end(struct COOR *, int, int *, int *);
int at_end(struct COOR *);
int read_row(void *);
void insert_value(int, int, double);
int free_ptr(struct COOR *);

/* set_error_handler.c */
void set_error_handler(struct Map_info *, dbDriver **);
