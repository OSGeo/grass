#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>
#include <grass/glocale.h>

extern FILE *dxf_fp;
extern int overwrite;

/* write_dxf.c */
int dxf_open(char *);
int dxf_header(void);
int dxf_tables(void);
int dxf_blocks(void);
int dxf_entities(void);
int dxf_endsec(void);
int dxf_eof(void);

/* header stuff */
int dxf_limits(double, double, double, double);

/* tables stuff */
int dxf_linetype_table(int);
int dxf_layer_table(int);
int dxf_endtable(void);
int dxf_solidline(void);
int dxf_layer0(void);
int dxf_layer(char *, int, char *, int);

/* entities */
int dxf_point(char *, double, double, double);
int dxf_polyline(char *);
int dxf_vertex(char *, double, double, double);
int dxf_text(char *, double, double, double, double, int, char *);
int dxf_poly_end(char *);

#endif
