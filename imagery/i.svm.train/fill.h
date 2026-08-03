/*
 * i.svm.train Functions filling svm_problem struct
 *
 *   SPDX-FileCopyrightText: 2023 Maris Nartiss
 *   SPDX-FileCopyrightText: Other GRASS authors
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *   Author: Maris Nartiss
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
