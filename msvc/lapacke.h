#ifndef GRASS_MSVC_LAPACKE_H
#define GRASS_MSVC_LAPACKE_H

#if defined(_MSC_VER)
#include <complex.h>
#define LAPACK_COMPLEX_CUSTOM
#define lapack_complex_float  _Fcomplex
#define lapack_complex_double _Dcomplex
#endif

#include <../openblas/lapacke.h>

#endif
