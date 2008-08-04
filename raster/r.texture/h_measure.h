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

float h_measure(int **, int, int, int, int);
float f1_asm(float **, int);
float f2_contrast(float **, int);
float f3_corr(float **, int);
float f4_var(float **, int);
float f5_idm(float **, int);
float f6_savg(float **, int);
float f7_svar(float **, int, double);
float f8_sentropy(float **, int);
float f9_entropy(float **, int);
float f10_dvar(float **, int);
float f11_dentropy(float **, int);
float f12_icorr(float **, int);
float f13_icorr(float **, int);
float f14_maxcorr(float **, int);
float *vector(int, int);
float **matrix(int, int, int, int);
void results(char *, float *);
void simplesrt(int, float[]);
void mkbalanced(float **, int);
void reduction(float **, int);
void hessenberg(float **, int, float[], float[]);
