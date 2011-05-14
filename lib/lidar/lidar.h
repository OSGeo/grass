
/***********************************************************************
 *
 * MODULE:       lidarlib
 *
 * AUTHOR(S):    Roberto Antolin
 *
 * PURPOSE:      LIDAR library
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *                           Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************************/

#ifndef _POLIMIFUNCT_H_
#define _POLIMIFUNCT_H_

#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include <grass/glocale.h>

/*----------------------------------------------------------------------------------------------------------*/
/*CONSTANTS DECLARATION */

#define NSPLX_MAX 	      150	/* Maximum number of splines along East direction used in the subregions interpolation */
#define NSPLY_MAX	      150	/* Maximum number of splines along North direction used in the subregions interpolation */
#define OVERLAP_SIZE 	       10	/* Subregions overlapping size. */
#define LATO 		     1000	/* Side's size for v.lidar.growing. */
#define CONTOUR		       15 	/**/
#define GENERAL_ROW 		0
#define GENERAL_COLUMN 	        1
#define FIRST_ROW 		2
#define LAST_ROW 		3
#define FIRST_COLUMN	 	4
#define LAST_COLUMN 		5
    /* FIELDS ID */
#define F_EDGE_DETECTION_CLASS	1
#define F_CLASSIFICATION	2
#define F_INTERPOLATION		3
#define F_COUNTER_OBJ		4
    /* PRE-CLASSIFICATION */
#define PRE_TERRAIN 		1
#define PRE_EDGE		2
#define PRE_UNKNOWN		3
    /* FINAL CLASSIFICATION */
#define TERRAIN_SINGLE 	        1
#define TERRAIN_DOUBLE	        2
#define OBJECT_DOUBLE		3
#define OBJECT_SINGLE		4
    /* SINGLE OR DOUBLE PULSE */
#define SINGLE_PULSE	        1
#define DOUBLE_PULSE	        2
    /* INTERPOLATOR */
#define P_BILINEAR 		1
#define P_BICUBIC 		0
    /* Boolean definitions */
#define TRUE 			1
#define FALSE 			0

/*----------------------------------------------------------------------------------------------------------*/
    /*STRUCTS DECLARATION */
    struct Reg_dimens
{
    double edge_h;		/*Horizontal tile edge */
    double edge_v;		/*Vertical tile edge */
    double overlap;		/*Tile's overlapping size */
    double sn_size;		/*South-North side size */
    double ew_size;		/*East-West side size */
};

struct Point
{
    double coordX;
    double coordY;
    double coordZ;
    int lineID;
    int cat;
};

struct element
{
    double value;
    double residual;
    int freq;
};

/*----------------------------------------------------------------------------------------------------------*/
/*FUNCTIONS DECLARATION */
/*zones */
void P_zero_dim(struct Reg_dimens * /**/);
int P_set_dim(struct Reg_dimens *, double, double, int *, int *);

int P_set_regions(struct Cell_head *, /**/
		  struct bound_box *, /**/
		  struct bound_box *, /**/ struct Reg_dimens, /**/ int /**/);

int P_get_edge(int, /**/ struct Reg_dimens *, /**/ double, /**/ double /**/);

int P_get_BandWidth(int, /**/ int /**/);

double P_estimate_splinestep(struct Map_info *, double *, double *);

struct Point *P_Read_Vector_Region_Map(struct Map_info *, /**/
				       struct Cell_head *, /**/
				       int *, /**/ int, /**/ int /**/);

struct Point *P_Read_Raster_Region_Map(SEGMENT *, /**/
				       struct Cell_head *, /**/
				       struct Cell_head *, /**/
				       int *, /**/ int /**/);

double P_Mean_Calc(struct Cell_head *, /**/ struct Point *, /**/ int /**/);

/*----------------------------------------------------------------------------------------------------------*/
void
P_Sparse_Points(struct Map_info *, /**/
		struct Cell_head *, /**/
		struct bound_box, /**/
		struct bound_box, /**/
		double **, /**/
		double *, /**/
		int *, /**/
		double, /**/
		double, /**/
		double, /**/
		int, /**/
		int, /**/
		int, /**/
		int, /**/
		struct line_cats *, /**/
		dbDriver *, /**/ double, /**/ char * /**/);

int P_Regular_Points(struct Cell_head *, /**/
                          struct Cell_head *, /**/
			  struct bound_box, /**/
			  struct bound_box, /**/
			  SEGMENT *, /**/
			  double *, /**/
			  double, /**/
			  double, /**/
			  double, /**/
			  double, /**/
			  int, /**/ int, /**/ int, /**/ int, /**/ int /**/);

/*----------------------------------------------------------------------------------------------------------*/
int P_Create_Aux2_Table(dbDriver *, /**/ char * /**/);

int P_Create_Aux4_Table(dbDriver *, /**/ char * /**/);

int P_Drop_Aux_Table(dbDriver *, /**/ char * /**/);

/*----------------------------------------------------------------------------------------------------------*/
void P_Aux_to_Raster(double **, /**/ int /**/);

void P_Aux_to_Vector(struct Map_info *, /**/
		     struct Map_info *, /**/ dbDriver *, /**/ char * /**/);

double **P_Null_Matrix(double ** /**/);

/*---------------------------------------------------------------------------------------*/
/*interpSpline */
void normalDefBicubic(double **N, double *TN, double *Q, double **obsVect,
		      double deltaX, double deltaY, int xNum, int yNum,
		      double xMin, double yMin, int obsNum, int parNum,
		      int BW);

void normalDefBilin(double **N, double *TN, double *Q, double **obsVect,
		    double deltaX, double deltaY, int xNum, int yNum,
		    double xMin, double yMin, int obsNum, int parNum, int BW);

void nCorrectLapl(double **N,	/* Normal Matrix () */
		  double lambda,	/*  */
		  int xNum,	/*  */
		  int yNum,	/*  */
		  double deltaX,	/*  */
		  double deltaY);	/*  */

void nCorrectGrad(double **N, double lambda, int xNum, int yNum,
		  double deltaX, double deltaY);

void obsEstimateBicubic(double **obsV,	/*  */
			double *obsE,	/*  */
			double *parV,	/*  */
			double deltX,	/*  */
			double deltY,	/*  */
			int xNm,	/*  */
			int yNm,	/*  */
			double xMi,	/*  */
			double yMi,	/*  */
			int obsN);	/*  */

double dataInterpolateBicubic(double x,	/*  */
			      double y,	/*  */
			      double deltaX,	/*  */
			      double deltaY,	/*  */
			      int xNum,	/*  */
			      int yNum,	/*  */
			      double xMin,	/*  */
			      double yMin,	/*  */
			      double *parVect);	/*  */

void obsEstimateBilin(double **obsV, double *obsE, double *parV, double deltX,
		      double deltY, int xNm, int yNm, double xMi, double yMi,
		      int obsN);

double dataInterpolateBilin(double x, double y, double deltaX, double deltaY,
			    int xNum, int yNum, double xMin, double yMin,
			    double *parVect);

#endif
