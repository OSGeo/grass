#ifndef GRASS_EDIT_H
#define GRASS_EDIT_H

#include <grass/gis.h>

/* edit_cats.c */
int E_edit_cats(const char *, struct Categories *, int);
int E_edit_fp_cats(const char *, struct Categories *);

/* edit_cellhd.c */
int E_edit_cellhd(struct Cell_head *, int);

/* edit_hist.c */
int E_edit_history(struct History *);

#endif
