README for zufall random number package, C version
------ --- ------ ------ ------ -------  - -------

[NOTE: I have not done extension testing of this port.
       use at your own risk. The output of the original
       FORTRAN program is in zufall.orig.output. The
       original FORTRAN source is at 
       netlib.att.com:netlib/random/zufall.f.Z  --jdm]

This package contains a portable random number generator set
for: uniform (u in [0,1)), normal (<g> = 0, <g^2> = 1), and
Poisson distributions. The basic module, the uniform generator,
uses a lagged Fibonacci series generator:

              t    = u[n-273] + u[n-607]
              u[n] = t - (float) ((int) t)

where each number generated, u[k], is floating point. Since
the numbers are floating point, the left end boundary of the
range contains zero. This package was ported from FORTRAN
to K&R C.

To compile this beast, edit the Makefile and run 'make'

External documentation, "Lagged Fibonacci Random Number Generators
for the NEC SX-3," is to be published in the International
Journal of High Speed Computing (1993). Otherwise, ask the
original author: 

         W. P. Petersen 
         IPS, RZ F-5
         ETHZ
         CH 8092, Zurich
         Switzerland

e-mail:  wpp@ips.ethz.ch.

The port to C was done by Darrell McCauley <darrell@mccauley-usa.com>

The package contains the following routines:
------------------------------------------------------
UNIFORM generator routines:

  int zufalli(seed)    /* initializes common block containing seeds. */
    int seed;          /* if seed=0, the default value is 1802. */
  
  int zufall(n,u)      /* returns set of n uniforms u[0], ..., u[n-1]. */
    int n
    double *u;

  int zufallsv(zusave) /* saves buffer and pointer in zusave,  */
  double *zusave;      /* for later restarts. zusave must have */
                       /* at least 608 doubles allocated */

  int zufallrs(zusave) /* restores seed buffer and pointer  */
  double *zusave;      /* from zusave. zusave must have */
                       /* at least 608 doubles allocated */

------------------------------------------------------
NORMAL generator routines:

  int normalen(n,g)    /* returns set of n normals g(1), ..., g(n)  */
    int n;             /* such that mean <g> = 0, and variance <g**2> = 1. */
    double *g;

  int normalsv(normsv) /* saves zufall seed buffer and pointer in normsv */
    double *normsv;    /* buffer/pointer for normalen restart also in normsv */
                       /* normsv must at least 1634 doubles allocated */

  int normalrs(normsv) /* restores zufall seed buffer/pointer and */
    double *normsv;    /* buffer/pointer for normalen restart from normsv */
                       /* normsv must at least 1634 doubles allocated */
------------------------------------------------------
POISSON generator routine:

  int fische(n,mu,q)   /* assigns set of n integers q, with Poisson */
  int n, *q;           /* distribution, density p(q,mu) = exp(-mu) mu**q/q! */
  double mu;           /* Use zufallsv and zufallrs for stop/restart 
                       /* sequence */
