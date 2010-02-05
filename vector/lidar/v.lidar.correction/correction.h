#include <assert.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <grass/PolimiFunct.h>

void
P_Sparse_Correction(struct Map_info *, /**/
		    struct Map_info *, /**/
		    struct Map_info *, /**/
		    struct Cell_head *, /**/
		    struct bound_box, /**/
		    struct bound_box, /**/
		    double **, /**/
		    double *, /**/
		    int *, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    int, /**/
		    int, /**/
		    int, /**/
		    dbDriver *, /**/
		    double, /**/
		    char *);

int Insert_Correction(double, /**/ int, /**/ dbDriver *, /**/ char *);

int UpDate_Correction(double, /**/ int, /**/ dbDriver *, /**/ char *);

int Select_Correction(double *, /**/ int, /**/ dbDriver *, /**/ char *);

struct Point *P_Read_Vector_Correction(struct Map_info *, /**/
				       struct Cell_head *, /**/
				       int *, /**/ int *, /**/ int /**/);

int correction(int, double, double, double, double);
