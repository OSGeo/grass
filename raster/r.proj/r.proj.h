/* @(#)r.proj.h v1.2 - 27 Jun 1995      -emes- */

#ifndef R_PROJ_H
#define R_PROJ_H

typedef void (*func) (FCELL **, void *, int, double *, double *,
		      struct Cell_head *);

struct menu
{
    func method;		/* routine to interpolate new value      */
    char *name;			/* method name                           */
    char *text;			/* menu display - full description       */
};

extern void bordwalk(struct Cell_head *, struct Cell_head *, struct pj_info *,
		     struct pj_info *);
extern FCELL **readcell(int);

/* declare resampling methods */
/* bilinear.c */
extern void p_bilinear(FCELL **, void *, int, double *, double *,
		       struct Cell_head *);
/* cubic.c */
extern void p_cubic(FCELL **, void *, int, double *, double *,
		    struct Cell_head *);
/* nearest.c */
extern void p_nearest(FCELL **, void *, int, double *, double *,
		      struct Cell_head *);

#endif
