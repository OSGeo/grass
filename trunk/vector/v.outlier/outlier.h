#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/lidar.h>

/*--------------------------------------------------------------------------*/
/*FUNCTIONS DECLARATION */
void
P_Outlier(struct Map_info *, /**/
	  struct Map_info *, /**/
	  struct Map_info *, /**/
	  struct Cell_head, /**/
	  struct bound_box, /**/
	  struct bound_box, /**/
	  double **, /**/
	  double *, /**/
	  double, /**/ double, /**/ int *, /**/ int, /**/ dbDriver *, /**/ char *);

int Insert_Outlier(double, /**/ int, /**/ dbDriver *, /**/ char *);

int UpDate_Outlier(double, /**/ int, /**/ dbDriver *, /**/ char *);

int Select_Outlier(double *, /**/ int, /**/ dbDriver *, /**/ char *);

void P_set_outlier_fn(int);
int P_is_outlier(double);
int P_is_outlier_p(double);
int P_is_outlier_n(double);
