#include <time.h>
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
extern char **semantic_labels;
=======
extern char **bandrefs;
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
extern char **semantic_labels;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
extern char **semantic_labels;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
extern char outsigfile[GNAME_MAX + GMAPSET_MAX];
extern char *insigfile;
extern char *reportfile;
extern DCELL **cell;
extern int *cellfd;
extern FILE *report;
extern int sample_rows, sample_cols;
extern int verbose;
extern time_t start_time;
