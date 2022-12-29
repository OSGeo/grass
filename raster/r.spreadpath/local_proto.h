#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include "point.h"


/* drawline.c */
void drawline(int x1, int y1, int x2, int y2);

/* insert.c */
void insert(POINT ** pres_pt, int row, int col, int backrow, int backcol);

/* path_finder.c */
void path_finder(int row, int col, int backrow, int backcol);

#endif
