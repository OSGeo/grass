#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>

struct dxf_file
{
    char *name;
    FILE *fp;
    /* for G_percent() */
    off_t size, pos;
    int curr_pos;
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
GLOBAL char **opt_layers;
GLOBAL char dxf_buf[DXF_BUF_SIZE];
GLOBAL int arr_max;
GLOBAL double *xpnts, *ypnts, *zpnts;
GLOBAL struct line_pnts *Points;

/* dxf_to_vect.c */
int dxf_to_vect(struct dxf_file *, struct Map_info *);
int check_ext(double, double, double);

/* layer_list.c */
void add_layer_to_list(const char *, int);
int is_layer_in_list(const char *);
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
void add_point(struct dxf_file *, struct Map_info *);

/* add_line.c */
void add_line(struct dxf_file *, struct Map_info *);

/* add_lwpolyline.c */
void add_lwpolyline(struct dxf_file *, struct Map_info *);

/* add_polyline.c */
void add_polyline(struct dxf_file *, struct Map_info *);

/* add_3dface.c */
void add_3dface(struct dxf_file *, struct Map_info *);

/* add_arc.c */
void add_arc(struct dxf_file *, struct Map_info *);

/* add_circle.c */
void add_circle(struct dxf_file *, struct Map_info *);

/* add_text.c */
void add_text(struct dxf_file *, struct Map_info *);

/* write_vect.c */
void write_vect(struct Map_info *, char *, char *, char *, char *, int, int);
int write_done(struct Map_info *);

#endif
