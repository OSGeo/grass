#include <math.h>
#include <string.h>
#include <grass/raster.h>

#define LIKELIHOOD float

struct files
{
    int output_fd;
    int goodness_fd;
    struct Categories output_labels;

    int *band_fd;
    int nbands;

    DCELL *cellbuf;
    CELL *outbuf;
    char *isdata;
};

struct parms
{
    char *output_map;
    char *goodness_map;
    char *group;
    char *subgroup;
    char *sigfile;
    int blocksize;
    int ml;
};

/* parse.c */
int parse(int, char *[], struct parms *);

/* closefiles.c */
int closefiles(struct parms *, struct files *);

/* openfiles.c */
int openfiles(struct parms *, struct files *);

/* Suboutines in alpha_max.c */
void alpha_max(double ***, double *, int, double);
void line_search(double ***, double *, int, double *, double);
int normalize(double[3]);
double func(double);
double log_like(double ***, double[3], int);
void gradient(double[3], double ***, double[3], int);

/* Subroutines in multialloc.c */
char *multialloc(size_t, int, ...);
void multifree(char *, int);
unsigned char **get_img(int, int, size_t);
void free_img(unsigned char **);

/* Subroutine in solve.c */
double solve(double (*)(double), double, double, double, int *);

/* Subroutine in invert.c */
int invert(double **, int);

#ifdef GRASS_IMAGERY_H
int segment(struct SigSet *, struct parms *, struct files *);

/* read_sig.c */
int read_signatures(struct parms *, struct SigSet *);

/* labels.c */
int create_output_labels(struct SigSet *, struct files *);

/* write_img.c */
int write_img(unsigned char **, float **, int, int, struct SigSet *, struct parms *,
	      struct files *);
#endif

/*  Look for prototypes that use the Region structure in region.h */
