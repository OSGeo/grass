/*
 * SPDX-FileCopyrightText: 1994 James Darrell McCauley
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define min(a, b) ((a) <= (b) ? (a) : (b))

/* clock cycle for machine */
#define CYCLE     2.9e-9 /* cycle for SX-3: */
/* #define CYCLE 6.0e-9 .* cycle for Y-MP: */
/* #define CYCLE 3.2e-9 .* cycle for VP2200 */

struct klotz0 {
    double buff[607];
    int ptr;
};

struct klotz1 {
    double xbuff[1024];
    int first, xptr;
};

/* fische.c */
int fische(int, double *, int *);

/* myrng.c */
int myrng(double *, int, int (*)(int, double *), double, double);

/* normal00.c */
int normal00(void);

/* normalen.c */
int normalen(int, double *);

/* normalrs.c */
int normalrs(double *);

/* normalsv.c */
int normalsv(double *);

/* zufall.c */
int zufall(int, double *);

/* zufalli.c */
int zufalli(int *);

/* zufallrs.c */
int zufallrs(double *);

/* zufallsv.c */
int zufallsv(double *);
