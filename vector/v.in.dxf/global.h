#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/Vect.h>

struct dxf_file
{
    char *name;
    FILE *fp;
    /* for G_percent() */
    unsigned long size, pos;
    int percent;
};

#define UNIDENTIFIED_LAYER "UNIDENTIFIED"
#define ARR_INCR 256
#define DXF_BUF_SIZE 256

#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int flag_list, flag_extent, flag_table, flag_topo, flag_invert,
    flag_one_layer, flag_frame;
GLOBAL int num_layers, found_layers;
GLOBAL char **layers;
GLOBAL char dxf_buf[DXF_BUF_SIZE], entity[DXF_BUF_SIZE];
GLOBAL int ARR_MAX;
GLOBAL double *xpnts, *ypnts, *zpnts;
GLOBAL struct line_pnts *Points;

/* dxf_to_vect.c */
int dxf_to_vect(struct dxf_file *, struct Map_info *);
int check_ext(double, double, double);
void set_entity(char *);

/* layer_list.c */
void add_layer_to_list(char *);
int is_layer_in_list(char *);
void init_list(void);

/* read_dxf.c */
struct dxf_file *dxf_open(char *);
void dxf_close(struct dxf_file *);
int dxf_find_header(struct dxf_file *);
#define dxf_get_code(a) dxf_read_code(a, dxf_buf, DXF_BUF_SIZE)
int dxf_read_code(struct dxf_file *, char *, int);

/* make_arc.c */
int make_arc(int, double, double, double, double, double, double);
int make_arc_from_polyline(int, double, double);

/* add_point.c */
int add_point(struct dxf_file *, struct Map_info *);
/* add_line.c */
int add_line(struct dxf_file *, struct Map_info *);
/* add_lwpolyline.c */
int add_lwpolyline(struct dxf_file *, struct Map_info *);
/* add_polyline.c */
int add_polyline(struct dxf_file *, struct Map_info *);
/* add_3dface.c */
int add_3dface(struct dxf_file *, struct Map_info *);
/* add_arc.c */
int add_arc(struct dxf_file *, struct Map_info *);
/* add_circle.c */
int add_circle(struct dxf_file *, struct Map_info *);
/* add_text.c */
int add_text(struct dxf_file *, struct Map_info *);

/* write_vect.c */
#define write_point(a, b) write_vect(a, b, entity, "", 1, GV_POINT)
#define write_line(a, b, c) write_vect(a, b, entity, "", c, GV_LINE)
#define write_face(a, b, c) write_vect(a, b, entity, "", c, GV_FACE)
#define write_text(a, b, c) write_vect(a, b, entity, c, 1, GV_POINT)
void write_vect(struct Map_info *, char *, char *, char *, int, int);
void write_done(struct Map_info *);

#endif
