/*!
   \file lib/imagery/iclass_local_proto.h

   \brief Imagery library - functions for wx.iclass

   Computation based on training areas for supervised classification.
   Based on i.class module (GRASS 6).

   Copyright (C) 1999-2007, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author David Satnik, Central Washington University (original author)
   \author Markus Neteler <neteler itc.it> (i.class module)
   \author Bernhard Reiter <bernhard intevation.de> (i.class module)
   \author Brad Douglas <rez touchofmadness.com>(i.class module)
   \author Glynn Clements <glynn gclements.plus.com> (i.class module)
   \author Hamish Bowman <hamish_b yahoo.com> (i.class module)
   \author Jan-Oliver Wagner <jan intevation.de> (i.class module)
   \author Anna Kratochvilova <kratochanna gmail.com> (rewriting for wx.iclass)
   \author Vaclav Petras <wenzeslaus gmail.com> (rewriting for wx.iclass)
 */

#ifndef ICLASS_LOCAL_PROTO_H
#define ICLASS_LOCAL_PROTO_H

#include <grass/imagery.h>
#include <grass/raster.h>
#include <grass/vector.h>

#define MAX_CATS 256

/*! Point of area perimeter */
typedef struct
{
    int x;			/*!< column */
    int y;			/*!< row */

} IClass_point;

/*! Holds perimeter points of one area.

   Perimeter is represented by rasterized area outline
   (not only vertices).
 */
typedef struct
{
    int npoints;
    IClass_point *points;

} IClass_perimeter;

/*! Holds perimeters of training areas. */
typedef struct
{
    int nperimeters;
    IClass_perimeter *perimeters;

} IClass_perimeter_list;

/* iclass_statistics.c */
void alloc_statistics(IClass_statistics * statistics, int nbands);

int make_statistics(IClass_statistics * statistics,
		    IClass_perimeter * perimeter, CELL ** band_buffer,
		    int *band_fd);
int make_all_statistics(IClass_statistics * statistics,
			IClass_perimeter_list * perimeters,
			CELL ** band_buffer, int *band_fd);
void create_raster(IClass_statistics * statistics, CELL ** band_buffer,
		   int *band_fd, const char *raster_name);
void band_range(IClass_statistics *, int);

float mean(IClass_statistics * statistics, int band);

float stddev(IClass_statistics * statistics, int band);

float var(IClass_statistics * statistics, int band1, int band2);

float var_signature(IClass_statistics * statistics, int band1, int band2);

/* iclass_bands.c */
void open_band_files(struct Ref *refer, CELL *** band_buffer, int **band_fd);

void close_band_files(struct Ref *refer, CELL ** band_buffer, int *band_fd);

void read_band_row(CELL ** band_buffer, int *band_fd, int nbands, int row);

/* iclass_perimeter.c */
int vector2perimeters(struct Map_info *, const char *layer_name,
		      int category, IClass_perimeter_list * perimeters,
		      struct Cell_head *band_region);
int make_perimeter(struct line_pnts *points, IClass_perimeter * perimeter,
		   struct Cell_head *band_region);
int edge2perimeter(IClass_perimeter * perimeter, int x0, int y0, int x1,
		   int y1);
void perimeter_add_point(IClass_perimeter * perimeter, int x, int y);

int edge_order(const void *aa, const void *bb);

void free_perimeters(IClass_perimeter_list * perimeters);

/* iclass.c */
int init_group(const char *group_name, struct Ref *refer);

#endif /* ICLASS_LOCAL_PROTO_H */
