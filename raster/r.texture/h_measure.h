/* h_measure.c */

/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from: 
 * 			prof. Giulio Antoniol - antoniol@ieee.org
 * 			prof. Michele Ceccarelli - ceccarelli@unisannio.it

 * PURPOSE:      Intended to explain GRASS raster programming.
 *               Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2002 by University of Sannio (BN) - Italy 
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This 
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

float h_measure(int);
void alloc_vars(int);
int set_vars(int **grays, int curr_rrow, int curr_col,
                int size, int offset, int t_d, int with_nulls);
int set_angle_vars(int angle, int have_px, int have_py,
                   int have_pxpys, int have_pxpyd);
