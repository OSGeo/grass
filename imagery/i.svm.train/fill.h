/*
 * i.svm.train Functions filling svm_problem struct
 *
 *   Copyright 2023 by Maris Nartiss, and the GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include <grass/config.h>
#if HAVE_SVM_H
#include <svm.h>
#elif HAVE_LIBSVM_SVM_H
#include <libsvm/svm.h>
#endif

#include <grass/imagery.h>

#ifndef FILL_H
#define FILL_H

#define SIZE_INCREMENT 64;

void fill_problem(const char *, const char *, struct Ref, const DCELL *,
                  const DCELL *, struct svm_problem *);

#endif // FILL_H
