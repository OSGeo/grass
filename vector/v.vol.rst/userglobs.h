#ifndef __USERGLOBS_H__
#define __USERGLOBS_H__

#include <grass/raster3d.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

extern double /* pargr */ xmin, xmax, ymin, ymax, zmin, zmax, wmin, wmax;
extern double /* norm */ xmin0, xmax0, ymin0, ymax0, zmin0, zmax0, wmin0,
    wmax0, delt, dnorm;
extern double /* MAT */ *A;
extern double /* PRISP */ fi, rsm, fstar2, alphat, betat;

extern double /* out */ *b, *w;
extern double /* orig */ x0utm, y0utm, z0utm;
extern double /* gcmax */ gmin, gmax, c1min, c1max, c2min, c2max, c3min,
    c3max;
extern double /* gcmax */ a1min, a1max, a2min, a2max;
extern float *zero_array1, *zero_array2, *zero_array3, *zero_array4,
    *zero_array5, *zero_array6, *zero_array7;
extern int out_cond1, out_cond2;
extern double xmn, xmx, ymn, ymx, zmn, zmx;
extern double z_orig_in, tb_res_in;

extern int cursegm;
extern int totsegm;
extern int iw2;
extern int n_rows_in;		/* fix by JH 04/24/02 */
extern int cv;
extern int sig1;

extern char msg[80];

extern struct Map_info Map;
extern dbString sql;
extern dbDriver *driver;
extern dbHandle handle;
extern struct field_info *f;
extern struct line_pnts *Pnts;
extern struct line_cats *Cats;
extern char buf[1024];
extern int count;

extern FILE *dev, *cvdevf;
extern FCELL *zero_array_cell;
extern RASTER3D_Region current_region;

#endif
