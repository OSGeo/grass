#ifndef BLAS_WRAP_H
#define BLAS_WRAP_H

extern int caxpy_(integer * n, complex * ca, complex * cx, integer * incx,
		  complex * cy, integer * incy);
extern int ccopy_(integer * n, complex * cx, integer * incx, complex * cy,
		  integer * incy);
extern C_f cdotc_(complex * ret_val, integer * n, complex * cx,
		  integer * incx, complex * cy, integer * incy);
extern C_f cdotu_(complex * ret_val, integer * n, complex * cx,
		  integer * incx, complex * cy, integer * incy);
extern int cgbmv_(char *trans, integer * m, integer * n, integer * kl,
		  integer * ku, complex * alpha, complex * a, integer * lda,
		  complex * x, integer * incx, complex * beta, complex * y,
		  integer * incy, ftnlen trans_len);
extern int cgemm_(char *transa, char *transb, integer * m, integer * n,
		  integer * k, complex * alpha, complex * a, integer * lda,
		  complex * b, integer * ldb, complex * beta, complex * c__,
		  integer * ldc, ftnlen transa_len, ftnlen transb_len);
extern int cgemv_(char *trans, integer * m, integer * n, complex * alpha,
		  complex * a, integer * lda, complex * x, integer * incx,
		  complex * beta, complex * y, integer * incy,
		  ftnlen trans_len);
extern int cgerc_(integer * m, integer * n, complex * alpha, complex * x,
		  integer * incx, complex * y, integer * incy, complex * a,
		  integer * lda);
extern int cgeru_(integer * m, integer * n, complex * alpha, complex * x,
		  integer * incx, complex * y, integer * incy, complex * a,
		  integer * lda);
extern int chbmv_(char *uplo, integer * n, integer * k, complex * alpha,
		  complex * a, integer * lda, complex * x, integer * incx,
		  complex * beta, complex * y, integer * incy,
		  ftnlen uplo_len);
extern int chemm_(char *side, char *uplo, integer * m, integer * n,
		  complex * alpha, complex * a, integer * lda, complex * b,
		  integer * ldb, complex * beta, complex * c__, integer * ldc,
		  ftnlen side_len, ftnlen uplo_len);
extern int chemv_(char *uplo, integer * n, complex * alpha, complex * a,
		  integer * lda, complex * x, integer * incx, complex * beta,
		  complex * y, integer * incy, ftnlen uplo_len);
extern int cher_(char *uplo, integer * n, real * alpha, complex * x,
		 integer * incx, complex * a, integer * lda, ftnlen uplo_len);
extern int cher2_(char *uplo, integer * n, complex * alpha, complex * x,
		  integer * incx, complex * y, integer * incy, complex * a,
		  integer * lda, ftnlen uplo_len);
extern int cher2k_(char *uplo, char *trans, integer * n, integer * k,
		   complex * alpha, complex * a, integer * lda, complex * b,
		   integer * ldb, real * beta, complex * c__, integer * ldc,
		   ftnlen uplo_len, ftnlen trans_len);
extern int cherk_(char *uplo, char *trans, integer * n, integer * k,
		  real * alpha, complex * a, integer * lda, real * beta,
		  complex * c__, integer * ldc, ftnlen uplo_len,
		  ftnlen trans_len);
extern int chpmv_(char *uplo, integer * n, complex * alpha, complex * ap,
		  complex * x, integer * incx, complex * beta, complex * y,
		  integer * incy, ftnlen uplo_len);
extern int chpr_(char *uplo, integer * n, real * alpha, complex * x,
		 integer * incx, complex * ap, ftnlen uplo_len);
extern int chpr2_(char *uplo, integer * n, complex * alpha, complex * x,
		  integer * incx, complex * y, integer * incy, complex * ap,
		  ftnlen uplo_len);
extern int crotg_(complex * ca, complex * cb, real * c__, complex * s);
extern int cscal_(integer * n, complex * ca, complex * cx, integer * incx);
extern int csrot_(integer * n, complex * cx, integer * incx, complex * cy,
		  integer * incy, real * c__, real * s);
extern int csscal_(integer * n, real * sa, complex * cx, integer * incx);
extern int cswap_(integer * n, complex * cx, integer * incx, complex * cy,
		  integer * incy);
extern int csymm_(char *side, char *uplo, integer * m, integer * n,
		  complex * alpha, complex * a, integer * lda, complex * b,
		  integer * ldb, complex * beta, complex * c__, integer * ldc,
		  ftnlen side_len, ftnlen uplo_len);
extern int csyr2k_(char *uplo, char *trans, integer * n, integer * k,
		   complex * alpha, complex * a, integer * lda, complex * b,
		   integer * ldb, complex * beta, complex * c__,
		   integer * ldc, ftnlen uplo_len, ftnlen trans_len);
extern int csyrk_(char *uplo, char *trans, integer * n, integer * k,
		  complex * alpha, complex * a, integer * lda, complex * beta,
		  complex * c__, integer * ldc, ftnlen uplo_len,
		  ftnlen trans_len);
extern int ctbmv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, complex * a, integer * lda, complex * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int ctbsv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, complex * a, integer * lda, complex * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int ctpmv_(char *uplo, char *trans, char *diag, integer * n,
		  complex * ap, complex * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int ctpsv_(char *uplo, char *trans, char *diag, integer * n,
		  complex * ap, complex * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int ctrmm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, complex * alpha, complex * a,
		  integer * lda, complex * b, integer * ldb, ftnlen side_len,
		  ftnlen uplo_len, ftnlen transa_len, ftnlen diag_len);
extern int ctrmv_(char *uplo, char *trans, char *diag, integer * n,
		  complex * a, integer * lda, complex * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ctrsm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, complex * alpha, complex * a,
		  integer * lda, complex * b, integer * ldb, ftnlen side_len,
		  ftnlen uplo_len, ftnlen transa_len, ftnlen diag_len);
extern int ctrsv_(char *uplo, char *trans, char *diag, integer * n,
		  complex * a, integer * lda, complex * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern doublereal dasum_(integer * n, doublereal * dx, integer * incx);
extern int daxpy_(integer * n, doublereal * da, doublereal * dx,
		  integer * incx, doublereal * dy, integer * incy);
extern doublereal dcabs1_(doublecomplex * z__);
extern int dcopy_(integer * n, doublereal * dx, integer * incx,
		  doublereal * dy, integer * incy);
extern doublereal ddot_(integer * n, doublereal * dx, integer * incx,
			doublereal * dy, integer * incy);
extern int dgbmv_(char *trans, integer * m, integer * n, integer * kl,
		  integer * ku, doublereal * alpha, doublereal * a,
		  integer * lda, doublereal * x, integer * incx,
		  doublereal * beta, doublereal * y, integer * incy,
		  ftnlen trans_len);
extern int dgemm_(char *transa, char *transb, integer * m, integer * n,
		  integer * k, doublereal * alpha, doublereal * a,
		  integer * lda, doublereal * b, integer * ldb,
		  doublereal * beta, doublereal * c__, integer * ldc);
extern int dgemv_(char *trans, integer * m, integer * n, doublereal * alpha,
		  doublereal * a, integer * lda, doublereal * x,
		  integer * incx, doublereal * beta, doublereal * y,
		  integer * incy, ftnlen trans_len);
extern int dger_(integer * m, integer * n, doublereal * alpha, doublereal * x,
		 integer * incx, doublereal * y, integer * incy,
		 doublereal * a, integer * lda);
extern doublereal dnrm2_(integer * n, doublereal * x, integer * incx);
extern int drot_(integer * n, doublereal * dx, integer * incx,
		 doublereal * dy, integer * incy, doublereal * c__,
		 doublereal * s);
extern int drotg_(doublereal * da, doublereal * db, doublereal * c__,
		  doublereal * s);
extern int drotm_(integer * n, doublereal * dx, integer * incx,
		  doublereal * dy, integer * incy, doublereal * dparam);
extern int drotmg_(doublereal * dd1, doublereal * dd2, doublereal * dx1,
		   doublereal * dy1, doublereal * dparam);
extern int dsbmv_(char *uplo, integer * n, integer * k, doublereal * alpha,
		  doublereal * a, integer * lda, doublereal * x,
		  integer * incx, doublereal * beta, doublereal * y,
		  integer * incy, ftnlen uplo_len);
extern int dscal_(integer * n, doublereal * da, doublereal * dx,
		  integer * incx);
extern doublereal dsdot_(integer * n, real * sx, integer * incx, real * sy,
			 integer * incy);
extern int dspmv_(char *uplo, integer * n, doublereal * alpha,
		  doublereal * ap, doublereal * x, integer * incx,
		  doublereal * beta, doublereal * y, integer * incy,
		  ftnlen uplo_len);
extern int dspr_(char *uplo, integer * n, doublereal * alpha, doublereal * x,
		 integer * incx, doublereal * ap, ftnlen uplo_len);
extern int dspr2_(char *uplo, integer * n, doublereal * alpha, doublereal * x,
		  integer * incx, doublereal * y, integer * incy,
		  doublereal * ap, ftnlen uplo_len);
extern int dswap_(integer * n, doublereal * dx, integer * incx,
		  doublereal * dy, integer * incy);
extern int dsymm_(char *side, char *uplo, integer * m, integer * n,
		  doublereal * alpha, doublereal * a, integer * lda,
		  doublereal * b, integer * ldb, doublereal * beta,
		  doublereal * c__, integer * ldc, ftnlen side_len,
		  ftnlen uplo_len);
extern int dsymv_(char *uplo, integer * n, doublereal * alpha, doublereal * a,
		  integer * lda, doublereal * x, integer * incx,
		  doublereal * beta, doublereal * y, integer * incy,
		  ftnlen uplo_len);
extern int dsyr_(char *uplo, integer * n, doublereal * alpha, doublereal * x,
		 integer * incx, doublereal * a, integer * lda,
		 ftnlen uplo_len);
extern int dsyr2_(char *uplo, integer * n, doublereal * alpha, doublereal * x,
		  integer * incx, doublereal * y, integer * incy,
		  doublereal * a, integer * lda, ftnlen uplo_len);
extern int dsyr2k_(char *uplo, char *trans, integer * n, integer * k,
		   doublereal * alpha, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, doublereal * beta,
		   doublereal * c__, integer * ldc, ftnlen uplo_len,
		   ftnlen trans_len);
extern int dsyrk_(char *uplo, char *trans, integer * n, integer * k,
		  doublereal * alpha, doublereal * a, integer * lda,
		  doublereal * beta, doublereal * c__, integer * ldc,
		  ftnlen uplo_len, ftnlen trans_len);
extern int dtbmv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, doublereal * a, integer * lda, doublereal * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int dtbsv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, doublereal * a, integer * lda, doublereal * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int dtpmv_(char *uplo, char *trans, char *diag, integer * n,
		  doublereal * ap, doublereal * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int dtpsv_(char *uplo, char *trans, char *diag, integer * n,
		  doublereal * ap, doublereal * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int dtrmm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, doublereal * alpha,
		  doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, ftnlen side_len, ftnlen uplo_len,
		  ftnlen transa_len, ftnlen diag_len);
extern int dtrmv_(char *uplo, char *trans, char *diag, integer * n,
		  doublereal * a, integer * lda, doublereal * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int dtrsm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, doublereal * alpha,
		  doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, ftnlen side_len, ftnlen uplo_len,
		  ftnlen transa_len, ftnlen diag_len);
extern int dtrsv_(char *uplo, char *trans, char *diag, integer * n,
		  doublereal * a, integer * lda, doublereal * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern doublereal dzasum_(integer * n, doublecomplex * zx, integer * incx);
extern doublereal dznrm2_(integer * n, doublecomplex * x, integer * incx);
extern integer icamax_(integer * n, complex * cx, integer * incx);
extern integer idamax_(integer * n, doublereal * dx, integer * incx);
extern integer isamax_(integer * n, real * sx, integer * incx);
extern integer izamax_(integer * n, doublecomplex * zx, integer * incx);
extern logical lsame_(char *ca, char *cb, ftnlen ca_len, ftnlen cb_len);
extern E_f sasum_(integer * n, real * sx, integer * incx);
extern int saxpy_(integer * n, real * sa, real * sx, integer * incx,
		  real * sy, integer * incy);
extern E_f scasum_(integer * n, complex * cx, integer * incx);
extern E_f scnrm2_(integer * n, complex * x, integer * incx);
extern int scopy_(integer * n, real * sx, integer * incx, real * sy,
		  integer * incy);
extern E_f sdot_(integer * n, real * sx, integer * incx, real * sy,
		 integer * incy);
extern E_f sdsdot_(integer * n, real * sb, real * sx, integer * incx,
		   real * sy, integer * incy);
extern int sgbmv_(char *trans, integer * m, integer * n, integer * kl,
		  integer * ku, real * alpha, real * a, integer * lda,
		  real * x, integer * incx, real * beta, real * y,
		  integer * incy, ftnlen trans_len);
extern int sgemm_(char *transa, char *transb, integer * m, integer * n,
		  integer * k, real * alpha, real * a, integer * lda,
		  real * b, integer * ldb, real * beta, real * c__,
		  integer * ldc, ftnlen transa_len, ftnlen transb_len);
extern int sgemv_(char *trans, integer * m, integer * n, real * alpha,
		  real * a, integer * lda, real * x, integer * incx,
		  real * beta, real * y, integer * incy, ftnlen trans_len);
extern int sger_(integer * m, integer * n, real * alpha, real * x,
		 integer * incx, real * y, integer * incy, real * a,
		 integer * lda);
extern E_f snrm2_(integer * n, real * x, integer * incx);
extern int srot_(integer * n, real * sx, integer * incx, real * sy,
		 integer * incy, real * c__, real * s);
extern int srotg_(real * sa, real * sb, real * c__, real * s);
extern int srotm_(integer * n, real * sx, integer * incx, real * sy,
		  integer * incy, real * sparam);
extern int srotmg_(real * sd1, real * sd2, real * sx1, real * sy1,
		   real * sparam);
extern int ssbmv_(char *uplo, integer * n, integer * k, real * alpha,
		  real * a, integer * lda, real * x, integer * incx,
		  real * beta, real * y, integer * incy, ftnlen uplo_len);
extern int sscal_(integer * n, real * sa, real * sx, integer * incx);
extern int sspmv_(char *uplo, integer * n, real * alpha, real * ap, real * x,
		  integer * incx, real * beta, real * y, integer * incy,
		  ftnlen uplo_len);
extern int sspr_(char *uplo, integer * n, real * alpha, real * x,
		 integer * incx, real * ap, ftnlen uplo_len);
extern int sspr2_(char *uplo, integer * n, real * alpha, real * x,
		  integer * incx, real * y, integer * incy, real * ap,
		  ftnlen uplo_len);
extern int sswap_(integer * n, real * sx, integer * incx, real * sy,
		  integer * incy);
extern int ssymm_(char *side, char *uplo, integer * m, integer * n,
		  real * alpha, real * a, integer * lda, real * b,
		  integer * ldb, real * beta, real * c__, integer * ldc,
		  ftnlen side_len, ftnlen uplo_len);
extern int ssymv_(char *uplo, integer * n, real * alpha, real * a,
		  integer * lda, real * x, integer * incx, real * beta,
		  real * y, integer * incy, ftnlen uplo_len);
extern int ssyr_(char *uplo, integer * n, real * alpha, real * x,
		 integer * incx, real * a, integer * lda, ftnlen uplo_len);
extern int ssyr2_(char *uplo, integer * n, real * alpha, real * x,
		  integer * incx, real * y, integer * incy, real * a,
		  integer * lda, ftnlen uplo_len);
extern int ssyr2k_(char *uplo, char *trans, integer * n, integer * k,
		   real * alpha, real * a, integer * lda, real * b,
		   integer * ldb, real * beta, real * c__, integer * ldc,
		   ftnlen uplo_len, ftnlen trans_len);
extern int ssyrk_(char *uplo, char *trans, integer * n, integer * k,
		  real * alpha, real * a, integer * lda, real * beta,
		  real * c__, integer * ldc, ftnlen uplo_len,
		  ftnlen trans_len);
extern int stbmv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, real * a, integer * lda, real * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int stbsv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, real * a, integer * lda, real * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int stpmv_(char *uplo, char *trans, char *diag, integer * n, real * ap,
		  real * x, integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int stpsv_(char *uplo, char *trans, char *diag, integer * n, real * ap,
		  real * x, integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int strmm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, real * alpha, real * a,
		  integer * lda, real * b, integer * ldb, ftnlen side_len,
		  ftnlen uplo_len, ftnlen transa_len, ftnlen diag_len);
extern int strmv_(char *uplo, char *trans, char *diag, integer * n, real * a,
		  integer * lda, real * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int strsm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, real * alpha, real * a,
		  integer * lda, real * b, integer * ldb, ftnlen side_len,
		  ftnlen uplo_len, ftnlen transa_len, ftnlen diag_len);
extern int strsv_(char *uplo, char *trans, char *diag, integer * n, real * a,
		  integer * lda, real * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int xerbla_(char *srname, integer * info, ftnlen srname_len);
extern int zaxpy_(integer * n, doublecomplex * za, doublecomplex * zx,
		  integer * incx, doublecomplex * zy, integer * incy);
extern int zcopy_(integer * n, doublecomplex * zx, integer * incx,
		  doublecomplex * zy, integer * incy);
extern Z_f zdotc_(doublecomplex * ret_val, integer * n, doublecomplex * zx,
		  integer * incx, doublecomplex * zy, integer * incy);
extern Z_f zdotu_(doublecomplex * ret_val, integer * n, doublecomplex * zx,
		  integer * incx, doublecomplex * zy, integer * incy);
extern int zdrot_(integer * n, doublecomplex * zx, integer * incx,
		  doublecomplex * zy, integer * incy, doublereal * c__,
		  doublereal * s);
extern int zdscal_(integer * n, doublereal * da, doublecomplex * zx,
		   integer * incx);
extern int zgbmv_(char *trans, integer * m, integer * n, integer * kl,
		  integer * ku, doublecomplex * alpha, doublecomplex * a,
		  integer * lda, doublecomplex * x, integer * incx,
		  doublecomplex * beta, doublecomplex * y, integer * incy,
		  ftnlen trans_len);
extern int zgemm_(char *transa, char *transb, integer * m, integer * n,
		  integer * k, doublecomplex * alpha, doublecomplex * a,
		  integer * lda, doublecomplex * b, integer * ldb,
		  doublecomplex * beta, doublecomplex * c__, integer * ldc,
		  ftnlen transa_len, ftnlen transb_len);
extern int zgemv_(char *trans, integer * m, integer * n,
		  doublecomplex * alpha, doublecomplex * a, integer * lda,
		  doublecomplex * x, integer * incx, doublecomplex * beta,
		  doublecomplex * y, integer * incy, ftnlen trans_len);
extern int zgerc_(integer * m, integer * n, doublecomplex * alpha,
		  doublecomplex * x, integer * incx, doublecomplex * y,
		  integer * incy, doublecomplex * a, integer * lda);
extern int zgeru_(integer * m, integer * n, doublecomplex * alpha,
		  doublecomplex * x, integer * incx, doublecomplex * y,
		  integer * incy, doublecomplex * a, integer * lda);
extern int zhbmv_(char *uplo, integer * n, integer * k, doublecomplex * alpha,
		  doublecomplex * a, integer * lda, doublecomplex * x,
		  integer * incx, doublecomplex * beta, doublecomplex * y,
		  integer * incy, ftnlen uplo_len);
extern int zhemm_(char *side, char *uplo, integer * m, integer * n,
		  doublecomplex * alpha, doublecomplex * a, integer * lda,
		  doublecomplex * b, integer * ldb, doublecomplex * beta,
		  doublecomplex * c__, integer * ldc, ftnlen side_len,
		  ftnlen uplo_len);
extern int zhemv_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * a, integer * lda, doublecomplex * x,
		  integer * incx, doublecomplex * beta, doublecomplex * y,
		  integer * incy, ftnlen uplo_len);
extern int zher_(char *uplo, integer * n, doublereal * alpha,
		 doublecomplex * x, integer * incx, doublecomplex * a,
		 integer * lda, ftnlen uplo_len);
extern int zher2_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * x, integer * incx, doublecomplex * y,
		  integer * incy, doublecomplex * a, integer * lda,
		  ftnlen uplo_len);
extern int zher2k_(char *uplo, char *trans, integer * n, integer * k,
		   doublecomplex * alpha, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublereal * beta,
		   doublecomplex * c__, integer * ldc, ftnlen uplo_len,
		   ftnlen trans_len);
extern int zherk_(char *uplo, char *trans, integer * n, integer * k,
		  doublereal * alpha, doublecomplex * a, integer * lda,
		  doublereal * beta, doublecomplex * c__, integer * ldc,
		  ftnlen uplo_len, ftnlen trans_len);
extern int zhpmv_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * ap, doublecomplex * x, integer * incx,
		  doublecomplex * beta, doublecomplex * y, integer * incy,
		  ftnlen uplo_len);
extern int zhpr_(char *uplo, integer * n, doublereal * alpha,
		 doublecomplex * x, integer * incx, doublecomplex * ap,
		 ftnlen uplo_len);
extern int zhpr2_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * x, integer * incx, doublecomplex * y,
		  integer * incy, doublecomplex * ap, ftnlen uplo_len);
extern int zrotg_(doublecomplex * ca, doublecomplex * cb, doublereal * c__,
		  doublecomplex * s);
extern int zscal_(integer * n, doublecomplex * za, doublecomplex * zx,
		  integer * incx);
extern int zswap_(integer * n, doublecomplex * zx, integer * incx,
		  doublecomplex * zy, integer * incy);
extern int zsymm_(char *side, char *uplo, integer * m, integer * n,
		  doublecomplex * alpha, doublecomplex * a, integer * lda,
		  doublecomplex * b, integer * ldb, doublecomplex * beta,
		  doublecomplex * c__, integer * ldc, ftnlen side_len,
		  ftnlen uplo_len);
extern int zsyr2k_(char *uplo, char *trans, integer * n, integer * k,
		   doublecomplex * alpha, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * beta,
		   doublecomplex * c__, integer * ldc, ftnlen uplo_len,
		   ftnlen trans_len);
extern int zsyrk_(char *uplo, char *trans, integer * n, integer * k,
		  doublecomplex * alpha, doublecomplex * a, integer * lda,
		  doublecomplex * beta, doublecomplex * c__, integer * ldc,
		  ftnlen uplo_len, ftnlen trans_len);
extern int ztbmv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, doublecomplex * a, integer * lda,
		  doublecomplex * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int ztbsv_(char *uplo, char *trans, char *diag, integer * n,
		  integer * k, doublecomplex * a, integer * lda,
		  doublecomplex * x, integer * incx, ftnlen uplo_len,
		  ftnlen trans_len, ftnlen diag_len);
extern int ztpmv_(char *uplo, char *trans, char *diag, integer * n,
		  doublecomplex * ap, doublecomplex * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ztpsv_(char *uplo, char *trans, char *diag, integer * n,
		  doublecomplex * ap, doublecomplex * x, integer * incx,
		  ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ztrmm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, doublecomplex * alpha,
		  doublecomplex * a, integer * lda, doublecomplex * b,
		  integer * ldb, ftnlen side_len, ftnlen uplo_len,
		  ftnlen transa_len, ftnlen diag_len);
extern int ztrmv_(char *uplo, char *trans, char *diag, integer * n,
		  doublecomplex * a, integer * lda, doublecomplex * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);
extern int ztrsm_(char *side, char *uplo, char *transa, char *diag,
		  integer * m, integer * n, doublecomplex * alpha,
		  doublecomplex * a, integer * lda, doublecomplex * b,
		  integer * ldb, ftnlen side_len, ftnlen uplo_len,
		  ftnlen transa_len, ftnlen diag_len);
extern int ztrsv_(char *uplo, char *trans, char *diag, integer * n,
		  doublecomplex * a, integer * lda, doublecomplex * x,
		  integer * incx, ftnlen uplo_len, ftnlen trans_len,
		  ftnlen diag_len);

#endif
