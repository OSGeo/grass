/*
 * SPDX-FileCopyrightText: 1994-1995 James Darrell McCauley
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

typedef struct {
    double x, y;
} COOR;

void count_sites(COOR *, int, int *, double, struct Map_info *, int);
COOR *find_quadrats(int, double, struct Cell_head);
void qindices(int *, int, double *, double *, double *, double *, double *,
              double *);
