#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


/* qsort() comparison function */
int dcmp(const void *, const void *);

/* misc internal support functions */
void wcoef(double[], int, int, double *, int *);
void wgp(double[], int, double, double, double, double[],
	 int, double, double, double, double, int *);
void nscor2(double[], int, int, int *);
void wext(double[], int, double, double[], int, double,
	  double *, double *, int *);

double alnorm(double, int);
double enormp(double);
double normp(double);
double xinormal(double);

double ppn7(double);
double ppnd16(double);


#endif /* __LOCAL_PROTO_H__ */
