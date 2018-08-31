
/* exit codes */
#define SP_FATAL     1		/* [ G_fatal_error () returns 1 ] */
#define SP_NOCHANGE  2
#define SP_UNKOWN    3

/* $GISBASE-relative locations of parameter files */
#define STP1927PARAMS "/etc/proj/state27"
#define STP1983PARAMS "/etc/proj/state83"

#define RADIUS_DEF 6370997.

struct proj_unit
{
    const char *units;
    const char *unit;
    double fact;
};

struct proj_desc
{
    const char *name;
    const char *type;
    const char *key;
    const char *desc;
};

struct proj_parm
{
    const char *name;
    int ask;
    int def_exists;
    double deflt;
};

/* get_deg.c */
int get_deg(char *, int);

/* get_num.c */
int get_double(const struct proj_parm *, const struct proj_desc *, double *);
int get_int(const struct proj_parm *, const struct proj_desc *, int *);
int get_LL_stuff(const struct proj_parm *, const struct proj_desc *, int,
		 double *);
int get_zone(void);
double prompt_num_double(char *, double, int);
int prompt_num_int(char *, int, int);

/* get_stp.c */
void get_stp_proj(char[]);
int get_stp_code(int, char *, char *);
int get_stp_num(void);
int ask_fips(FILE *, int *, int *, int *);

/* main.c */
/* some global variables */
extern int ier, proj_index, zone, snum, spath;

extern double radius, kfact, mfact, msfact, nfact, qfact,
    wfact, unit_fact, x_false, y_false, heigh, azim, tilt;
int min1(int, int);

#ifdef __GNUC_MINOR__
int leave(int) __attribute__ ((__noreturn__));
#else
int leave(int);
#endif

/* this is from gislib! */
/* table.c */
int init_table(void);
int get_proj_index(char *);
int init_unit_table(void);

/* get_datum.c */
int ask_datum(char *, char *, char *);

/* proj.c */
struct proj_unit *get_proj_unit(const char *arg);
struct proj_desc *get_proj_desc(const char *arg);
struct proj_parm *get_proj_parms(const char *arg);
