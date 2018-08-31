#include <stdio.h>
#include <math.h>
#include <sys/types.h>

#define GRID_ID "grid003.02"
#define DSPF_ID "dspf003.02"

#define LINTERP(A,B,C)  ((C-A)/(B-A))*255
#define VOID_TYPE char

#define FLINTERP(A,B,C,D,E)  (((C-A)*(E-D))/(B-A)) + D
/*#define LENGTH(A,B,C)  sqrt(A*A + B*B + C*C) */

/* used in the lambert shading model */
#define NV(A,B,C)	((A*A) + (B*B) + (C*C))
#define MAXLITS		  3

#define MAXTHRESH	127
#define MAXPOLY		 10

typedef struct
{
    int nthres;			/* number of thresholds */
    float tvalue[MAXTHRESH];	/* array of threshold values */
    int litmodel;		/* 1 = flat, 2 = gradient(6), 3 = gradient(26) */
} cmndln_info;

typedef struct
{
    int token;			/*type of file */
    void *g3mapin, *g3mapout;	/* RASTER3D_Map */
    FILE *datainfp, *dataoutfp;
    FILE *dspfinfp, *dspfoutfp;
    int xdim, ydim, zdim;
    float north, south, east, west;
    float top, bottom;
    float ns_res, ew_res, tb_res;
    int zone;
    int proj;
    int type;			/*1 = short int, 2 = integer, 3 = float */
    float min, max;		/* max and min values present in the data */
    long Dataoff;		/* offset of beginning of data in file */
    long Lookoff;		/* offset of beginning of lookup table in file */
    cmndln_info linefax;	/* more global info */
    int headsize;		/* size of datainf file header, necessary for random
				   access to grid3 data */
} file_info;


typedef struct
{
    float v1[3];		/*polygon vertices */
    float v2[3];
    float v3[3];
    float n1[3], n2[3], n3[3];	/*normals for vertices */
} poly_info;

typedef struct
{
    int npoly;			/* number of polygons in cube at given threshold */
    int t_ndx;			/* index of given threshold */
    poly_info poly[MAXPOLY];	/*vertices and normals */
} cube_info;

typedef struct
{
    int n_thresh;
    cube_info data[MAXTHRESH];
} Cube_data;

typedef struct
{
    int nverts;
    int verts[8];
    int nedges;
    int edges[12];
    int npolys;
    int polys[30];
} CELL_ENTRY;			/* for writing out in condensed format */

/* cube_io.c */
int write_cube(Cube_data *, int, file_info *);
int write_cube_buffer(unsigned char *, int, int, file_info *);
int read_cube(Cube_data *, file_info *);
int my_fread(char *, int, int, FILE *);
int my_fread(char *, int, int, FILE *);
int reset_reads(file_info *);
int new_dspf(file_info *);

/* dspf_header.c */
int dfwrite_header(file_info *);
int dfread_header(file_info *);
int dfread_header_old(file_info *, FILE *);

/* print_info.c */
int print_head_info(file_info *);

/* struct_copy.c */
int struct_copy(char *, char *, int);

extern CELL_ENTRY cell_table[];
