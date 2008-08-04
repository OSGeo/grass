/* color_rnge.c */
double decide_color_range(double, double, double);

/* newsegment.c */
struct point *segment(int, int, int, double, double, int, int, int, int,
		      SEGMENT *, SEGMENT *, SEGMENT *, int, int, int, int,
		      double);
/* pts_elim.c */
double find_orientation(int, int, int);
double find_inclination(int, int, int, SEGMENT *, int, int, int, double);
