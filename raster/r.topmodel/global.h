#include <stdio.h>

#define	BUF_SIZE	1024

/* file_io.c */
void get_line(FILE * fp, char *buffer);
void read_input(void);
void write_output(void);

/* topmodel.c */
void create_topidxstats(char *topidx, int ntopidxclasses, char *outtopidxstats);
double calculate_lambda(void);
void initialize(void);
void calculate_flows(void);
double calculate_efficiency(void);
void calculate_others(void);
void run_topmodel(void);

/* infiltration.c */
double calculate_infiltration(int timestep, double R);


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
    double *Ad;
};

/* Topographic index statistics file */
struct topidxstats
{
    /* misc.ntopidxclasses */
    double *atb;
    double *Aatb_r;
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
};

/* Miscellaneous TOPMODEL variables */
struct misc
{
    /* Number of non-null cells */
    int ncells;
    /* Number of topographic index classes */
    int ntopidxclasses;
    int delay;
    int tc;
    int tcsub;
    double lnTe;
    double vch;
    double vr;
    double lambda;
    double qss;
    double qs0;
    double Qt_peak;
    double Qt_mean;
    int tobs_peak;
    int tt_peak;
    /* params.nch's */
    double *tch;
    /* misc.tcsub's */
    double *Ad;
    /* input.ntimestep's */
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
GLOBAL char buf[BUF_SIZE];
