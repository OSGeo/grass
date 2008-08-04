#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <grass/gis.h>


#define	FILL		0x1
#define	DIR		0x2
#define	BELEV		0x4
#define	TOPIDX		0x8
#define	IDXSTATS	0x10
#define	OUTPUT		0x20

#define	BUFSIZE		1024
#define	ZERO		0.0000001
#define	TOLERANCE	0.00001
#define	MAXITER		20
#define	NTERMS		10


/* check_ready.c */
int check_ready(void);
int check_required(void);
int check_names(void);
int check_io(void);

/* misc.c */
int run(char *buf);
void gregion(void);
void depressionless(void);
void basin_elevation(void);
void top_index(void);

/* file_io.c */
void get_line(FILE * fp, char *buffer);
void read_inputs(void);
void write_outputs(void);

/* topmodel.c */
double get_lambda(void);
void initialize(void);
void implement(void);
double get_Em(void);
void others(void);
void topmodel(void);

/* infiltration.c */
double get_f(double t, double R);


#ifdef MAIN
#	define	GLOBAL
#else
#	define	GLOBAL	extern
#endif


/* Topographic index statistics file */
GLOBAL struct
{
    /* misc.nidxclass's */
    double *atb, *Aatb_r;
} idxstats;

/* Parameters file */
GLOBAL struct
{
    char *name;
    double A, qs0, lnTe, m, Sr0, Srmax, td, vch, vr;
    int infex;
    double K0, psi, dtheta;
    int nch;
    /* params.nch's */
    double *d, *Ad_r;
} params;

/* Input file */
GLOBAL struct
{
    int ntimestep;
    double dt;
    /* input.ntimestep's */
    double *R, *Ep;
} input;

/* Map names */
GLOBAL struct
{
    char *basin, *elev, *belev, *fill, *dir, *topidx;
} map;

/* File names */
GLOBAL struct
{
    char *idxstats, *params, *input, *output, *Qobs;
} file;

/* Miscellaneous TOPMODEL variables */
GLOBAL struct
{
    /* Number of non-null cells */
    int ncell;
    /* Number of topographic index classes */
    int nidxclass;
    /* Model efficiency */
    double Em;
    int ndelay, nreach;
    double lnTe, vch, vr;
    double lambda;
    double qss, qs0;
    double Qobs_peak, Qt_peak, Qobs_mean, Qt_mean;
    int tobs_peak, tt_peak;
    /* params.nch's */
    double *tch;
    /* misc.nreach's */
    double *Ad;
    /* input.ntimestep's */
    double *Qobs;
    double *Qt;
    double *qs;			/* spatially constant? */
    double *S_mean;
    double *f;
    double *fex;
    /* input.ntimestep * (misc.nidxclass + 1)'s */
    double **qt, **qo, **qv;
    /* input.ntimestep * misc.nidxclass's */
    double **Srz, **Suz;
    double **S;
    double **Ea;
    double **ex;
    /* Miscellaneous variables */
    int timestep, idxclass;
} misc;


GLOBAL struct
{
    /* Input flag */
    char input;
    /* Overwrite flag */
    char overwr;
    /* Overwrite list */
    int overwrlist;
} flg;


/* Miscellaneous variables */
GLOBAL char *gisbase, *mapset;
GLOBAL char buf[BUFSIZE];
