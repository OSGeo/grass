
#ifndef CRS3D_H
#define CRS3D_H_

#define MAXORDER 3

struct Control_Points_3D
{
    int count;
    double *e1;
    double *n1;
    double *z1;
    double *e2;
    double *n2;
    double *z2;
    int *status;
};


/* crs3d.c */
int CRS_compute_georef_equations_3d(struct Control_Points_3D *,
                                    double *, double *, double *,
				    double *, double *, double *,
				    int);
int CRS_georef_3d(double, double, double,
                  double *, double *, double *,
		  double *, double *, double *,
		  int);

/* orthorot.c */
int CRS_compute_georef_equations_or(struct Control_Points_3D *,
                                    double *, double *);
int CRS_georef_or(double, double, double,
                  double *, double *, double *,
		  double *);

#endif
