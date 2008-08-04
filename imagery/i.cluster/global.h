#include <grass/config.h>

#ifndef TIME_WITH_SYS_TIME
#ifdef HAVE_TIME
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#else
#include <time.h>
#include <sys/time.h>
#endif

#include <grass/imagery.h>
#include <grass/cluster.h>

extern struct Cluster C;
extern struct Signature in_sig;

extern int maxclass;
extern double conv;
extern double sep;
extern int iters;
extern int mcs;
extern char *group;
extern char *subgroup;
extern struct Ref ref;
extern char *outsigfile;
extern char *insigfile;
extern char *reportfile;
extern DCELL **cell;
extern int *cellfd;
extern FILE *report;
extern int sample_rows, sample_cols;
extern int verbose;
extern time_t start_time;
