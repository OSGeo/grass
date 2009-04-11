#ifndef __USEREXTERNS_H__
#define __USEREXTERNS_H__

extern int KMAX2, KMIN, KMAX, KMAXPOINTS;
extern struct octtree *root;
extern int NPOINT;
extern int OUTRANGE;
extern int NPT;

extern int nsizr, nsizc, nsizl, total;


extern char *input;
extern char *cellinp;
extern char *cellout;
extern char *scol;
extern char *maskmap;
extern char *mapset;
extern char *devi;
extern char *outz, *gradient, *aspect1, *aspect2, *ncurv, *gcurv, *mcurv;
extern char *cvdev;


extern double ns_res, ew_res, tb_res;
extern struct BM *bitmask;
extern double *az, *adx, *ady, *adz, *adxx, *adyy, *adxy, *adxz, *adyz, *adzz;
extern double ertot, ertre, zminac, zmaxac, dmin, wmult, zmult, zminacell,
    zmaxacell;


extern double DETERM;
extern int NERROR, cond1, cond2;

extern FILE *fdinp, *fdzout, *fd4;
extern int fdcell, fdcout;

extern int sdisk, disk;
extern FILE *Tmp_fd_z;
extern char *Tmp_file_z;
extern FILE *Tmp_fd_dx;
extern char *Tmp_file_dx;
extern FILE *Tmp_fd_dy;
extern char *Tmp_file_dy;
extern FILE *Tmp_fd_xx;
extern char *Tmp_file_xx;
extern FILE *Tmp_fd_yy;
extern char *Tmp_file_yy;
extern FILE *Tmp_fd_xy;
extern char *Tmp_file_xy;
extern FILE *Tmp_fd_dz;
extern char *Tmp_file_dz;
extern FILE *Tmp_fd_cell;
extern char *Tmp_file_cell;

extern struct Cell_head cellhd;

#endif
