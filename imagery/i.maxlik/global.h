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
extern char *class_name, *reject_name;
extern double *B;
extern double *P;
