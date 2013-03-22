#ifndef GRASS_IMAGERY_H
#define GRASS_IMAGERY_H

#include <grass/gis.h>
#include <grass/raster.h>

/* File/directory name lengths */
#define INAME_LEN GNAME_MAX	/* coupled to raster map name length */

struct Ref_Color
{
    unsigned char *table;	/* color table for min-max values */
    unsigned char *index;	/* data translation index */
    unsigned char *buf;	/* data buffer for reading color file */
    int fd;			/* for image i/o */
    CELL min, max;		/* min,max CELL values */
    int n;			/* index into Ref_Files */
};

struct Ref_Files
{
    char name[INAME_LEN];	/* length is not in sync with other definitions */
    char mapset[INAME_LEN];
};

struct Ref
{
    int nfiles;
    struct Ref_Files *file;
    struct Ref_Color red, grn, blu;
};

struct Tape_Info
{
    char title[75];
    char id[2][75];
    char desc[5][75];
};

struct Control_Points
{
    int count;
    double *e1;
    double *n1;
    double *e2;
    double *n2;
    int *status;
};

struct One_Sig
{
    char desc[100];
    int npoints;
    double *mean;		/* one mean for each band */
    double **var;		/* covariance band-band   */
    int status;		/* may be used to 'delete' a signature */
    float r, g, b;		/* color */
    int have_color;
};

struct Signature
{
    int nbands;
    int nsigs;
    char title[100];
    struct One_Sig *sig;
};

struct SubSig
{
    double N;
    double pi;
    double *means;
    double **R;
    double **Rinv;
    double cnst;
    int used;
};

struct ClassData
{
    int npixels;
    int count;
    double **x;		/* pixel list: x[npixels][nbands] */
    double **p;		/* prob        p[npixels][subclasses] */
};

struct ClassSig
{
    long classnum;
    char *title;
    int used;
    int type;
    int nsubclasses;
    struct SubSig *SubSig;
    struct ClassData ClassData;
};

struct SigSet
{
    int nbands;
    int nclasses;
    char *title;
    struct ClassSig *ClassSig;
};

/* IClass */

/*! Holds statistical values for creating histograms and raster maps for one class.

  One class is represented by one category (cat).
*/
typedef struct
{
    int cat;                /*!< class */
    const char *name;       /*!< signature description (class name) */
    const char *color;      /*!< class color (RRR:GGG:BBB)*/
    int nbands;             /*!< number of bands */
    
    int ncells;             /*!< number of cells in training areas */

    int *band_min;          /*!< minimum value for each band */
    int *band_max;          /*!< maximum value for each band */
    float *band_sum;        /*!< sum of values for each band */
    float *band_mean;       /*!< mean of values for each band */
    float *band_stddev;     /*!< standard deviation for each band */

    float **band_product;   /*!< sum of products of cell category values of 2 bands */
    int **band_histo;       /*!< number of cells for cell category value (0-256) for each band */

    int *band_range_min;    /*!< min range of values to create raster map */
    int *band_range_max;    /*!< max range of values to create raster map */
    float nstd;             /*!< multiplier of standard deviation */

    
} IClass_statistics;

#define SIGNATURE_TYPE_MIXED 1

#define GROUPFILE "CURGROUP"
#define SUBGROUPFILE "CURSUBGROUP"

#include <grass/defs/imagery.h>

#endif
