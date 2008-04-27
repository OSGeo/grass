#include <grass/imagery.h>

#ifndef GLOBAL
#define GLOBAL extern
#endif

GLOBAL char *group;
GLOBAL char *subgroup;
GLOBAL char *sigfile;
GLOBAL struct Ref Ref;
GLOBAL struct Signature S;
GLOBAL CELL **cell;
GLOBAL int *cellfd;
GLOBAL CELL *class_cell, *reject_cell;
GLOBAL int class_fd, reject_fd;
GLOBAL char *class_name, *reject_name;
GLOBAL double *B;
GLOBAL double *P;
