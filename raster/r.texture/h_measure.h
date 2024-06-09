/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from:
 *                         prof. Giulio Antoniol - antoniol@ieee.org
 *                         prof. Michele Ceccarelli - ceccarelli@unisannio.it

 * PURPOSE:      Intended to explain GRASS raster programming.
 *               Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2002 by University of Sannio (BN) - Italy
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include "matvec.h"

#define BL  "Direction             "
#define F1  "Angular Second Moment "
#define F2  "Contrast              "
#define F3  "Correlation           "
#define F4  "Variance              "
#define F5  "Inverse Diff Moment   "
#define F6  "Sum Average           "
#define F7  "Sum Variance          "
#define F8  "Sum Entropy           "
#define F9  "Entropy               "
#define F10 "Difference Variance   "
#define F11 "Difference Entropy    "
#define F12 "Measure of Correlation-1 "
#define F13 "Measure of Correlation-2 "

typedef struct menu {
    char *name;   /* measure name */
    char *desc;   /* menu display - full description */
    char *suffix; /* output suffix */
    char useme;   /* calculate this measure if set */
    int idx;      /* measure index */
} menu;

float h_measure(int t_m, struct matvec *mv);

float f1_asm(struct matvec *mv);
float f2_contrast(struct matvec *mv);
float f3_corr(struct matvec *mv);
float f4_var(struct matvec *mv);
float f5_idm(struct matvec *mv);
float f6_savg(struct matvec *mv);
float f7_svar(struct matvec *mv);
float f8_sentropy(struct matvec *mv);
float f9_entropy(struct matvec *mv);
float f10_dvar(struct matvec *mv);
float f11_dentropy(struct matvec *mv);
float f12_icorr(struct matvec *mv);
float f13_icorr(struct matvec *mv);
