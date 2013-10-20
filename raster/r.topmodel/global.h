#include <stdio.h>

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


/* file_io.c */
void get_line(FILE * fp, char *buffer);
void read_input(void);
void write_output(void);

/* topmodel.c */
void create_topidxstats(char *topidx, int ntopidxclasses, char *outtopidxstats);
double calculate_lambda(void);
void initialize(void);
void calculate_flows(void);
double calculate_Em(void);
void calculate_others(void);
void run_topmodel(void);

/* infiltration.c */
double calculate_f(double t, double R);


/* Topographic index statistics file */
struct topidxstats
{
    /* misc.ntopidxclasses */
    double *atb;
    double *Aatb_r;
};

/* Parameters file */
struct params
{
    char *name;
    double A;
    double qs0;
    double lnTe;
    double m;
    double Sr0;
    double Srmax;
    double td;
    double vch;
    double vr;
    int infex;
    double K0;
    double psi;
    double dtheta;
    int nch;
    /* params.nch's */
    double *d;
    double *Ad_r;
};

/* Input file */
struct input
{
    int ntimesteps;
    double dt;
    /* input.ntimestep's */
    double *R;
    double *Ep;
};

/* File names */
struct file
{
    char *params;
    char *topidxstats;
    char *input;
    char *output;
    char *qobs;
};

/* Miscellaneous TOPMODEL variables */
struct misc
{
    /* Number of non-null cells */
    int ncells;
    /* Number of topographic index classes */
    int ntopidxclasses;
    /* Model efficiency */
    double Em;
    int ndelays;
    int nreaches;
    double lnTe;
    double vch;
    double vr;
    double lambda;
    double qss;
    double qs0;
    double Qobs_peak;
    double Qt_peak;
    double Qobs_mean;
    double Qt_mean;
    int tobs_peak;
    int tt_peak;
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
    /* input.ntimestep * (misc.ntopidxclasses + 1)'s */
    double **qt;
    double **qo;
    double **qv;
    /* input.ntimestep * misc.ntopidxclassess */
    double **Srz;
    double **Suz;
    double **S;
    double **Ea;
    double **ex;
    /* Miscellaneous variables */
    int timestep;
    int topidxclass;
};

#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL struct params params;
GLOBAL struct topidxstats topidxstats;
GLOBAL struct input input;
GLOBAL struct file file;
GLOBAL struct misc misc;

/* Miscellaneous variables */
GLOBAL char buf[BUFSIZE];
