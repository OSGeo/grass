#ifndef _LOCAL_PROTO_H
#define _LOCAL_PROTO_H


#define BAND2   0
#define BAND3   1
#define BAND4   2
#define BAND5   3
#define BAND6   4

#define NO_CLOUD      0
#define IS_CLOUD      1
#define COLD_CLOUD   30
#define WARM_CLOUD   50

#define NO_DEFINED    1
#define IS_SHADOW     2
#define IS_COLD_CLOUD 6
#define IS_WARM_CLOUD 9


typedef struct
{
    int fd;
    void *rast;
    char name[GNAME_MAX];

} Gfile;


void acca_algorithm(Gfile *, Gfile[], int, int, int);
void acca_first(Gfile *, Gfile[], int, int[], int[], int[], double[]);
void acca_second(Gfile *, Gfile, int, double, double);

int shadow_algorithm(double[]);

void filter_holes(Gfile *);

void hist_put(double t, int hist[]);
double quantile(double q, int hist[]);
double moment(int n, int hist[], int k);

#endif
