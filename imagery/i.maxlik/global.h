#include <grass/imagery.h>

extern char *group;
extern char *subgroup;
extern char *sigfile;
extern struct Ref Ref;
extern struct Signature S;
extern DCELL **cell;
extern int *cellfd;
extern CELL *class_cell, *reject_cell;
extern int class_fd, reject_fd;
extern char *reject_name;
extern char class_name[GNAME_MAX];
extern double *B;
extern double *P;
