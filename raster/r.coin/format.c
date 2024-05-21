/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *

***************************************************************************/

#include <stdio.h>
#include <string.h>

int format_double(double v, char *buf, int n)
{
    char fmt[15];
    int k;

    sprintf(fmt, "%%%d.2lf", n);
    sprintf(buf, fmt, v);

<<<<<<< HEAD
<<<<<<< HEAD
    for (k = n; (ssize_t)strlen(buf) > n; k--) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (k = n; (ssize_t)strlen(buf) > n; k--) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    for (k = n; (ssize_t)strlen(buf) > n; k--) {
=======
>>>>>>> osgeo-main
    for (k = n; strlen(buf) > n; k--) {
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (k = n; (ssize_t)strlen(buf) > n; k--) {
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
    for (k = n; strlen(buf) > n; k--) {
=======
    for (k = n; (ssize_t)strlen(buf) > n; k--) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        sprintf(fmt, "%%%d.%dg", n, k);
        sprintf(buf, fmt, v);
    }

    return 0;
}
