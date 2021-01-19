/*
 * i.svm.train Functions filling svm_problem struct
 *  
 *   Copyright 2020 by Maris Nartiss, and The GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include <libsvm/svm.h>

#include <grass/imagery.h>

#ifndef FILL_H
#define FILL_H

#define SIZE_INCREMENT 64;

void fill_problem(const char *, const char *, struct Ref, const char *, struct svm_problem *);

#endif // FILL_H
