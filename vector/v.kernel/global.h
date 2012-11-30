#include <grass/gis.h>

#define NODE_NONE                   0
/* According Okabe 2009 */
#define NODE_SIMILAR                1
#define NODE_EQUAL_SPLIT            2
#define NODE_CONTINUOUS_EQUAL_SPLIT 3

#define KERNEL_UNIFORM      0
#define KERNEL_TRIANGULAR   1
#define KERNEL_EPANECHNIKOV 2
#define KERNEL_QUARTIC      3
#define KERNEL_TRIWEIGHT    4
#define KERNEL_GAUSSIAN     5
#define KERNEL_COSINE       6


void setKernelFunction(int function, int dimension, double bandwidth, double *term);
double kernelFunction(double term, double bandwidth, double x);

double euclidean_distance(double *x, double *y, int n);
double gaussian2dBySigma(double d, double sigma);
double gaussianFunction(double x, double sigma, double dimension);
double gaussianKernel(double x, double term);

double invGaussian2d(double sigma, double prob);
double gaussian2dByTerms(double d, double term1, double term2);
double brent_iterate(double (*f) (), double x_lower, double x_upper,
		     int maxiter);
double kernel1(double d, double rs, double lambda);
double segno(double x);

/* main.c */
int read_points(struct Map_info *In, double ***coordinate, double dsize);
double compute_all_distances(double **coordinate, double **dists, int n,
			     double dmax);
double compute_all_net_distances(struct Map_info *In, struct Map_info *Net,
				 double netmax, double **dists, double dmax);
void compute_distance(double N, double E, double sigma, double term,
                      double *gaussian, double dmax, struct bound_box *box,
		      struct boxlist *NList);
void compute_net_distance(double x, double y, struct Map_info *In,
			  struct Map_info *Net, double netmax, double sigma,
			  double term, double *gaussian, double dmax, int node_method);
