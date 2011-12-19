
/***************************************************************************/

/***************************************************************************/
/*
   CRS.H - Center for Remote Sensing rectification routines

   Written By: Brian J. Buckley

   At: The Center for Remote Sensing
   Michigan State University
   302 Berkey Hall
   East Lansing, MI  48824
   (517)353-7195

   Written: 12/19/91

   Last Update: 12/26/91 Brian J. Buckley
 */

/***************************************************************************/

/***************************************************************************/

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


/* crs.c */
int CRS_compute_georef_equations(struct Control_Points *, double *,
					double *, double *, double *, int);
int CRS_georef(double, double, double *, double *, double *, double *,
		      int);

/* crs3d.c */
int CRS_compute_georef_equations_3d(struct Control_Points_3D *,
                                    double *, double *, double *,
				    double *, double *, double *,
				    int);
int CRS_georef_3d(double, double, double,
                  double *, double *, double *,
		  double *, double *, double *,
		  int);
