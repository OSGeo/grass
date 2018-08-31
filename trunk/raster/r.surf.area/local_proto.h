#define X 0
#define Y 1
#define Z 2

void add_row_area(DCELL *, DCELL *, double, struct Cell_head *,
		  double *, double *);
void add_null_area(DCELL *, struct Cell_head *, double *);
void v3cross(double[], double[], double[]);
void v3mag(double[], double *);
double conv_value(double, int);
