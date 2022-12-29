
/*****************************************************************************
*
* MODULE:       Grass numerical math interface
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:      ccmath library function wrapper
* 		part of the gmath library
*               
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/


#if defined(HAVE_CCMATH)
#include <ccmath.h>
#else
#include <grass/ccmath_grass.h>
#endif
/**
 * Documentation and ccmath library version 2.2.1 by Daniel A. Atkinson
 *
                                Chapter 1

                              LINEAR ALGEBRA

                                 Summary

               The matrix algebra library contains functions that
               perform the standard computations of linear algebra.
               General areas covered are:

                         o Solution of Linear Systems
                         o Matrix Inversion
                         o Eigensystem Analysis
                         o Matrix Utility Operations
                         o Singular Value Decomposition

               The operations covered here are fundamental to many
               areas of mathematics and statistics. Thus, functions
               in this library segment are called by other library
               functions. Both real and complex valued matrices
               are covered by functions in the first four of these
               categories.


 Notes on Contents

     Functions in this library segment provide the basic operations of
 numerical linear algebra and some useful utility functions for operations on
 vectors and matrices. The following list describes the functions available for
 operations with real-valued matrices.


 o  Solving and Inverting Linear Systems:

    solv  --------- solve a general system of real linear equations.
    solvps  ------- solve a real symmetric linear system.
    solvru  ------- solve a real right upper triangular linear system.
    solvtd  ------- solve a tridiagonal real linear system.

    minv  --------- invert a general real square matrix.
    psinv  -------- invert a real symmetric matrix.
    ruinv  -------- invert a right upper triangular matrix.


     The solution of a general linear system and efficient algorithms for
 solving special systems with symmetric and tridiagonal matrices are provided
 by these functions. The general solution function employs a LU factorization
 with partial pivoting and it is very robust. It will work efficiently on any
 problem that is not ill-conditioned. The symmetric matrix solution is based
 on a modified Cholesky factorization. It is best used on positive definite
 matrices that do not require pivoting for numeric stability. Tridiagonal
 solvers require order-N operations (N = dimension). Thus, they are highly
 recommended for this important class of sparse systems. Two matrix inversion
 routines are provided. The general inversion function is again LU based. It
 is suitable for use on any stable (ie. well-conditioned) problem. The
 Cholesky based symmetric matrix inversion is efficient and safe for use on
 matrices known to be positive definite, such as the variance matrices
 encountered in statistical computations. Both the solver and the inverse
 functions are designed to enhance data locality. They are very effective
 on modern microprocessors.


 o  Eigensystem Analysis:

    eigen  ------ extract all eigen values and vectors of a real
                  symmetric matrix.
    eigval  ----- extract the eigen values of a real symmetric matrix.
    evmax  ------ compute the eigen value of maximum absolute magnitude
                  and its corresponding vector for a symmetric matrix.


     Eigensystem functions operate on real symmetric matrices. Two forms of
 the general eigen routine are provided because the computation of eigen values
 only is much faster when vectors are not required. The basic algorithms use
 a Householder reduction to tridiagonal form followed by QR iterations with
 shifts to enhance convergence. This has become the accepted standard for
 symmetric eigensystem computation. The evmax function uses an efficient
 iterative power method algorithm to extract the eigen value of maximum
 absolute size and the corresponding eigenvector.


 o Singular Value Decomposition:

    svdval  ----- compute the singular values of a m by n real matrix.
    sv2val  ----- compute the singular values of a real matrix
                  efficiently for m >> n.
    svduv  ------ compute the singular values and the transformation
                  matrices u and v for a real m by n matrix.
    sv2uv  ------ compute the singular values and transformation
                  matrices efficiently for m >> n.
    svdu1v  ----- compute the singular values and transformation
                  matrices u1 and v, where u1 overloads the input
                  with the first n column vectors of u.
    sv2u1v  ----- compute the singular values and the transformation
                  matrices u1 and v efficiently for m >> n.


     Singular value decomposition is extremely useful when dealing with linear
 systems that may be singular. Singular values with values near zero are flags
 of a potential rank deficiency in the system matrix. They can be used to
 identify the presence of an ill-conditioned problem and, in some cases, to
 deal with the potential instability. They are applied to the linear least
 squares problem in this library. Singular values also define some important
 matrix norm parameters such as the 2-norm and the condition value. A complete
 decomposition provides both singular values and an orthogonal decomposition of
 vector spaces related to the matrix identifying the range and null-space.
 Fortunately, a highly stable algorithm based on Householder reduction to
 bidiagonal form and QR rotations can be used to implement the decomposition.
 The library provides two forms with one more efficient when the dimensions
 satisfy m > (3/2)n.

 General Technical Comments

     Efficient computation with matrices on modern processors must be
 adapted to the storage scheme employed for matrix elements. The functions
 of this library segment do not employ the multidimensional array intrinsic
 of the C language. Access to elements employs the simple row-major scheme
 described here.

     Matrices are modeled by the library functions as arrays with elements
 stored in row order. Thus, the element in the jth row and kth column of
 the n by n matrix M, stored in the array mat[], is addressed by

           M[j,k] = mat[n*j+k]  , with   0 =< j,k <= n-1 .

 (Remember that C employs zero as the starting index.) The storage order has
 important implications for data locality.

     The algorithms employed here all have excellent numerical stability, and
 the default double precision arithmetic of C enhances this. Thus, any
 problems encountered in using the matrix algebra functions will almost
 certainly be due to an ill-conditioned matrix. (The Hilbert matrices,

                 H[i,j] = 1/(1+i+j)  for i,j < n

 form a good example of such ill-conditioned systems.) We remind the reader
 that the appropriate response to such ill-conditioning is to seek an
 alternative approach to the problem. The option of increasing precision has
 already been exploited. Modification of the linear algebra algorithm code is
 not normally effective in an ill-conditioned problem.

------------------------------------------------------------------------------
                      FUNCTION SYNOPSES
------------------------------------------------------------------------------

 Linear System Solutions:
-----------------------------------------------------------------------------
*/
/**
     \brief Solve a general linear system  A*x = b.

     \param  a = array containing system matrix A in row order (altered to L-U factored form by computation)
     \param  b = array containing system vector b at entry and solution vector x at exit
     \param  n = dimension of system
     \return 0 -> normal exit; -1 -> singular input
 */
int G_math_solv(double **a,double *b,int n)
{
    return solv(a[0],b, n);
}


/**
     \brief Solve a symmetric positive definite linear system S*x = b.

     \param  a = array containing system matrix S (altered to Cholesky upper right factor by computation)
     \param  b = array containing system vector b as input and solution vector x as output
     \param  n = dimension of system
     \return: 0 -> normal exit; -1 -> input matrix not positive definite
 */
 int G_math_solvps(double **a,double *b,int n)
{
    return solvps(a[0], b,n);
}


/**
     \brief Solve a tridiagonal linear system M*x = y.

     \param a = array containing m+1 diagonal elements of M
     \param  b = array of m elements below the main diagonal of M
     \param  c = array of m elements above the main diagonal
     \param  x = array containing the system vector y initially, and the solution vector at exit (m+1 elements)
     \param  m = dimension parameter ( M is (m+1)x(m+1) )

*/
void G_math_solvtd(double *a,double *b,double *c,double *x,int m)
{
    solvtd(a, b, c, x, m);
    return;
}


/*
     \brief Solve an upper right triangular linear system T*x = b.

     \param  a = pointer to array of upper right triangular matrix T
     \param  b = pointer to array of system vector The computation overloads this with the solution vector x.
     \param  n = dimension (dim(a)=n*n,dim(b)=n)
     \return value: f = status flag, with 0 -> normal exit, -1 -> system singular
*/
int G_math_solvru(double **a,double *b,int n)
{
    return solvru(a[0], b, n);
}


/**
     \brief Invert (in place) a general real matrix A -> Inv(A).

     \param  a = array containing the input matrix A. This is converted to the inverse matrix.
     \param  n = dimension of the system (i.e. A is n x n )
     \return: 0 -> normal exit, 1 -> singular input matrix
*/
int G_math_minv(double **a,int n)
{
    return minv(a[0], n);
}


/**
     \brief Invert (in place) a symmetric real matrix, V -> Inv(V).

     The input matrix V is symmetric (V[i,j] = V[j,i]).
     \param  a = array containing a symmetric input matrix. This is converted to the inverse matrix.
     \param  n = dimension of the system (dim(v)=n*n)
     \return: 0 -> normal exit 1 -> input matrix not positive definite
*/
int G_math_psinv(double **a,int n)
{
    return psinv( a[0], n);
}


/**
     \brief Invert an upper right triangular matrix T -> Inv(T).

     \param  a = pointer to array of upper right triangular matrix, This is replaced by the inverse matrix.
     \param  n = dimension (dim(a)=n*n)
     \return value: status flag, with 0 -> matrix inverted -1 -> matrix singular
*/
int G_math_ruinv(double **a,int n)
{
    return ruinv(a[0], n);
}


/*
-----------------------------------------------------------------------------

     Symmetric Eigensystem Analysis:
-----------------------------------------------------------------------------
*/
/**

     \brief Compute the eigenvalues of a real symmetric matrix A.

     \param  a = pointer to array of symmetric n by n input matrix A. The computation alters these values.
     \param  ev = pointer to array of the output eigenvalues
     \param  n = dimension parameter (dim(a)= n*n, dim(ev)= n)
*/
void G_math_eigval(double **a,double *ev,int n)
{
    eigval(a[0], ev, n);
    return;
}


/**
     \brief Compute the eigenvalues and eigenvectors of a real symmetric matrix A.

      The input and output matrices are related by

          A = E*D*E~ where D is the diagonal matrix of eigenvalues
          D[i,j] = ev[i] if i=j and 0 otherwise.

     The columns of E are the eigenvectors.

     \param  a = pointer to store for symmetric n by n input matrix A. The computation overloads this with an orthogonal matrix of eigenvectors E.
     \param  ev = pointer to the array of the output eigenvalues
     \param  n = dimension parameter (dim(a)= n*n, dim(ev)= n)
*/
void G_math_eigen(double **a,double *ev,int n)
{
    eigen(a[0], ev, n);
    return;
}


/*
     \brief Compute the maximum (absolute) eigenvalue and corresponding eigenvector of a real symmetric matrix A.


     \param  a = array containing symmetric input matrix A
     \param  u = array containing the n components of the eigenvector at exit (vector normalized to 1)
     \param  n = dimension of system
     \return: ev = eigenvalue of A with maximum absolute value HUGE -> convergence failure
*/
double G_math_evmax(double **a,double *u,int n)
{
    return evmax(a[0], u, n);
}


/* 
------------------------------------------------------------------------------

 Singular Value Decomposition:
------------------------------------------------------------------------------

     A number of versions of the Singular Value Decomposition (SVD)
     are implemented in the library. They support the efficient
     computation of this important factorization for a real m by n
     matrix A. The general form of the SVD is

          A = U*S*V~     with S = | D |
                                  | 0 |

     where U is an m by m orthogonal matrix, V is an n by n orthogonal matrix,
     D is the n by n diagonal matrix of singular value, and S is the singular
     m by n matrix produced by the transformation.

     The singular values computed by these functions provide important
     information on the rank of the matrix A, and on several matrix
     norms of A. The number of non-zero singular values d[i] in D
     equal to the rank of A. The two norm of A is

          ||A|| = max(d[i]) , and the condition number is

          k(A) = max(d[i])/min(d[i]) .

     The Frobenius norm of the matrix A is

          Fn(A) = Sum(i=0 to n-1) d[i]^2 .

     Singular values consistent with zero are easily recognized, since
     the decomposition algorithms have excellent numerical stability.
     The value of a 'zero' d[i] is no larger than a few times the
     computational rounding error e.
     
     The matrix U1 is formed from the first n orthonormal column vectors
     of U.  U1[i,j] = U[i,j] for i = 1 to m and j = 1 to n. A singular
     value decomposition of A can also be expressed in terms of the m by\
     n matrix U1, with

                       A = U1*D*V~ .

     SVD functions with three forms of output are provided. The first
     form computes only the singular values, while the second computes
     the singular values and the U and V orthogonal transformation
     matrices. The third form of output computes singular values, the
     V matrix, and saves space by overloading the input array with
     the U1 matrix.

     Two forms of decomposition algorithm are available for each of the
     three output types. One is computationally efficient when m ~ n.
     The second, distinguished by the prefix 'sv2' in the function name,
     employs a two stage Householder reduction to accelerate computation
     when m substantially exceeds n. Use of functions of the second form
     is recommended for m > 2n.

     Singular value output from each of the six SVD functions satisfies

          d[i] >= 0 for i = 0 to n-1.
-------------------------------------------------------------------------------
*/


/**
     \brief Compute the singular values of a real m by n matrix A.


     \param  d = pointer to double array of dimension n (output = singular values of A)
     \param  a = pointer to store of the m by n input matrix A (A is altered by the computation)
     \param  m = number of rows in A
     \param  n = number of columns in A (m>=n required)
     \return value: status flag with: 0 -> success -1 -> input error m < n

*/
int G_math_svdval(double *d,double **a,int m,int n)
{
    return svdval(d, a[0], m, n);
}


/**

     \brief Compute singular values when m >> n.

     \param  d = pointer to double array of dimension n (output = singular values of A)
     \param  a = pointer to store of the m by n input matrix A (A is altered by the computation)
     \param  m = number of rows in A
     \param  n = number of columns in A (m>=n required)
     \return value: status flag with: 0 -> success -1 -> input error m < n
*/
int G_math_sv2val(double *d,double **a,int m,int n)
{
    return sv2val(d, a[0], m, n);
}


/*
     \brief Compute the singular value transformation S = U~*A*V.
     
     \param  d = pointer to double array of dimension n (output = singular values of A)
     \param  a = pointer to store of the m by n input matrix A (A is altered by the computation)
     \param  u = pointer to store for m by m orthogonal matrix U
     \param  v = pointer to store for n by n orthogonal matrix V
     \param  m = number of rows in A
     \param  n = number of columns in A (m>=n required)
     \return value: status flag with: 0 -> success -1 -> input error m < n
*/
int G_math_svduv(double *d,double **a,double **u,int m,double **v,int n)
{
    return svduv(d, a[0], u[0], m, v[0], n);
}


/**
     \brief Compute the singular value transformation when m >> n.
     
     \param  d = pointer to double array of dimension n (output = singular values of A)
     \param  a = pointer to store of the m by n input matrix A (A is altered by the computation)
     \param  u = pointer to store for m by m orthogonal matrix U
     \param  v = pointer to store for n by n orthogonal matrix V
     \param  m = number of rows in A
     \param  n = number of columns in A (m>=n required)
     \return value: status flag with: 0 -> success -1 -> input error m < n
*/
int G_math_sv2uv(double *d,double **a,double **u,int m,double **v,int n)
{
    return sv2uv(d, a[0], u[0], m, v[0], n);
}


/**

     \brief Compute the singular value transformation with A overloaded by the partial U-matrix.
     
     \param  d = pointer to double array of dimension n
           (output = singular values of A)
     \param   a = pointer to store of the m by n input matrix A (At output a is overloaded by the matrix U1 whose n columns are orthogonal vectors equal to the first n columns of U.)
     \param   v = pointer to store for n by n orthogonal matrix V
     \param   m = number of rows in A
     \param   n = number of columns in A (m>=n required)
     \return value: status flag with: 0 -> success -1 -> input error m < n

*/
int G_math_svdu1v(double *d,double **a,int m,double **v,int n)
{
    return svdu1v(d, a[0], m, v[0], n);
}
