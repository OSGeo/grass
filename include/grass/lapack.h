#ifndef LAPACK_WRAP_
#define LAPACK_WRAP_

extern int cbdsqr_(char *uplo, integer * n, integer * ncvt, integer * nru,
		   integer * ncc, real * d__, real * e, complex * vt,
		   integer * ldvt, complex * u, integer * ldu, complex * c__,
		   integer * ldc, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int cgbbrd_(char *vect, integer * m, integer * n, integer * ncc,
		   integer * kl, integer * ku, complex * ab, integer * ldab,
		   real * d__, real * e, complex * q, integer * ldq,
		   complex * pt, integer * ldpt, complex * c__, integer * ldc,
		   complex * work, real * rwork, integer * info,
		   ftnlen vect_len);
extern int cgbcon_(char *norm, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, integer * ipiv, real * anorm,
		   real * rcond, complex * work, real * rwork, integer * info,
		   ftnlen norm_len);
extern int cgbequ_(integer * m, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, real * r__, real * c__,
		   real * rowcnd, real * colcnd, real * amax, integer * info);
extern int cgbrfs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, complex * ab, integer * ldab,
		   complex * afb, integer * ldafb, integer * ipiv,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen trans_len);
extern int cgbsv_(integer * n, integer * kl, integer * ku, integer * nrhs,
		  complex * ab, integer * ldab, integer * ipiv, complex * b,
		  integer * ldb, integer * info);
extern int cgbsvx_(char *fact, char *trans, integer * n, integer * kl,
		   integer * ku, integer * nrhs, complex * ab, integer * ldab,
		   complex * afb, integer * ldafb, integer * ipiv,
		   char *equed, real * r__, real * c__, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen fact_len, ftnlen trans_len,
		   ftnlen equed_len);
extern int cgbtf2_(integer * m, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int cgbtrf_(integer * m, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int cgbtrs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, complex * ab, integer * ldab,
		   integer * ipiv, complex * b, integer * ldb, integer * info,
		   ftnlen trans_len);
extern int cgebak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, real * scale, integer * m, complex * v,
		   integer * ldv, integer * info, ftnlen job_len,
		   ftnlen side_len);
extern int cgebal_(char *job, integer * n, complex * a, integer * lda,
		   integer * ilo, integer * ihi, real * scale, integer * info,
		   ftnlen job_len);
extern int cgebd2_(integer * m, integer * n, complex * a, integer * lda,
		   real * d__, real * e, complex * tauq, complex * taup,
		   complex * work, integer * info);
extern int cgebrd_(integer * m, integer * n, complex * a, integer * lda,
		   real * d__, real * e, complex * tauq, complex * taup,
		   complex * work, integer * lwork, integer * info);
extern int cgecon_(char *norm, integer * n, complex * a, integer * lda,
		   real * anorm, real * rcond, complex * work, real * rwork,
		   integer * info, ftnlen norm_len);
extern int cgeequ_(integer * m, integer * n, complex * a, integer * lda,
		   real * r__, real * c__, real * rowcnd, real * colcnd,
		   real * amax, integer * info);
extern int cgees_(char *jobvs, char *sort, L_fp select, integer * n,
		  complex * a, integer * lda, integer * sdim, complex * w,
		  complex * vs, integer * ldvs, complex * work,
		  integer * lwork, real * rwork, logical * bwork,
		  integer * info, ftnlen jobvs_len, ftnlen sort_len);
extern int cgeesx_(char *jobvs, char *sort, L_fp select, char *sense,
		   integer * n, complex * a, integer * lda, integer * sdim,
		   complex * w, complex * vs, integer * ldvs, real * rconde,
		   real * rcondv, complex * work, integer * lwork,
		   real * rwork, logical * bwork, integer * info,
		   ftnlen jobvs_len, ftnlen sort_len, ftnlen sense_len);
extern int cgeev_(char *jobvl, char *jobvr, integer * n, complex * a,
		  integer * lda, complex * w, complex * vl, integer * ldvl,
		  complex * vr, integer * ldvr, complex * work,
		  integer * lwork, real * rwork, integer * info,
		  ftnlen jobvl_len, ftnlen jobvr_len);
extern int cgeevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, complex * a, integer * lda, complex * w,
		   complex * vl, integer * ldvl, complex * vr, integer * ldvr,
		   integer * ilo, integer * ihi, real * scale, real * abnrm,
		   real * rconde, real * rcondv, complex * work,
		   integer * lwork, real * rwork, integer * info,
		   ftnlen balanc_len, ftnlen jobvl_len, ftnlen jobvr_len,
		   ftnlen sense_len);
extern int cgegs_(char *jobvsl, char *jobvsr, integer * n, complex * a,
		  integer * lda, complex * b, integer * ldb, complex * alpha,
		  complex * beta, complex * vsl, integer * ldvsl,
		  complex * vsr, integer * ldvsr, complex * work,
		  integer * lwork, real * rwork, integer * info,
		  ftnlen jobvsl_len, ftnlen jobvsr_len);
extern int cgegv_(char *jobvl, char *jobvr, integer * n, complex * a,
		  integer * lda, complex * b, integer * ldb, complex * alpha,
		  complex * beta, complex * vl, integer * ldvl, complex * vr,
		  integer * ldvr, complex * work, integer * lwork,
		  real * rwork, integer * info, ftnlen jobvl_len,
		  ftnlen jobvr_len);
extern int cgehd2_(integer * n, integer * ilo, integer * ihi, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * info);
extern int cgehrd_(integer * n, integer * ilo, integer * ihi, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cgelq2_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * info);
extern int cgelqf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info);
extern int cgels_(char *trans, integer * m, integer * n, integer * nrhs,
		  complex * a, integer * lda, complex * b, integer * ldb,
		  complex * work, integer * lwork, integer * info,
		  ftnlen trans_len);
extern int cgelsd_(integer * m, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * b, integer * ldb, real * s,
		   real * rcond, integer * rank, complex * work,
		   integer * lwork, real * rwork, integer * iwork,
		   integer * info);
extern int cgelss_(integer * m, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * b, integer * ldb, real * s,
		   real * rcond, integer * rank, complex * work,
		   integer * lwork, real * rwork, integer * info);
extern int cgelsx_(integer * m, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * b, integer * ldb, integer * jpvt,
		   real * rcond, integer * rank, complex * work, real * rwork,
		   integer * info);
extern int cgelsy_(integer * m, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * b, integer * ldb, integer * jpvt,
		   real * rcond, integer * rank, complex * work,
		   integer * lwork, real * rwork, integer * info);
extern int cgeql2_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * info);
extern int cgeqlf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info);
extern int cgeqp3_(integer * m, integer * n, complex * a, integer * lda,
		   integer * jpvt, complex * tau, complex * work,
		   integer * lwork, real * rwork, integer * info);
extern int cgeqpf_(integer * m, integer * n, complex * a, integer * lda,
		   integer * jpvt, complex * tau, complex * work,
		   real * rwork, integer * info);
extern int cgeqr2_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * info);
extern int cgeqrf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info);
extern int cgerfs_(char *trans, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen trans_len);
extern int cgerq2_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * info);
extern int cgerqf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info);
extern int cgesc2_(integer * n, complex * a, integer * lda, complex * rhs,
		   integer * ipiv, integer * jpiv, real * scale);
extern int cgesdd_(char *jobz, integer * m, integer * n, complex * a,
		   integer * lda, real * s, complex * u, integer * ldu,
		   complex * vt, integer * ldvt, complex * work,
		   integer * lwork, real * rwork, integer * iwork,
		   integer * info, ftnlen jobz_len);
extern int cgesv_(integer * n, integer * nrhs, complex * a, integer * lda,
		  integer * ipiv, complex * b, integer * ldb, integer * info);
extern int cgesvd_(char *jobu, char *jobvt, integer * m, integer * n,
		   complex * a, integer * lda, real * s, complex * u,
		   integer * ldu, complex * vt, integer * ldvt,
		   complex * work, integer * lwork, real * rwork,
		   integer * info, ftnlen jobu_len, ftnlen jobvt_len);
extern int cgesvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   complex * a, integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, char *equed, real * r__, real * c__,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * rcond, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen fact_len,
		   ftnlen trans_len, ftnlen equed_len);
extern int cgetc2_(integer * n, complex * a, integer * lda, integer * ipiv,
		   integer * jpiv, integer * info);
extern int cgetf2_(integer * m, integer * n, complex * a, integer * lda,
		   integer * ipiv, integer * info);
extern int cgetrf_(integer * m, integer * n, complex * a, integer * lda,
		   integer * ipiv, integer * info);
extern int cgetri_(integer * n, complex * a, integer * lda, integer * ipiv,
		   complex * work, integer * lwork, integer * info);
extern int cgetrs_(char *trans, integer * n, integer * nrhs, complex * a,
		   integer * lda, integer * ipiv, complex * b, integer * ldb,
		   integer * info, ftnlen trans_len);
extern int cggbak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, real * lscale, real * rscale, integer * m,
		   complex * v, integer * ldv, integer * info, ftnlen job_len,
		   ftnlen side_len);
extern int cggbal_(char *job, integer * n, complex * a, integer * lda,
		   complex * b, integer * ldb, integer * ilo, integer * ihi,
		   real * lscale, real * rscale, real * work, integer * info,
		   ftnlen job_len);
extern int cgges_(char *jobvsl, char *jobvsr, char *sort, L_fp selctg,
		  integer * n, complex * a, integer * lda, complex * b,
		  integer * ldb, integer * sdim, complex * alpha,
		  complex * beta, complex * vsl, integer * ldvsl,
		  complex * vsr, integer * ldvsr, complex * work,
		  integer * lwork, real * rwork, logical * bwork,
		  integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len,
		  ftnlen sort_len);
extern int cggesx_(char *jobvsl, char *jobvsr, char *sort, L_fp selctg,
		   char *sense, integer * n, complex * a, integer * lda,
		   complex * b, integer * ldb, integer * sdim,
		   complex * alpha, complex * beta, complex * vsl,
		   integer * ldvsl, complex * vsr, integer * ldvsr,
		   real * rconde, real * rcondv, complex * work,
		   integer * lwork, real * rwork, integer * iwork,
		   integer * liwork, logical * bwork, integer * info,
		   ftnlen jobvsl_len, ftnlen jobvsr_len, ftnlen sort_len,
		   ftnlen sense_len);
extern int cggev_(char *jobvl, char *jobvr, integer * n, complex * a,
		  integer * lda, complex * b, integer * ldb, complex * alpha,
		  complex * beta, complex * vl, integer * ldvl, complex * vr,
		  integer * ldvr, complex * work, integer * lwork,
		  real * rwork, integer * info, ftnlen jobvl_len,
		  ftnlen jobvr_len);
extern int cggevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, complex * a, integer * lda, complex * b,
		   integer * ldb, complex * alpha, complex * beta,
		   complex * vl, integer * ldvl, complex * vr, integer * ldvr,
		   integer * ilo, integer * ihi, real * lscale, real * rscale,
		   real * abnrm, real * bbnrm, real * rconde, real * rcondv,
		   complex * work, integer * lwork, real * rwork,
		   integer * iwork, logical * bwork, integer * info,
		   ftnlen balanc_len, ftnlen jobvl_len, ftnlen jobvr_len,
		   ftnlen sense_len);
extern int cggglm_(integer * n, integer * m, integer * p, complex * a,
		   integer * lda, complex * b, integer * ldb, complex * d__,
		   complex * x, complex * y, complex * work, integer * lwork,
		   integer * info);
extern int cgghrd_(char *compq, char *compz, integer * n, integer * ilo,
		   integer * ihi, complex * a, integer * lda, complex * b,
		   integer * ldb, complex * q, integer * ldq, complex * z__,
		   integer * ldz, integer * info, ftnlen compq_len,
		   ftnlen compz_len);
extern int cgglse_(integer * m, integer * n, integer * p, complex * a,
		   integer * lda, complex * b, integer * ldb, complex * c__,
		   complex * d__, complex * x, complex * work,
		   integer * lwork, integer * info);
extern int cggqrf_(integer * n, integer * m, integer * p, complex * a,
		   integer * lda, complex * taua, complex * b, integer * ldb,
		   complex * taub, complex * work, integer * lwork,
		   integer * info);
extern int cggrqf_(integer * m, integer * p, integer * n, complex * a,
		   integer * lda, complex * taua, complex * b, integer * ldb,
		   complex * taub, complex * work, integer * lwork,
		   integer * info);
extern int cggsvd_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * n, integer * p, integer * k, integer * l,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   real * alpha, real * beta, complex * u, integer * ldu,
		   complex * v, integer * ldv, complex * q, integer * ldq,
		   complex * work, real * rwork, integer * iwork,
		   integer * info, ftnlen jobu_len, ftnlen jobv_len,
		   ftnlen jobq_len);
extern int cggsvp_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, complex * a, integer * lda,
		   complex * b, integer * ldb, real * tola, real * tolb,
		   integer * k, integer * l, complex * u, integer * ldu,
		   complex * v, integer * ldv, complex * q, integer * ldq,
		   integer * iwork, real * rwork, complex * tau,
		   complex * work, integer * info, ftnlen jobu_len,
		   ftnlen jobv_len, ftnlen jobq_len);
extern int cgtcon_(char *norm, integer * n, complex * dl, complex * d__,
		   complex * du, complex * du2, integer * ipiv, real * anorm,
		   real * rcond, complex * work, integer * info,
		   ftnlen norm_len);
extern int cgtrfs_(char *trans, integer * n, integer * nrhs, complex * dl,
		   complex * d__, complex * du, complex * dlf, complex * df,
		   complex * duf, complex * du2, integer * ipiv, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * ferr,
		   real * berr, complex * work, real * rwork, integer * info,
		   ftnlen trans_len);
extern int cgtsv_(integer * n, integer * nrhs, complex * dl, complex * d__,
		  complex * du, complex * b, integer * ldb, integer * info);
extern int cgtsvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   complex * dl, complex * d__, complex * du, complex * dlf,
		   complex * df, complex * duf, complex * du2, integer * ipiv,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * rcond, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen fact_len,
		   ftnlen trans_len);
extern int cgttrf_(integer * n, complex * dl, complex * d__, complex * du,
		   complex * du2, integer * ipiv, integer * info);
extern int cgttrs_(char *trans, integer * n, integer * nrhs, complex * dl,
		   complex * d__, complex * du, complex * du2, integer * ipiv,
		   complex * b, integer * ldb, integer * info,
		   ftnlen trans_len);
extern int cgtts2_(integer * itrans, integer * n, integer * nrhs,
		   complex * dl, complex * d__, complex * du, complex * du2,
		   integer * ipiv, complex * b, integer * ldb);
extern int chbev_(char *jobz, char *uplo, integer * n, integer * kd,
		  complex * ab, integer * ldab, real * w, complex * z__,
		  integer * ldz, complex * work, real * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int chbevd_(char *jobz, char *uplo, integer * n, integer * kd,
		   complex * ab, integer * ldab, real * w, complex * z__,
		   integer * ldz, complex * work, integer * lwork,
		   real * rwork, integer * lrwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int chbevx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * kd, complex * ab, integer * ldab, complex * q,
		   integer * ldq, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   complex * z__, integer * ldz, complex * work, real * rwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int chbgst_(char *vect, char *uplo, integer * n, integer * ka,
		   integer * kb, complex * ab, integer * ldab, complex * bb,
		   integer * ldbb, complex * x, integer * ldx, complex * work,
		   real * rwork, integer * info, ftnlen vect_len,
		   ftnlen uplo_len);
extern int chbgv_(char *jobz, char *uplo, integer * n, integer * ka,
		  integer * kb, complex * ab, integer * ldab, complex * bb,
		  integer * ldbb, real * w, complex * z__, integer * ldz,
		  complex * work, real * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int chbgvd_(char *jobz, char *uplo, integer * n, integer * ka,
		   integer * kb, complex * ab, integer * ldab, complex * bb,
		   integer * ldbb, real * w, complex * z__, integer * ldz,
		   complex * work, integer * lwork, real * rwork,
		   integer * lrwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int chbgvx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * ka, integer * kb, complex * ab, integer * ldab,
		   complex * bb, integer * ldbb, complex * q, integer * ldq,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, complex * z__,
		   integer * ldz, complex * work, real * rwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int chbtrd_(char *vect, char *uplo, integer * n, integer * kd,
		   complex * ab, integer * ldab, real * d__, real * e,
		   complex * q, integer * ldq, complex * work, integer * info,
		   ftnlen vect_len, ftnlen uplo_len);
extern int checon_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, real * anorm, real * rcond, complex * work,
		   integer * info, ftnlen uplo_len);
extern int cheev_(char *jobz, char *uplo, integer * n, complex * a,
		  integer * lda, real * w, complex * work, integer * lwork,
		  real * rwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int cheevd_(char *jobz, char *uplo, integer * n, complex * a,
		   integer * lda, real * w, complex * work, integer * lwork,
		   real * rwork, integer * lrwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int cheevr_(char *jobz, char *range, char *uplo, integer * n,
		   complex * a, integer * lda, real * vl, real * vu,
		   integer * il, integer * iu, real * abstol, integer * m,
		   real * w, complex * z__, integer * ldz, integer * isuppz,
		   complex * work, integer * lwork, real * rwork,
		   integer * lrwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int cheevx_(char *jobz, char *range, char *uplo, integer * n,
		   complex * a, integer * lda, real * vl, real * vu,
		   integer * il, integer * iu, real * abstol, integer * m,
		   real * w, complex * z__, integer * ldz, complex * work,
		   integer * lwork, real * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int chegs2_(integer * itype, char *uplo, integer * n, complex * a,
		   integer * lda, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int chegst_(integer * itype, char *uplo, integer * n, complex * a,
		   integer * lda, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int chegv_(integer * itype, char *jobz, char *uplo, integer * n,
		  complex * a, integer * lda, complex * b, integer * ldb,
		  real * w, complex * work, integer * lwork, real * rwork,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int chegvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   real * w, complex * work, integer * lwork, real * rwork,
		   integer * lrwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int chegvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, complex * a, integer * lda, complex * b,
		   integer * ldb, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   complex * z__, integer * ldz, complex * work,
		   integer * lwork, real * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int cherfs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen uplo_len);
extern int chesv_(char *uplo, integer * n, integer * nrhs, complex * a,
		  integer * lda, integer * ipiv, complex * b, integer * ldb,
		  complex * work, integer * lwork, integer * info,
		  ftnlen uplo_len);
extern int chesvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * a, integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * rcond, real * ferr, real * berr,
		   complex * work, integer * lwork, real * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int chetd2_(char *uplo, integer * n, complex * a, integer * lda,
		   real * d__, real * e, complex * tau, integer * info,
		   ftnlen uplo_len);
extern int chetf2_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int chetrd_(char *uplo, integer * n, complex * a, integer * lda,
		   real * d__, real * e, complex * tau, complex * work,
		   integer * lwork, integer * info, ftnlen uplo_len);
extern int chetrf_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, complex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int chetri_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, complex * work, integer * info,
		   ftnlen uplo_len);
extern int chetrs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, integer * ipiv, complex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int chgeqz_(char *job, char *compq, char *compz, integer * n,
		   integer * ilo, integer * ihi, complex * a, integer * lda,
		   complex * b, integer * ldb, complex * alpha,
		   complex * beta, complex * q, integer * ldq, complex * z__,
		   integer * ldz, complex * work, integer * lwork,
		   real * rwork, integer * info, ftnlen job_len,
		   ftnlen compq_len, ftnlen compz_len);
extern int chpcon_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   real * anorm, real * rcond, complex * work, integer * info,
		   ftnlen uplo_len);
extern int chpev_(char *jobz, char *uplo, integer * n, complex * ap, real * w,
		  complex * z__, integer * ldz, complex * work, real * rwork,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int chpevd_(char *jobz, char *uplo, integer * n, complex * ap,
		   real * w, complex * z__, integer * ldz, complex * work,
		   integer * lwork, real * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int chpevx_(char *jobz, char *range, char *uplo, integer * n,
		   complex * ap, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   complex * z__, integer * ldz, complex * work, real * rwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int chpgst_(integer * itype, char *uplo, integer * n, complex * ap,
		   complex * bp, integer * info, ftnlen uplo_len);
extern int chpgv_(integer * itype, char *jobz, char *uplo, integer * n,
		  complex * ap, complex * bp, real * w, complex * z__,
		  integer * ldz, complex * work, real * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int chpgvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   complex * ap, complex * bp, real * w, complex * z__,
		   integer * ldz, complex * work, integer * lwork,
		   real * rwork, integer * lrwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int chpgvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, complex * ap, complex * bp, real * vl,
		   real * vu, integer * il, integer * iu, real * abstol,
		   integer * m, real * w, complex * z__, integer * ldz,
		   complex * work, real * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int chprfs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   complex * afp, integer * ipiv, complex * b, integer * ldb,
		   complex * x, integer * ldx, real * ferr, real * berr,
		   complex * work, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int chpsv_(char *uplo, integer * n, integer * nrhs, complex * ap,
		  integer * ipiv, complex * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int chpsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * ap, complex * afp, integer * ipiv, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int chptrd_(char *uplo, integer * n, complex * ap, real * d__,
		   real * e, complex * tau, integer * info, ftnlen uplo_len);
extern int chptrf_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   integer * info, ftnlen uplo_len);
extern int chptri_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   complex * work, integer * info, ftnlen uplo_len);
extern int chptrs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   integer * ipiv, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int chsein_(char *side, char *eigsrc, char *initv, logical * select,
		   integer * n, complex * h__, integer * ldh, complex * w,
		   complex * vl, integer * ldvl, complex * vr, integer * ldvr,
		   integer * mm, integer * m, complex * work, real * rwork,
		   integer * ifaill, integer * ifailr, integer * info,
		   ftnlen side_len, ftnlen eigsrc_len, ftnlen initv_len);
extern int chseqr_(char *job, char *compz, integer * n, integer * ilo,
		   integer * ihi, complex * h__, integer * ldh, complex * w,
		   complex * z__, integer * ldz, complex * work,
		   integer * lwork, integer * info, ftnlen job_len,
		   ftnlen compz_len);
extern int clabrd_(integer * m, integer * n, integer * nb, complex * a,
		   integer * lda, real * d__, real * e, complex * tauq,
		   complex * taup, complex * x, integer * ldx, complex * y,
		   integer * ldy);
extern int clacgv_(integer * n, complex * x, integer * incx);
extern int clacon_(integer * n, complex * v, complex * x, real * est,
		   integer * kase);
extern int clacp2_(char *uplo, integer * m, integer * n, real * a,
		   integer * lda, complex * b, integer * ldb,
		   ftnlen uplo_len);
extern int clacpy_(char *uplo, integer * m, integer * n, complex * a,
		   integer * lda, complex * b, integer * ldb,
		   ftnlen uplo_len);
extern int clacrm_(integer * m, integer * n, complex * a, integer * lda,
		   real * b, integer * ldb, complex * c__, integer * ldc,
		   real * rwork);
extern int clacrt_(integer * n, complex * cx, integer * incx, complex * cy,
		   integer * incy, complex * c__, complex * s);
extern C_f cladiv_(complex * ret_val, complex * x, complex * y);
extern int claed0_(integer * qsiz, integer * n, real * d__, real * e,
		   complex * q, integer * ldq, complex * qstore,
		   integer * ldqs, real * rwork, integer * iwork,
		   integer * info);
extern int claed7_(integer * n, integer * cutpnt, integer * qsiz,
		   integer * tlvls, integer * curlvl, integer * curpbm,
		   real * d__, complex * q, integer * ldq, real * rho,
		   integer * indxq, real * qstore, integer * qptr,
		   integer * prmptr, integer * perm, integer * givptr,
		   integer * givcol, real * givnum, complex * work,
		   real * rwork, integer * iwork, integer * info);
extern int claed8_(integer * k, integer * n, integer * qsiz, complex * q,
		   integer * ldq, real * d__, real * rho, integer * cutpnt,
		   real * z__, real * dlamda, complex * q2, integer * ldq2,
		   real * w, integer * indxp, integer * indx, integer * indxq,
		   integer * perm, integer * givptr, integer * givcol,
		   real * givnum, integer * info);
extern int claein_(logical * rightv, logical * noinit, integer * n,
		   complex * h__, integer * ldh, complex * w, complex * v,
		   complex * b, integer * ldb, real * rwork, real * eps3,
		   real * smlnum, integer * info);
extern int claesy_(complex * a, complex * b, complex * c__, complex * rt1,
		   complex * rt2, complex * evscal, complex * cs1,
		   complex * sn1);
extern int claev2_(complex * a, complex * b, complex * c__, real * rt1,
		   real * rt2, real * cs1, complex * sn1);
extern int clags2_(logical * upper, real * a1, complex * a2, real * a3,
		   real * b1, complex * b2, real * b3, real * csu,
		   complex * snu, real * csv, complex * snv, real * csq,
		   complex * snq);
extern int clagtm_(char *trans, integer * n, integer * nrhs, real * alpha,
		   complex * dl, complex * d__, complex * du, complex * x,
		   integer * ldx, real * beta, complex * b, integer * ldb,
		   ftnlen trans_len);
extern int clahef_(char *uplo, integer * n, integer * nb, integer * kb,
		   complex * a, integer * lda, integer * ipiv, complex * w,
		   integer * ldw, integer * info, ftnlen uplo_len);
extern int clahqr_(logical * wantt, logical * wantz, integer * n,
		   integer * ilo, integer * ihi, complex * h__, integer * ldh,
		   complex * w, integer * iloz, integer * ihiz, complex * z__,
		   integer * ldz, integer * info);
extern int clahrd_(integer * n, integer * k, integer * nb, complex * a,
		   integer * lda, complex * tau, complex * t, integer * ldt,
		   complex * y, integer * ldy);
extern int claic1_(integer * job, integer * j, complex * x, real * sest,
		   complex * w, complex * gamma, real * sestpr, complex * s,
		   complex * c__);
extern int clals0_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * nrhs, complex * b, integer * ldb,
		   complex * bx, integer * ldbx, integer * perm,
		   integer * givptr, integer * givcol, integer * ldgcol,
		   real * givnum, integer * ldgnum, real * poles, real * difl,
		   real * difr, real * z__, integer * k, real * c__, real * s,
		   real * rwork, integer * info);
extern int clalsa_(integer * icompq, integer * smlsiz, integer * n,
		   integer * nrhs, complex * b, integer * ldb, complex * bx,
		   integer * ldbx, real * u, integer * ldu, real * vt,
		   integer * k, real * difl, real * difr, real * z__,
		   real * poles, integer * givptr, integer * givcol,
		   integer * ldgcol, integer * perm, real * givnum,
		   real * c__, real * s, real * rwork, integer * iwork,
		   integer * info);
extern int clalsd_(char *uplo, integer * smlsiz, integer * n, integer * nrhs,
		   real * d__, real * e, complex * b, integer * ldb,
		   real * rcond, integer * rank, complex * work, real * rwork,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern E_f clangb_(char *norm, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, real * work,
		   ftnlen norm_len);
extern E_f clange_(char *norm, integer * m, integer * n, complex * a,
		   integer * lda, real * work, ftnlen norm_len);
extern E_f clangt_(char *norm, integer * n, complex * dl, complex * d__,
		   complex * du, ftnlen norm_len);
extern E_f clanhb_(char *norm, char *uplo, integer * n, integer * k,
		   complex * ab, integer * ldab, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f clanhe_(char *norm, char *uplo, integer * n, complex * a,
		   integer * lda, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f clanhp_(char *norm, char *uplo, integer * n, complex * ap,
		   real * work, ftnlen norm_len, ftnlen uplo_len);
extern E_f clanhs_(char *norm, integer * n, complex * a, integer * lda,
		   real * work, ftnlen norm_len);
extern E_f clanht_(char *norm, integer * n, real * d__, complex * e,
		   ftnlen norm_len);
extern E_f clansb_(char *norm, char *uplo, integer * n, integer * k,
		   complex * ab, integer * ldab, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f clansp_(char *norm, char *uplo, integer * n, complex * ap,
		   real * work, ftnlen norm_len, ftnlen uplo_len);
extern E_f clansy_(char *norm, char *uplo, integer * n, complex * a,
		   integer * lda, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f clantb_(char *norm, char *uplo, char *diag, integer * n,
		   integer * k, complex * ab, integer * ldab, real * work,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern E_f clantp_(char *norm, char *uplo, char *diag, integer * n,
		   complex * ap, real * work, ftnlen norm_len,
		   ftnlen uplo_len, ftnlen diag_len);
extern E_f clantr_(char *norm, char *uplo, char *diag, integer * m,
		   integer * n, complex * a, integer * lda, real * work,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int clapll_(integer * n, complex * x, integer * incx, complex * y,
		   integer * incy, real * ssmin);
extern int clapmt_(logical * forwrd, integer * m, integer * n, complex * x,
		   integer * ldx, integer * k);
extern int claqgb_(integer * m, integer * n, integer * kl, integer * ku,
		   complex * ab, integer * ldab, real * r__, real * c__,
		   real * rowcnd, real * colcnd, real * amax, char *equed,
		   ftnlen equed_len);
extern int claqge_(integer * m, integer * n, complex * a, integer * lda,
		   real * r__, real * c__, real * rowcnd, real * colcnd,
		   real * amax, char *equed, ftnlen equed_len);
extern int claqhb_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, real * s, real * scond, real * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int claqhe_(char *uplo, integer * n, complex * a, integer * lda,
		   real * s, real * scond, real * amax, char *equed,
		   ftnlen uplo_len, ftnlen equed_len);
extern int claqhp_(char *uplo, integer * n, complex * ap, real * s,
		   real * scond, real * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int claqp2_(integer * m, integer * n, integer * offset, complex * a,
		   integer * lda, integer * jpvt, complex * tau, real * vn1,
		   real * vn2, complex * work);
extern int claqps_(integer * m, integer * n, integer * offset, integer * nb,
		   integer * kb, complex * a, integer * lda, integer * jpvt,
		   complex * tau, real * vn1, real * vn2, complex * auxv,
		   complex * f, integer * ldf);
extern int claqsb_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, real * s, real * scond, real * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int claqsp_(char *uplo, integer * n, complex * ap, real * s,
		   real * scond, real * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int claqsy_(char *uplo, integer * n, complex * a, integer * lda,
		   real * s, real * scond, real * amax, char *equed,
		   ftnlen uplo_len, ftnlen equed_len);
extern int clar1v_(integer * n, integer * b1, integer * bn, real * sigma,
		   real * d__, real * l, real * ld, real * lld, real * gersch,
		   complex * z__, real * ztz, real * mingma, integer * r__,
		   integer * isuppz, real * work);
extern int clar2v_(integer * n, complex * x, complex * y, complex * z__,
		   integer * incx, real * c__, complex * s, integer * incc);
extern int clarcm_(integer * m, integer * n, real * a, integer * lda,
		   complex * b, integer * ldb, complex * c__, integer * ldc,
		   real * rwork);
extern int clarf_(char *side, integer * m, integer * n, complex * v,
		  integer * incv, complex * tau, complex * c__, integer * ldc,
		  complex * work, ftnlen side_len);
extern int clarfb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, complex * v,
		   integer * ldv, complex * t, integer * ldt, complex * c__,
		   integer * ldc, complex * work, integer * ldwork,
		   ftnlen side_len, ftnlen trans_len, ftnlen direct_len,
		   ftnlen storev_len);
extern int clarfg_(integer * n, complex * alpha, complex * x, integer * incx,
		   complex * tau);
extern int clarft_(char *direct, char *storev, integer * n, integer * k,
		   complex * v, integer * ldv, complex * tau, complex * t,
		   integer * ldt, ftnlen direct_len, ftnlen storev_len);
extern int clarfx_(char *side, integer * m, integer * n, complex * v,
		   complex * tau, complex * c__, integer * ldc,
		   complex * work, ftnlen side_len);
extern int clargv_(integer * n, complex * x, integer * incx, complex * y,
		   integer * incy, real * c__, integer * incc);
extern int clarnv_(integer * idist, integer * iseed, integer * n,
		   complex * x);
extern int clarrv_(integer * n, real * d__, real * l, integer * isplit,
		   integer * m, real * w, integer * iblock, real * gersch,
		   real * tol, complex * z__, integer * ldz, integer * isuppz,
		   real * work, integer * iwork, integer * info);
extern int clartg_(complex * f, complex * g, real * cs, complex * sn,
		   complex * r__);
extern int clartv_(integer * n, complex * x, integer * incx, complex * y,
		   integer * incy, real * c__, complex * s, integer * incc);
extern int clarz_(char *side, integer * m, integer * n, integer * l,
		  complex * v, integer * incv, complex * tau, complex * c__,
		  integer * ldc, complex * work, ftnlen side_len);
extern int clarzb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, integer * l,
		   complex * v, integer * ldv, complex * t, integer * ldt,
		   complex * c__, integer * ldc, complex * work,
		   integer * ldwork, ftnlen side_len, ftnlen trans_len,
		   ftnlen direct_len, ftnlen storev_len);
extern int clarzt_(char *direct, char *storev, integer * n, integer * k,
		   complex * v, integer * ldv, complex * tau, complex * t,
		   integer * ldt, ftnlen direct_len, ftnlen storev_len);
extern int clascl_(char *type__, integer * kl, integer * ku, real * cfrom,
		   real * cto, integer * m, integer * n, complex * a,
		   integer * lda, integer * info, ftnlen type_len);
extern int claset_(char *uplo, integer * m, integer * n, complex * alpha,
		   complex * beta, complex * a, integer * lda,
		   ftnlen uplo_len);
extern int clasr_(char *side, char *pivot, char *direct, integer * m,
		  integer * n, real * c__, real * s, complex * a,
		  integer * lda, ftnlen side_len, ftnlen pivot_len,
		  ftnlen direct_len);
extern int classq_(integer * n, complex * x, integer * incx, real * scale,
		   real * sumsq);
extern int claswp_(integer * n, complex * a, integer * lda, integer * k1,
		   integer * k2, integer * ipiv, integer * incx);
extern int clasyf_(char *uplo, integer * n, integer * nb, integer * kb,
		   complex * a, integer * lda, integer * ipiv, complex * w,
		   integer * ldw, integer * info, ftnlen uplo_len);
extern int clatbs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, integer * kd, complex * ab, integer * ldab,
		   complex * x, real * scale, real * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int clatdf_(integer * ijob, integer * n, complex * z__, integer * ldz,
		   complex * rhs, real * rdsum, real * rdscal, integer * ipiv,
		   integer * jpiv);
extern int clatps_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, complex * ap, complex * x, real * scale,
		   real * cnorm, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len, ftnlen normin_len);
extern int clatrd_(char *uplo, integer * n, integer * nb, complex * a,
		   integer * lda, real * e, complex * tau, complex * w,
		   integer * ldw, ftnlen uplo_len);
extern int clatrs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, complex * a, integer * lda, complex * x,
		   real * scale, real * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int clatrz_(integer * m, integer * n, integer * l, complex * a,
		   integer * lda, complex * tau, complex * work);
extern int clatzm_(char *side, integer * m, integer * n, complex * v,
		   integer * incv, complex * tau, complex * c1, complex * c2,
		   integer * ldc, complex * work, ftnlen side_len);
extern int clauu2_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int clauum_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int cpbcon_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, real * anorm, real * rcond, complex * work,
		   real * rwork, integer * info, ftnlen uplo_len);
extern int cpbequ_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, real * s, real * scond, real * amax,
		   integer * info, ftnlen uplo_len);
extern int cpbrfs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   complex * ab, integer * ldab, complex * afb,
		   integer * ldafb, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen uplo_len);
extern int cpbstf_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int cpbsv_(char *uplo, integer * n, integer * kd, integer * nrhs,
		  complex * ab, integer * ldab, complex * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int cpbsvx_(char *fact, char *uplo, integer * n, integer * kd,
		   integer * nrhs, complex * ab, integer * ldab,
		   complex * afb, integer * ldafb, char *equed, real * s,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * rcond, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int cpbtf2_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int cpbtrf_(char *uplo, integer * n, integer * kd, complex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int cpbtrs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   complex * ab, integer * ldab, complex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int cpocon_(char *uplo, integer * n, complex * a, integer * lda,
		   real * anorm, real * rcond, complex * work, real * rwork,
		   integer * info, ftnlen uplo_len);
extern int cpoequ_(integer * n, complex * a, integer * lda, real * s,
		   real * scond, real * amax, integer * info);
extern int cporfs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * af, integer * ldaf, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * ferr,
		   real * berr, complex * work, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int cposv_(char *uplo, integer * n, integer * nrhs, complex * a,
		  integer * lda, complex * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int cposvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * a, integer * lda, complex * af, integer * ldaf,
		   char *equed, real * s, complex * b, integer * ldb,
		   complex * x, integer * ldx, real * rcond, real * ferr,
		   real * berr, complex * work, real * rwork, integer * info,
		   ftnlen fact_len, ftnlen uplo_len, ftnlen equed_len);
extern int cpotf2_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int cpotrf_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int cpotri_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int cpotrs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int cppcon_(char *uplo, integer * n, complex * ap, real * anorm,
		   real * rcond, complex * work, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int cppequ_(char *uplo, integer * n, complex * ap, real * s,
		   real * scond, real * amax, integer * info,
		   ftnlen uplo_len);
extern int cpprfs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   complex * afp, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen uplo_len);
extern int cppsv_(char *uplo, integer * n, integer * nrhs, complex * ap,
		  complex * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int cppsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * ap, complex * afp, char *equed, real * s,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * rcond, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int cpptrf_(char *uplo, integer * n, complex * ap, integer * info,
		   ftnlen uplo_len);
extern int cpptri_(char *uplo, integer * n, complex * ap, integer * info,
		   ftnlen uplo_len);
extern int cpptrs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int cptcon_(integer * n, real * d__, complex * e, real * anorm,
		   real * rcond, real * rwork, integer * info);
extern int cpteqr_(char *compz, integer * n, real * d__, real * e,
		   complex * z__, integer * ldz, real * work, integer * info,
		   ftnlen compz_len);
extern int cptrfs_(char *uplo, integer * n, integer * nrhs, real * d__,
		   complex * e, real * df, complex * ef, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * ferr,
		   real * berr, complex * work, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int cptsv_(integer * n, integer * nrhs, real * d__, complex * e,
		  complex * b, integer * ldb, integer * info);
extern int cptsvx_(char *fact, integer * n, integer * nrhs, real * d__,
		   complex * e, real * df, complex * ef, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen fact_len);
extern int cpttrf_(integer * n, real * d__, complex * e, integer * info);
extern int cpttrs_(char *uplo, integer * n, integer * nrhs, real * d__,
		   complex * e, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int cptts2_(integer * iuplo, integer * n, integer * nrhs, real * d__,
		   complex * e, complex * b, integer * ldb);
extern int crot_(integer * n, complex * cx, integer * incx, complex * cy,
		 integer * incy, real * c__, complex * s);
extern int cspcon_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   real * anorm, real * rcond, complex * work, integer * info,
		   ftnlen uplo_len);
extern int cspmv_(char *uplo, integer * n, complex * alpha, complex * ap,
		  complex * x, integer * incx, complex * beta, complex * y,
		  integer * incy, ftnlen uplo_len);
extern int cspr_(char *uplo, integer * n, complex * alpha, complex * x,
		 integer * incx, complex * ap, ftnlen uplo_len);
extern int csprfs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   complex * afp, integer * ipiv, complex * b, integer * ldb,
		   complex * x, integer * ldx, real * ferr, real * berr,
		   complex * work, real * rwork, integer * info,
		   ftnlen uplo_len);
extern int cspsv_(char *uplo, integer * n, integer * nrhs, complex * ap,
		  integer * ipiv, complex * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int cspsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * ap, complex * afp, integer * ipiv, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int csptrf_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   integer * info, ftnlen uplo_len);
extern int csptri_(char *uplo, integer * n, complex * ap, integer * ipiv,
		   complex * work, integer * info, ftnlen uplo_len);
extern int csptrs_(char *uplo, integer * n, integer * nrhs, complex * ap,
		   integer * ipiv, complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int csrot_(integer * n, complex * cx, integer * incx, complex * cy,
		  integer * incy, real * c__, real * s);
extern int csrscl_(integer * n, real * sa, complex * sx, integer * incx);
extern int cstedc_(char *compz, integer * n, real * d__, real * e,
		   complex * z__, integer * ldz, complex * work,
		   integer * lwork, real * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen compz_len);
extern int cstegr_(char *jobz, char *range, integer * n, real * d__, real * e,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, complex * z__,
		   integer * ldz, integer * isuppz, real * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int cstein_(integer * n, real * d__, real * e, integer * m, real * w,
		   integer * iblock, integer * isplit, complex * z__,
		   integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info);
extern int csteqr_(char *compz, integer * n, real * d__, real * e,
		   complex * z__, integer * ldz, real * work, integer * info,
		   ftnlen compz_len);
extern int csycon_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, real * anorm, real * rcond, complex * work,
		   integer * info, ftnlen uplo_len);
extern int csymv_(char *uplo, integer * n, complex * alpha, complex * a,
		  integer * lda, complex * x, integer * incx, complex * beta,
		  complex * y, integer * incy, ftnlen uplo_len);
extern int csyr_(char *uplo, integer * n, complex * alpha, complex * x,
		 integer * incx, complex * a, integer * lda, ftnlen uplo_len);
extern int csyrfs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * ferr, real * berr, complex * work,
		   real * rwork, integer * info, ftnlen uplo_len);
extern int csysv_(char *uplo, integer * n, integer * nrhs, complex * a,
		  integer * lda, integer * ipiv, complex * b, integer * ldb,
		  complex * work, integer * lwork, integer * info,
		  ftnlen uplo_len);
extern int csysvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   complex * a, integer * lda, complex * af, integer * ldaf,
		   integer * ipiv, complex * b, integer * ldb, complex * x,
		   integer * ldx, real * rcond, real * ferr, real * berr,
		   complex * work, integer * lwork, real * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int csytf2_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int csytrf_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, complex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int csytri_(char *uplo, integer * n, complex * a, integer * lda,
		   integer * ipiv, complex * work, integer * info,
		   ftnlen uplo_len);
extern int csytrs_(char *uplo, integer * n, integer * nrhs, complex * a,
		   integer * lda, integer * ipiv, complex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int ctbcon_(char *norm, char *uplo, char *diag, integer * n,
		   integer * kd, complex * ab, integer * ldab, real * rcond,
		   complex * work, real * rwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int ctbrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, complex * ab, integer * ldab,
		   complex * b, integer * ldb, complex * x, integer * ldx,
		   real * ferr, real * berr, complex * work, real * rwork,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int ctbtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, complex * ab, integer * ldab,
		   complex * b, integer * ldb, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ctgevc_(char *side, char *howmny, logical * select, integer * n,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   complex * vl, integer * ldvl, complex * vr, integer * ldvr,
		   integer * mm, integer * m, complex * work, real * rwork,
		   integer * info, ftnlen side_len, ftnlen howmny_len);
extern int ctgex2_(logical * wantq, logical * wantz, integer * n, complex * a,
		   integer * lda, complex * b, integer * ldb, complex * q,
		   integer * ldq, complex * z__, integer * ldz, integer * j1,
		   integer * info);
extern int ctgexc_(logical * wantq, logical * wantz, integer * n, complex * a,
		   integer * lda, complex * b, integer * ldb, complex * q,
		   integer * ldq, complex * z__, integer * ldz,
		   integer * ifst, integer * ilst, integer * info);
extern int ctgsen_(integer * ijob, logical * wantq, logical * wantz,
		   logical * select, integer * n, complex * a, integer * lda,
		   complex * b, integer * ldb, complex * alpha,
		   complex * beta, complex * q, integer * ldq, complex * z__,
		   integer * ldz, integer * m, real * pl, real * pr,
		   real * dif, complex * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info);
extern int ctgsja_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, integer * k, integer * l,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   real * tola, real * tolb, real * alpha, real * beta,
		   complex * u, integer * ldu, complex * v, integer * ldv,
		   complex * q, integer * ldq, complex * work,
		   integer * ncycle, integer * info, ftnlen jobu_len,
		   ftnlen jobv_len, ftnlen jobq_len);
extern int ctgsna_(char *job, char *howmny, logical * select, integer * n,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   complex * vl, integer * ldvl, complex * vr, integer * ldvr,
		   real * s, real * dif, integer * mm, integer * m,
		   complex * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen job_len, ftnlen howmny_len);
extern int ctgsy2_(char *trans, integer * ijob, integer * m, integer * n,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   complex * c__, integer * ldc, complex * d__, integer * ldd,
		   complex * e, integer * lde, complex * f, integer * ldf,
		   real * scale, real * rdsum, real * rdscal, integer * info,
		   ftnlen trans_len);
extern int ctgsyl_(char *trans, integer * ijob, integer * m, integer * n,
		   complex * a, integer * lda, complex * b, integer * ldb,
		   complex * c__, integer * ldc, complex * d__, integer * ldd,
		   complex * e, integer * lde, complex * f, integer * ldf,
		   real * scale, real * dif, complex * work, integer * lwork,
		   integer * iwork, integer * info, ftnlen trans_len);
extern int ctpcon_(char *norm, char *uplo, char *diag, integer * n,
		   complex * ap, real * rcond, complex * work, real * rwork,
		   integer * info, ftnlen norm_len, ftnlen uplo_len,
		   ftnlen diag_len);
extern int ctprfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, complex * ap, complex * b, integer * ldb,
		   complex * x, integer * ldx, real * ferr, real * berr,
		   complex * work, real * rwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ctptri_(char *uplo, char *diag, integer * n, complex * ap,
		   integer * info, ftnlen uplo_len, ftnlen diag_len);
extern int ctptrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, complex * ap, complex * b, integer * ldb,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int ctrcon_(char *norm, char *uplo, char *diag, integer * n,
		   complex * a, integer * lda, real * rcond, complex * work,
		   real * rwork, integer * info, ftnlen norm_len,
		   ftnlen uplo_len, ftnlen diag_len);
extern int ctrevc_(char *side, char *howmny, logical * select, integer * n,
		   complex * t, integer * ldt, complex * vl, integer * ldvl,
		   complex * vr, integer * ldvr, integer * mm, integer * m,
		   complex * work, real * rwork, integer * info,
		   ftnlen side_len, ftnlen howmny_len);
extern int ctrexc_(char *compq, integer * n, complex * t, integer * ldt,
		   complex * q, integer * ldq, integer * ifst, integer * ilst,
		   integer * info, ftnlen compq_len);
extern int ctrrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, complex * a, integer * lda, complex * b,
		   integer * ldb, complex * x, integer * ldx, real * ferr,
		   real * berr, complex * work, real * rwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ctrsen_(char *job, char *compq, logical * select, integer * n,
		   complex * t, integer * ldt, complex * q, integer * ldq,
		   complex * w, integer * m, real * s, real * sep,
		   complex * work, integer * lwork, integer * info,
		   ftnlen job_len, ftnlen compq_len);
extern int ctrsna_(char *job, char *howmny, logical * select, integer * n,
		   complex * t, integer * ldt, complex * vl, integer * ldvl,
		   complex * vr, integer * ldvr, real * s, real * sep,
		   integer * mm, integer * m, complex * work,
		   integer * ldwork, real * rwork, integer * info,
		   ftnlen job_len, ftnlen howmny_len);
extern int ctrsyl_(char *trana, char *tranb, integer * isgn, integer * m,
		   integer * n, complex * a, integer * lda, complex * b,
		   integer * ldb, complex * c__, integer * ldc, real * scale,
		   integer * info, ftnlen trana_len, ftnlen tranb_len);
extern int ctrti2_(char *uplo, char *diag, integer * n, complex * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int ctrtri_(char *uplo, char *diag, integer * n, complex * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int ctrtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, complex * a, integer * lda, complex * b,
		   integer * ldb, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int ctzrqf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, integer * info);
extern int ctzrzf_(integer * m, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info);
extern int cung2l_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * info);
extern int cung2r_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * info);
extern int cungbr_(char *vect, integer * m, integer * n, integer * k,
		   complex * a, integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info, ftnlen vect_len);
extern int cunghr_(integer * n, integer * ilo, integer * ihi, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cungl2_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * info);
extern int cunglq_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cungql_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cungqr_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cungr2_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * info);
extern int cungrq_(integer * m, integer * n, integer * k, complex * a,
		   integer * lda, complex * tau, complex * work,
		   integer * lwork, integer * info);
extern int cungtr_(char *uplo, integer * n, complex * a, integer * lda,
		   complex * tau, complex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int cunm2l_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int cunm2r_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int cunmbr_(char *vect, char *side, char *trans, integer * m,
		   integer * n, integer * k, complex * a, integer * lda,
		   complex * tau, complex * c__, integer * ldc,
		   complex * work, integer * lwork, integer * info,
		   ftnlen vect_len, ftnlen side_len, ftnlen trans_len);
extern int cunmhr_(char *side, char *trans, integer * m, integer * n,
		   integer * ilo, integer * ihi, complex * a, integer * lda,
		   complex * tau, complex * c__, integer * ldc,
		   complex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int cunml2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int cunmlq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int cunmql_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int cunmqr_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int cunmr2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int cunmr3_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, complex * a, integer * lda,
		   complex * tau, complex * c__, integer * ldc,
		   complex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int cunmrq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int cunmrz_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, complex * a, integer * lda,
		   complex * tau, complex * c__, integer * ldc,
		   complex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int cunmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, complex * a, integer * lda, complex * tau,
		   complex * c__, integer * ldc, complex * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen uplo_len, ftnlen trans_len);
extern int cupgtr_(char *uplo, integer * n, complex * ap, complex * tau,
		   complex * q, integer * ldq, complex * work, integer * info,
		   ftnlen uplo_len);
extern int cupmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, complex * ap, complex * tau, complex * c__,
		   integer * ldc, complex * work, integer * info,
		   ftnlen side_len, ftnlen uplo_len, ftnlen trans_len);
extern int dbdsdc_(char *uplo, char *compq, integer * n, doublereal * d__,
		   doublereal * e, doublereal * u, integer * ldu,
		   doublereal * vt, integer * ldvt, doublereal * q,
		   integer * iq, doublereal * work, integer * iwork,
		   integer * info, ftnlen uplo_len, ftnlen compq_len);
extern int dbdsqr_(char *uplo, integer * n, integer * ncvt, integer * nru,
		   integer * ncc, doublereal * d__, doublereal * e,
		   doublereal * vt, integer * ldvt, doublereal * u,
		   integer * ldu, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen uplo_len);
extern int ddisna_(char *job, integer * m, integer * n, doublereal * d__,
		   doublereal * sep, integer * info, ftnlen job_len);
extern int dgbbrd_(char *vect, integer * m, integer * n, integer * ncc,
		   integer * kl, integer * ku, doublereal * ab,
		   integer * ldab, doublereal * d__, doublereal * e,
		   doublereal * q, integer * ldq, doublereal * pt,
		   integer * ldpt, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen vect_len);
extern int dgbcon_(char *norm, integer * n, integer * kl, integer * ku,
		   doublereal * ab, integer * ldab, integer * ipiv,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen norm_len);
extern int dgbequ_(integer * m, integer * n, integer * kl, integer * ku,
		   doublereal * ab, integer * ldab, doublereal * r__,
		   doublereal * c__, doublereal * rowcnd, doublereal * colcnd,
		   doublereal * amax, integer * info);
extern int dgbrfs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, doublereal * ab, integer * ldab,
		   doublereal * afb, integer * ldafb, integer * ipiv,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen trans_len);
extern int dgbsv_(integer * n, integer * kl, integer * ku, integer * nrhs,
		  doublereal * ab, integer * ldab, integer * ipiv,
		  doublereal * b, integer * ldb, integer * info);
extern int dgbsvx_(char *fact, char *trans, integer * n, integer * kl,
		   integer * ku, integer * nrhs, doublereal * ab,
		   integer * ldab, doublereal * afb, integer * ldafb,
		   integer * ipiv, char *equed, doublereal * r__,
		   doublereal * c__, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublereal * work,
		   integer * iwork, integer * info, ftnlen fact_len,
		   ftnlen trans_len, ftnlen equed_len);
extern int dgbtf2_(integer * m, integer * n, integer * kl, integer * ku,
		   doublereal * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int dgbtrf_(integer * m, integer * n, integer * kl, integer * ku,
		   doublereal * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int dgbtrs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, doublereal * ab, integer * ldab,
		   integer * ipiv, doublereal * b, integer * ldb,
		   integer * info, ftnlen trans_len);
extern int dgebak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, doublereal * scale, integer * m,
		   doublereal * v, integer * ldv, integer * info,
		   ftnlen job_len, ftnlen side_len);
extern int dgebal_(char *job, integer * n, doublereal * a, integer * lda,
		   integer * ilo, integer * ihi, doublereal * scale,
		   integer * info, ftnlen job_len);
extern int dgebd2_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * d__, doublereal * e, doublereal * tauq,
		   doublereal * taup, doublereal * work, integer * info);
extern int dgebrd_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * d__, doublereal * e, doublereal * tauq,
		   doublereal * taup, doublereal * work, integer * lwork,
		   integer * info);
extern int dgecon_(char *norm, integer * n, doublereal * a, integer * lda,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen norm_len);
extern int dgeequ_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * r__, doublereal * c__, doublereal * rowcnd,
		   doublereal * colcnd, doublereal * amax, integer * info);
extern int dgees_(char *jobvs, char *sort, L_fp select, integer * n,
		  doublereal * a, integer * lda, integer * sdim,
		  doublereal * wr, doublereal * wi, doublereal * vs,
		  integer * ldvs, doublereal * work, integer * lwork,
		  logical * bwork, integer * info, ftnlen jobvs_len,
		  ftnlen sort_len);
extern int dgeesx_(char *jobvs, char *sort, L_fp select, char *sense,
		   integer * n, doublereal * a, integer * lda, integer * sdim,
		   doublereal * wr, doublereal * wi, doublereal * vs,
		   integer * ldvs, doublereal * rconde, doublereal * rcondv,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * liwork, logical * bwork, integer * info,
		   ftnlen jobvs_len, ftnlen sort_len, ftnlen sense_len);
extern int dgeev_(char *jobvl, char *jobvr, integer * n, doublereal * a,
		  integer * lda, doublereal * wr, doublereal * wi,
		  doublereal * vl, integer * ldvl, doublereal * vr,
		  integer * ldvr, doublereal * work, integer * lwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int dgeevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, doublereal * a, integer * lda,
		   doublereal * wr, doublereal * wi, doublereal * vl,
		   integer * ldvl, doublereal * vr, integer * ldvr,
		   integer * ilo, integer * ihi, doublereal * scale,
		   doublereal * abnrm, doublereal * rconde,
		   doublereal * rcondv, doublereal * work, integer * lwork,
		   integer * iwork, integer * info, ftnlen balanc_len,
		   ftnlen jobvl_len, ftnlen jobvr_len, ftnlen sense_len);
extern int dgegs_(char *jobvsl, char *jobvsr, integer * n, doublereal * a,
		  integer * lda, doublereal * b, integer * ldb,
		  doublereal * alphar, doublereal * alphai, doublereal * beta,
		  doublereal * vsl, integer * ldvsl, doublereal * vsr,
		  integer * ldvsr, doublereal * work, integer * lwork,
		  integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len);
extern int dgegv_(char *jobvl, char *jobvr, integer * n, doublereal * a,
		  integer * lda, doublereal * b, integer * ldb,
		  doublereal * alphar, doublereal * alphai, doublereal * beta,
		  doublereal * vl, integer * ldvl, doublereal * vr,
		  integer * ldvr, doublereal * work, integer * lwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int dgehd2_(integer * n, integer * ilo, integer * ihi, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * info);
extern int dgehrd_(integer * n, integer * ilo, integer * ihi, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dgelq2_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * info);
extern int dgelqf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info);
extern int dgels_(char *trans, integer * m, integer * n, integer * nrhs,
		  doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, doublereal * work, integer * lwork,
		  integer * info, ftnlen trans_len);
extern int dgelsd_(integer * m, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * s, doublereal * rcond, integer * rank,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * info);
extern int dgelss_(integer * m, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * s, doublereal * rcond, integer * rank,
		   doublereal * work, integer * lwork, integer * info);
extern int dgelsx_(integer * m, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   integer * jpvt, doublereal * rcond, integer * rank,
		   doublereal * work, integer * info);
extern int dgelsy_(integer * m, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   integer * jpvt, doublereal * rcond, integer * rank,
		   doublereal * work, integer * lwork, integer * info);
extern int dgeql2_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * info);
extern int dgeqlf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info);
extern int dgeqp3_(integer * m, integer * n, doublereal * a, integer * lda,
		   integer * jpvt, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dgeqpf_(integer * m, integer * n, doublereal * a, integer * lda,
		   integer * jpvt, doublereal * tau, doublereal * work,
		   integer * info);
extern int dgeqr2_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * info);
extern int dgeqrf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info);
extern int dgerfs_(char *trans, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * af, integer * ldaf,
		   integer * ipiv, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen trans_len);
extern int dgerq2_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * info);
extern int dgerqf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info);
extern int dgesc2_(integer * n, doublereal * a, integer * lda,
		   doublereal * rhs, integer * ipiv, integer * jpiv,
		   doublereal * scale);
extern int dgesdd_(char *jobz, integer * m, integer * n, doublereal * a,
		   integer * lda, doublereal * s, doublereal * u,
		   integer * ldu, doublereal * vt, integer * ldvt,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen jobz_len);
extern int dgesv_(integer * n, integer * nrhs, doublereal * a, integer * lda,
		  integer * ipiv, doublereal * b, integer * ldb,
		  integer * info);
extern int dgesvd_(char *jobu, char *jobvt, integer * m, integer * n,
		   doublereal * a, integer * lda, doublereal * s,
		   doublereal * u, integer * ldu, doublereal * vt,
		   integer * ldvt, doublereal * work, integer * lwork,
		   integer * info, ftnlen jobu_len, ftnlen jobvt_len);
extern int dgesvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   doublereal * a, integer * lda, doublereal * af,
		   integer * ldaf, integer * ipiv, char *equed,
		   doublereal * r__, doublereal * c__, doublereal * b,
		   integer * ldb, doublereal * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen fact_len, ftnlen trans_len, ftnlen equed_len);
extern int dgetc2_(integer * n, doublereal * a, integer * lda, integer * ipiv,
		   integer * jpiv, integer * info);
extern int dgetf2_(integer * m, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, integer * info);
extern int dgetrf_(integer * m, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, integer * info);
extern int dgetri_(integer * n, doublereal * a, integer * lda, integer * ipiv,
		   doublereal * work, integer * lwork, integer * info);
extern int dgetrs_(char *trans, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, integer * ipiv, doublereal * b,
		   integer * ldb, integer * info, ftnlen trans_len);
extern int dggbak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   integer * m, doublereal * v, integer * ldv, integer * info,
		   ftnlen job_len, ftnlen side_len);
extern int dggbal_(char *job, integer * n, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   doublereal * work, integer * info, ftnlen job_len);
extern int dgges_(char *jobvsl, char *jobvsr, char *sort, L_fp delctg,
		  integer * n, doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, integer * sdim, doublereal * alphar,
		  doublereal * alphai, doublereal * beta, doublereal * vsl,
		  integer * ldvsl, doublereal * vsr, integer * ldvsr,
		  doublereal * work, integer * lwork, logical * bwork,
		  integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len,
		  ftnlen sort_len);
extern int dggesx_(char *jobvsl, char *jobvsr, char *sort, L_fp delctg,
		   char *sense, integer * n, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, integer * sdim,
		   doublereal * alphar, doublereal * alphai,
		   doublereal * beta, doublereal * vsl, integer * ldvsl,
		   doublereal * vsr, integer * ldvsr, doublereal * rconde,
		   doublereal * rcondv, doublereal * work, integer * lwork,
		   integer * iwork, integer * liwork, logical * bwork,
		   integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len,
		   ftnlen sort_len, ftnlen sense_len);
extern int dggev_(char *jobvl, char *jobvr, integer * n, doublereal * a,
		  integer * lda, doublereal * b, integer * ldb,
		  doublereal * alphar, doublereal * alphai, doublereal * beta,
		  doublereal * vl, integer * ldvl, doublereal * vr,
		  integer * ldvr, doublereal * work, integer * lwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int dggevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * alphar, doublereal * alphai,
		   doublereal * beta, doublereal * vl, integer * ldvl,
		   doublereal * vr, integer * ldvr, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   doublereal * abnrm, doublereal * bbnrm,
		   doublereal * rconde, doublereal * rcondv,
		   doublereal * work, integer * lwork, integer * iwork,
		   logical * bwork, integer * info, ftnlen balanc_len,
		   ftnlen jobvl_len, ftnlen jobvr_len, ftnlen sense_len);
extern int dggglm_(integer * n, integer * m, integer * p, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * d__, doublereal * x, doublereal * y,
		   doublereal * work, integer * lwork, integer * info);
extern int dgghrd_(char *compq, char *compz, integer * n, integer * ilo,
		   integer * ihi, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, doublereal * q,
		   integer * ldq, doublereal * z__, integer * ldz,
		   integer * info, ftnlen compq_len, ftnlen compz_len);
extern int dgglse_(integer * m, integer * n, integer * p, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * c__, doublereal * d__, doublereal * x,
		   doublereal * work, integer * lwork, integer * info);
extern int dggqrf_(integer * n, integer * m, integer * p, doublereal * a,
		   integer * lda, doublereal * taua, doublereal * b,
		   integer * ldb, doublereal * taub, doublereal * work,
		   integer * lwork, integer * info);
extern int dggrqf_(integer * m, integer * p, integer * n, doublereal * a,
		   integer * lda, doublereal * taua, doublereal * b,
		   integer * ldb, doublereal * taub, doublereal * work,
		   integer * lwork, integer * info);
extern int dggsvd_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * n, integer * p, integer * k, integer * l,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * alpha, doublereal * beta,
		   doublereal * u, integer * ldu, doublereal * v,
		   integer * ldv, doublereal * q, integer * ldq,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen jobu_len, ftnlen jobv_len, ftnlen jobq_len);
extern int dggsvp_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, doublereal * tola,
		   doublereal * tolb, integer * k, integer * l,
		   doublereal * u, integer * ldu, doublereal * v,
		   integer * ldv, doublereal * q, integer * ldq,
		   integer * iwork, doublereal * tau, doublereal * work,
		   integer * info, ftnlen jobu_len, ftnlen jobv_len,
		   ftnlen jobq_len);
extern int dgtcon_(char *norm, integer * n, doublereal * dl, doublereal * d__,
		   doublereal * du, doublereal * du2, integer * ipiv,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen norm_len);
extern int dgtrfs_(char *trans, integer * n, integer * nrhs, doublereal * dl,
		   doublereal * d__, doublereal * du, doublereal * dlf,
		   doublereal * df, doublereal * duf, doublereal * du2,
		   integer * ipiv, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen trans_len);
extern int dgtsv_(integer * n, integer * nrhs, doublereal * dl,
		  doublereal * d__, doublereal * du, doublereal * b,
		  integer * ldb, integer * info);
extern int dgtsvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   doublereal * dl, doublereal * d__, doublereal * du,
		   doublereal * dlf, doublereal * df, doublereal * duf,
		   doublereal * du2, integer * ipiv, doublereal * b,
		   integer * ldb, doublereal * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen fact_len, ftnlen trans_len);
extern int dgttrf_(integer * n, doublereal * dl, doublereal * d__,
		   doublereal * du, doublereal * du2, integer * ipiv,
		   integer * info);
extern int dgttrs_(char *trans, integer * n, integer * nrhs, doublereal * dl,
		   doublereal * d__, doublereal * du, doublereal * du2,
		   integer * ipiv, doublereal * b, integer * ldb,
		   integer * info, ftnlen trans_len);
extern int dgtts2_(integer * itrans, integer * n, integer * nrhs,
		   doublereal * dl, doublereal * d__, doublereal * du,
		   doublereal * du2, integer * ipiv, doublereal * b,
		   integer * ldb);
extern int dhgeqz_(char *job, char *compq, char *compz, integer * n,
		   integer * ilo, integer * ihi, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * alphar, doublereal * alphai,
		   doublereal * beta, doublereal * q, integer * ldq,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * info, ftnlen job_len,
		   ftnlen compq_len, ftnlen compz_len);
extern int dhsein_(char *side, char *eigsrc, char *initv, logical * select,
		   integer * n, doublereal * h__, integer * ldh,
		   doublereal * wr, doublereal * wi, doublereal * vl,
		   integer * ldvl, doublereal * vr, integer * ldvr,
		   integer * mm, integer * m, doublereal * work,
		   integer * ifaill, integer * ifailr, integer * info,
		   ftnlen side_len, ftnlen eigsrc_len, ftnlen initv_len);
extern int dhseqr_(char *job, char *compz, integer * n, integer * ilo,
		   integer * ihi, doublereal * h__, integer * ldh,
		   doublereal * wr, doublereal * wi, doublereal * z__,
		   integer * ldz, doublereal * work, integer * lwork,
		   integer * info, ftnlen job_len, ftnlen compz_len);
extern int dlabad_(doublereal * small, doublereal * large);
extern int dlabrd_(integer * m, integer * n, integer * nb, doublereal * a,
		   integer * lda, doublereal * d__, doublereal * e,
		   doublereal * tauq, doublereal * taup, doublereal * x,
		   integer * ldx, doublereal * y, integer * ldy);
extern int dlacon_(integer * n, doublereal * v, doublereal * x,
		   integer * isgn, doublereal * est, integer * kase);
extern int dlacpy_(char *uplo, integer * m, integer * n, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   ftnlen uplo_len);
extern int dladiv_(doublereal * a, doublereal * b, doublereal * c__,
		   doublereal * d__, doublereal * p, doublereal * q);
extern int dlae2_(doublereal * a, doublereal * b, doublereal * c__,
		  doublereal * rt1, doublereal * rt2);
extern int dlaebz_(integer * ijob, integer * nitmax, integer * n,
		   integer * mmax, integer * minp, integer * nbmin,
		   doublereal * abstol, doublereal * reltol,
		   doublereal * pivmin, doublereal * d__, doublereal * e,
		   doublereal * e2, integer * nval, doublereal * ab,
		   doublereal * c__, integer * mout, integer * nab,
		   doublereal * work, integer * iwork, integer * info);
extern int dlaed0_(integer * icompq, integer * qsiz, integer * n,
		   doublereal * d__, doublereal * e, doublereal * q,
		   integer * ldq, doublereal * qstore, integer * ldqs,
		   doublereal * work, integer * iwork, integer * info);
extern int dlaed1_(integer * n, doublereal * d__, doublereal * q,
		   integer * ldq, integer * indxq, doublereal * rho,
		   integer * cutpnt, doublereal * work, integer * iwork,
		   integer * info);
extern int dlaed2_(integer * k, integer * n, integer * n1, doublereal * d__,
		   doublereal * q, integer * ldq, integer * indxq,
		   doublereal * rho, doublereal * z__, doublereal * dlamda,
		   doublereal * w, doublereal * q2, integer * indx,
		   integer * indxc, integer * indxp, integer * coltyp,
		   integer * info);
extern int dlaed3_(integer * k, integer * n, integer * n1, doublereal * d__,
		   doublereal * q, integer * ldq, doublereal * rho,
		   doublereal * dlamda, doublereal * q2, integer * indx,
		   integer * ctot, doublereal * w, doublereal * s,
		   integer * info);
extern int dlaed4_(integer * n, integer * i__, doublereal * d__,
		   doublereal * z__, doublereal * delta, doublereal * rho,
		   doublereal * dlam, integer * info);
extern int dlaed5_(integer * i__, doublereal * d__, doublereal * z__,
		   doublereal * delta, doublereal * rho, doublereal * dlam);
extern int dlaed6_(integer * kniter, logical * orgati, doublereal * rho,
		   doublereal * d__, doublereal * z__, doublereal * finit,
		   doublereal * tau, integer * info);
extern int dlaed7_(integer * icompq, integer * n, integer * qsiz,
		   integer * tlvls, integer * curlvl, integer * curpbm,
		   doublereal * d__, doublereal * q, integer * ldq,
		   integer * indxq, doublereal * rho, integer * cutpnt,
		   doublereal * qstore, integer * qptr, integer * prmptr,
		   integer * perm, integer * givptr, integer * givcol,
		   doublereal * givnum, doublereal * work, integer * iwork,
		   integer * info);
extern int dlaed8_(integer * icompq, integer * k, integer * n, integer * qsiz,
		   doublereal * d__, doublereal * q, integer * ldq,
		   integer * indxq, doublereal * rho, integer * cutpnt,
		   doublereal * z__, doublereal * dlamda, doublereal * q2,
		   integer * ldq2, doublereal * w, integer * perm,
		   integer * givptr, integer * givcol, doublereal * givnum,
		   integer * indxp, integer * indx, integer * info);
extern int dlaed9_(integer * k, integer * kstart, integer * kstop,
		   integer * n, doublereal * d__, doublereal * q,
		   integer * ldq, doublereal * rho, doublereal * dlamda,
		   doublereal * w, doublereal * s, integer * lds,
		   integer * info);
extern int dlaeda_(integer * n, integer * tlvls, integer * curlvl,
		   integer * curpbm, integer * prmptr, integer * perm,
		   integer * givptr, integer * givcol, doublereal * givnum,
		   doublereal * q, integer * qptr, doublereal * z__,
		   doublereal * ztemp, integer * info);
extern int dlaein_(logical * rightv, logical * noinit, integer * n,
		   doublereal * h__, integer * ldh, doublereal * wr,
		   doublereal * wi, doublereal * vr, doublereal * vi,
		   doublereal * b, integer * ldb, doublereal * work,
		   doublereal * eps3, doublereal * smlnum,
		   doublereal * bignum, integer * info);
extern int dlaev2_(doublereal * a, doublereal * b, doublereal * c__,
		   doublereal * rt1, doublereal * rt2, doublereal * cs1,
		   doublereal * sn1);
extern int dlaexc_(logical * wantq, integer * n, doublereal * t,
		   integer * ldt, doublereal * q, integer * ldq, integer * j1,
		   integer * n1, integer * n2, doublereal * work,
		   integer * info);
extern int dlag2_(doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, doublereal * safmin, doublereal * scale1,
		  doublereal * scale2, doublereal * wr1, doublereal * wr2,
		  doublereal * wi);
extern int dlags2_(logical * upper, doublereal * a1, doublereal * a2,
		   doublereal * a3, doublereal * b1, doublereal * b2,
		   doublereal * b3, doublereal * csu, doublereal * snu,
		   doublereal * csv, doublereal * snv, doublereal * csq,
		   doublereal * snq);
extern int dlagtf_(integer * n, doublereal * a, doublereal * lambda,
		   doublereal * b, doublereal * c__, doublereal * tol,
		   doublereal * d__, integer * in, integer * info);
extern int dlagtm_(char *trans, integer * n, integer * nrhs,
		   doublereal * alpha, doublereal * dl, doublereal * d__,
		   doublereal * du, doublereal * x, integer * ldx,
		   doublereal * beta, doublereal * b, integer * ldb,
		   ftnlen trans_len);
extern int dlagts_(integer * job, integer * n, doublereal * a, doublereal * b,
		   doublereal * c__, doublereal * d__, integer * in,
		   doublereal * y, doublereal * tol, integer * info);
extern int dlagv2_(doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * alphar, doublereal * alphai,
		   doublereal * beta, doublereal * csl, doublereal * snl,
		   doublereal * csr, doublereal * snr);
extern int dlahqr_(logical * wantt, logical * wantz, integer * n,
		   integer * ilo, integer * ihi, doublereal * h__,
		   integer * ldh, doublereal * wr, doublereal * wi,
		   integer * iloz, integer * ihiz, doublereal * z__,
		   integer * ldz, integer * info);
extern int dlahrd_(integer * n, integer * k, integer * nb, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * t,
		   integer * ldt, doublereal * y, integer * ldy);
extern int dlaic1_(integer * job, integer * j, doublereal * x,
		   doublereal * sest, doublereal * w, doublereal * gamma,
		   doublereal * sestpr, doublereal * s, doublereal * c__);
extern int dlaln2_(logical * ltrans, integer * na, integer * nw,
		   doublereal * smin, doublereal * ca, doublereal * a,
		   integer * lda, doublereal * d1, doublereal * d2,
		   doublereal * b, integer * ldb, doublereal * wr,
		   doublereal * wi, doublereal * x, integer * ldx,
		   doublereal * scale, doublereal * xnorm, integer * info);
extern int dlals0_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * nrhs, doublereal * b,
		   integer * ldb, doublereal * bx, integer * ldbx,
		   integer * perm, integer * givptr, integer * givcol,
		   integer * ldgcol, doublereal * givnum, integer * ldgnum,
		   doublereal * poles, doublereal * difl, doublereal * difr,
		   doublereal * z__, integer * k, doublereal * c__,
		   doublereal * s, doublereal * work, integer * info);
extern int dlalsa_(integer * icompq, integer * smlsiz, integer * n,
		   integer * nrhs, doublereal * b, integer * ldb,
		   doublereal * bx, integer * ldbx, doublereal * u,
		   integer * ldu, doublereal * vt, integer * k,
		   doublereal * difl, doublereal * difr, doublereal * z__,
		   doublereal * poles, integer * givptr, integer * givcol,
		   integer * ldgcol, integer * perm, doublereal * givnum,
		   doublereal * c__, doublereal * s, doublereal * work,
		   integer * iwork, integer * info);
extern int dlalsd_(char *uplo, integer * smlsiz, integer * n, integer * nrhs,
		   doublereal * d__, doublereal * e, doublereal * b,
		   integer * ldb, doublereal * rcond, integer * rank,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern doublereal dlamch_(char *cmach, ftnlen cmach_len);
extern int dlamc1_(integer * beta, integer * t, logical * rnd,
		   logical * ieee1);
extern int dlamc2_(integer * beta, integer * t, logical * rnd,
		   doublereal * eps, integer * emin, doublereal * rmin,
		   integer * emax, doublereal * rmax);
extern doublereal dlamc3_(doublereal * a, doublereal * b);
extern int dlamc4_(integer * emin, doublereal * start, integer * base);
extern int dlamc5_(integer * beta, integer * p, integer * emin,
		   logical * ieee, integer * emax, doublereal * rmax);
extern int dlamrg_(integer * n1, integer * n2, doublereal * a,
		   integer * dtrd1, integer * dtrd2, integer * index);
extern doublereal dlangb_(char *norm, integer * n, integer * kl, integer * ku,
			  doublereal * ab, integer * ldab, doublereal * work,
			  ftnlen norm_len);
extern doublereal dlange_(char *norm, integer * m, integer * n,
			  doublereal * a, integer * lda, doublereal * work,
			  ftnlen norm_len);
extern doublereal dlangt_(char *norm, integer * n, doublereal * dl,
			  doublereal * d__, doublereal * du, ftnlen norm_len);
extern doublereal dlanhs_(char *norm, integer * n, doublereal * a,
			  integer * lda, doublereal * work, ftnlen norm_len);
extern doublereal dlansb_(char *norm, char *uplo, integer * n, integer * k,
			  doublereal * ab, integer * ldab, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len);
extern doublereal dlansp_(char *norm, char *uplo, integer * n,
			  doublereal * ap, doublereal * work, ftnlen norm_len,
			  ftnlen uplo_len);
extern doublereal dlanst_(char *norm, integer * n, doublereal * d__,
			  doublereal * e, ftnlen norm_len);
extern doublereal dlansy_(char *norm, char *uplo, integer * n, doublereal * a,
			  integer * lda, doublereal * work, ftnlen norm_len,
			  ftnlen uplo_len);
extern doublereal dlantb_(char *norm, char *uplo, char *diag, integer * n,
			  integer * k, doublereal * ab, integer * ldab,
			  doublereal * work, ftnlen norm_len, ftnlen uplo_len,
			  ftnlen diag_len);
extern doublereal dlantp_(char *norm, char *uplo, char *diag, integer * n,
			  doublereal * ap, doublereal * work, ftnlen norm_len,
			  ftnlen uplo_len, ftnlen diag_len);
extern doublereal dlantr_(char *norm, char *uplo, char *diag, integer * m,
			  integer * n, doublereal * a, integer * lda,
			  doublereal * work, ftnlen norm_len, ftnlen uplo_len,
			  ftnlen diag_len);
extern int dlanv2_(doublereal * a, doublereal * b, doublereal * c__,
		   doublereal * d__, doublereal * rt1r, doublereal * rt1i,
		   doublereal * rt2r, doublereal * rt2i, doublereal * cs,
		   doublereal * sn);
extern int dlapll_(integer * n, doublereal * x, integer * incx,
		   doublereal * y, integer * incy, doublereal * ssmin);
extern int dlapmt_(logical * forwrd, integer * m, integer * n, doublereal * x,
		   integer * ldx, integer * k);
extern doublereal dlapy2_(doublereal * x, doublereal * y);
extern doublereal dlapy3_(doublereal * x, doublereal * y, doublereal * z__);
extern int dlaqgb_(integer * m, integer * n, integer * kl, integer * ku,
		   doublereal * ab, integer * ldab, doublereal * r__,
		   doublereal * c__, doublereal * rowcnd, doublereal * colcnd,
		   doublereal * amax, char *equed, ftnlen equed_len);
extern int dlaqge_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * r__, doublereal * c__, doublereal * rowcnd,
		   doublereal * colcnd, doublereal * amax, char *equed,
		   ftnlen equed_len);
extern int dlaqp2_(integer * m, integer * n, integer * offset, doublereal * a,
		   integer * lda, integer * jpvt, doublereal * tau,
		   doublereal * vn1, doublereal * vn2, doublereal * work);
extern int dlaqps_(integer * m, integer * n, integer * offset, integer * nb,
		   integer * kb, doublereal * a, integer * lda,
		   integer * jpvt, doublereal * tau, doublereal * vn1,
		   doublereal * vn2, doublereal * auxv, doublereal * f,
		   integer * ldf);
extern int dlaqsb_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, doublereal * s, doublereal * scond,
		   doublereal * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int dlaqsp_(char *uplo, integer * n, doublereal * ap, doublereal * s,
		   doublereal * scond, doublereal * amax, char *equed,
		   ftnlen uplo_len, ftnlen equed_len);
extern int dlaqsy_(char *uplo, integer * n, doublereal * a, integer * lda,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int dlaqtr_(logical * ltran, logical * lreal, integer * n,
		   doublereal * t, integer * ldt, doublereal * b,
		   doublereal * w, doublereal * scale, doublereal * x,
		   doublereal * work, integer * info);
extern int dlar1v_(integer * n, integer * b1, integer * bn,
		   doublereal * sigma, doublereal * d__, doublereal * l,
		   doublereal * ld, doublereal * lld, doublereal * gersch,
		   doublereal * z__, doublereal * ztz, doublereal * mingma,
		   integer * r__, integer * isuppz, doublereal * work);
extern int dlar2v_(integer * n, doublereal * x, doublereal * y,
		   doublereal * z__, integer * incx, doublereal * c__,
		   doublereal * s, integer * incc);
extern int dlarf_(char *side, integer * m, integer * n, doublereal * v,
		  integer * incv, doublereal * tau, doublereal * c__,
		  integer * ldc, doublereal * work, ftnlen side_len);
extern int dlarfb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, doublereal * v,
		   integer * ldv, doublereal * t, integer * ldt,
		   doublereal * c__, integer * ldc, doublereal * work,
		   integer * ldwork, ftnlen side_len, ftnlen trans_len,
		   ftnlen direct_len, ftnlen storev_len);
extern int dlarfg_(integer * n, doublereal * alpha, doublereal * x,
		   integer * incx, doublereal * tau);
extern int dlarft_(char *direct, char *storev, integer * n, integer * k,
		   doublereal * v, integer * ldv, doublereal * tau,
		   doublereal * t, integer * ldt, ftnlen direct_len,
		   ftnlen storev_len);
extern int dlarfx_(char *side, integer * m, integer * n, doublereal * v,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, ftnlen side_len);
extern int dlargv_(integer * n, doublereal * x, integer * incx,
		   doublereal * y, integer * incy, doublereal * c__,
		   integer * incc);
extern int dlarnv_(integer * idist, integer * iseed, integer * n,
		   doublereal * x);
extern int dlarrb_(integer * n, doublereal * d__, doublereal * l,
		   doublereal * ld, doublereal * lld, integer * ifirst,
		   integer * ilast, doublereal * sigma, doublereal * reltol,
		   doublereal * w, doublereal * wgap, doublereal * werr,
		   doublereal * work, integer * iwork, integer * info);
extern int dlarre_(integer * n, doublereal * d__, doublereal * e,
		   doublereal * tol, integer * nsplit, integer * isplit,
		   integer * m, doublereal * w, doublereal * woff,
		   doublereal * gersch, doublereal * work, integer * info);
extern int dlarrf_(integer * n, doublereal * d__, doublereal * l,
		   doublereal * ld, doublereal * lld, integer * ifirst,
		   integer * ilast, doublereal * w, doublereal * dplus,
		   doublereal * lplus, doublereal * work, integer * iwork,
		   integer * info);
extern int dlarrv_(integer * n, doublereal * d__, doublereal * l,
		   integer * isplit, integer * m, doublereal * w,
		   integer * iblock, doublereal * gersch, doublereal * tol,
		   doublereal * z__, integer * ldz, integer * isuppz,
		   doublereal * work, integer * iwork, integer * info);
extern int dlartg_(doublereal * f, doublereal * g, doublereal * cs,
		   doublereal * sn, doublereal * r__);
extern int dlartv_(integer * n, doublereal * x, integer * incx,
		   doublereal * y, integer * incy, doublereal * c__,
		   doublereal * s, integer * incc);
extern int dlaruv_(integer * iseed, integer * n, doublereal * x);
extern int dlarz_(char *side, integer * m, integer * n, integer * l,
		  doublereal * v, integer * incv, doublereal * tau,
		  doublereal * c__, integer * ldc, doublereal * work,
		  ftnlen side_len);
extern int dlarzb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, integer * l,
		   doublereal * v, integer * ldv, doublereal * t,
		   integer * ldt, doublereal * c__, integer * ldc,
		   doublereal * work, integer * ldwork, ftnlen side_len,
		   ftnlen trans_len, ftnlen direct_len, ftnlen storev_len);
extern int dlarzt_(char *direct, char *storev, integer * n, integer * k,
		   doublereal * v, integer * ldv, doublereal * tau,
		   doublereal * t, integer * ldt, ftnlen direct_len,
		   ftnlen storev_len);
extern int dlas2_(doublereal * f, doublereal * g, doublereal * h__,
		  doublereal * ssmin, doublereal * ssmax);
extern int dlascl_(char *type__, integer * kl, integer * ku,
		   doublereal * cfrom, doublereal * cto, integer * m,
		   integer * n, doublereal * a, integer * lda, integer * info,
		   ftnlen type_len);
extern int dlasd0_(integer * n, integer * sqre, doublereal * d__,
		   doublereal * e, doublereal * u, integer * ldu,
		   doublereal * vt, integer * ldvt, integer * smlsiz,
		   integer * iwork, doublereal * work, integer * info);
extern int dlasd1_(integer * nl, integer * nr, integer * sqre,
		   doublereal * d__, doublereal * alpha, doublereal * beta,
		   doublereal * u, integer * ldu, doublereal * vt,
		   integer * ldvt, integer * idxq, integer * iwork,
		   doublereal * work, integer * info);
extern int dlasd2_(integer * nl, integer * nr, integer * sqre, integer * k,
		   doublereal * d__, doublereal * z__, doublereal * alpha,
		   doublereal * beta, doublereal * u, integer * ldu,
		   doublereal * vt, integer * ldvt, doublereal * dsigma,
		   doublereal * u2, integer * ldu2, doublereal * vt2,
		   integer * ldvt2, integer * idxp, integer * idx,
		   integer * idxc, integer * idxq, integer * coltyp,
		   integer * info);
extern int dlasd3_(integer * nl, integer * nr, integer * sqre, integer * k,
		   doublereal * d__, doublereal * q, integer * ldq,
		   doublereal * dsigma, doublereal * u, integer * ldu,
		   doublereal * u2, integer * ldu2, doublereal * vt,
		   integer * ldvt, doublereal * vt2, integer * ldvt2,
		   integer * idxc, integer * ctot, doublereal * z__,
		   integer * info);
extern int dlasd4_(integer * n, integer * i__, doublereal * d__,
		   doublereal * z__, doublereal * delta, doublereal * rho,
		   doublereal * sigma, doublereal * work, integer * info);
extern int dlasd5_(integer * i__, doublereal * d__, doublereal * z__,
		   doublereal * delta, doublereal * rho, doublereal * dsigma,
		   doublereal * work);
extern int dlasd6_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, doublereal * d__, doublereal * vf,
		   doublereal * vl, doublereal * alpha, doublereal * beta,
		   integer * idxq, integer * perm, integer * givptr,
		   integer * givcol, integer * ldgcol, doublereal * givnum,
		   integer * ldgnum, doublereal * poles, doublereal * difl,
		   doublereal * difr, doublereal * z__, integer * k,
		   doublereal * c__, doublereal * s, doublereal * work,
		   integer * iwork, integer * info);
extern int dlasd7_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * k, doublereal * d__,
		   doublereal * z__, doublereal * zw, doublereal * vf,
		   doublereal * vfw, doublereal * vl, doublereal * vlw,
		   doublereal * alpha, doublereal * beta, doublereal * dsigma,
		   integer * idx, integer * idxp, integer * idxq,
		   integer * perm, integer * givptr, integer * givcol,
		   integer * ldgcol, doublereal * givnum, integer * ldgnum,
		   doublereal * c__, doublereal * s, integer * info);
extern int dlasd8_(integer * icompq, integer * k, doublereal * d__,
		   doublereal * z__, doublereal * vf, doublereal * vl,
		   doublereal * difl, doublereal * difr, integer * lddifr,
		   doublereal * dsigma, doublereal * work, integer * info);
extern int dlasd9_(integer * icompq, integer * ldu, integer * k,
		   doublereal * d__, doublereal * z__, doublereal * vf,
		   doublereal * vl, doublereal * difl, doublereal * difr,
		   doublereal * dsigma, doublereal * work, integer * info);
extern int dlasda_(integer * icompq, integer * smlsiz, integer * n,
		   integer * sqre, doublereal * d__, doublereal * e,
		   doublereal * u, integer * ldu, doublereal * vt,
		   integer * k, doublereal * difl, doublereal * difr,
		   doublereal * z__, doublereal * poles, integer * givptr,
		   integer * givcol, integer * ldgcol, integer * perm,
		   doublereal * givnum, doublereal * c__, doublereal * s,
		   doublereal * work, integer * iwork, integer * info);
extern int dlasdq_(char *uplo, integer * sqre, integer * n, integer * ncvt,
		   integer * nru, integer * ncc, doublereal * d__,
		   doublereal * e, doublereal * vt, integer * ldvt,
		   doublereal * u, integer * ldu, doublereal * c__,
		   integer * ldc, doublereal * work, integer * info,
		   ftnlen uplo_len);
extern int dlasdt_(integer * n, integer * lvl, integer * nd, integer * inode,
		   integer * ndiml, integer * ndimr, integer * msub);
extern int dlaset_(char *uplo, integer * m, integer * n, doublereal * alpha,
		   doublereal * beta, doublereal * a, integer * lda,
		   ftnlen uplo_len);
extern int dlasq1_(integer * n, doublereal * d__, doublereal * e,
		   doublereal * work, integer * info);
extern int dlasq2_(integer * n, doublereal * z__, integer * info);
extern int dlasq3_(integer * i0, integer * n0, doublereal * z__, integer * pp,
		   doublereal * dmin__, doublereal * sigma,
		   doublereal * desig, doublereal * qmax, integer * nfail,
		   integer * iter, integer * ndiv);
extern int dlasq4_(integer * i0, integer * n0, doublereal * z__, integer * pp,
		   integer * n0in, doublereal * dmin__, doublereal * dmin1,
		   doublereal * dmin2, doublereal * dn, doublereal * dn1,
		   doublereal * dn2, doublereal * tau, integer * ttype);
extern int dlasq5_(integer * i0, integer * n0, doublereal * z__, integer * pp,
		   doublereal * tau, doublereal * dmin__, doublereal * dmin1,
		   doublereal * dmin2, doublereal * dn, doublereal * dnm1,
		   doublereal * dnm2);
extern int dlasq6_(integer * i0, integer * n0, doublereal * z__, integer * pp,
		   doublereal * dmin__, doublereal * dmin1,
		   doublereal * dmin2, doublereal * dn, doublereal * dnm1,
		   doublereal * dnm2);
extern int dlasr_(char *side, char *pivot, char *direct, integer * m,
		  integer * n, doublereal * c__, doublereal * s,
		  doublereal * a, integer * lda, ftnlen side_len,
		  ftnlen pivot_len, ftnlen direct_len);
extern int dlasrt_(char *id, integer * n, doublereal * d__, integer * info,
		   ftnlen id_len);
extern int dlassq_(integer * n, doublereal * x, integer * incx,
		   doublereal * scale, doublereal * sumsq);
extern int dlasv2_(doublereal * f, doublereal * g, doublereal * h__,
		   doublereal * ssmin, doublereal * ssmax, doublereal * snr,
		   doublereal * csr, doublereal * snl, doublereal * csl);
extern int dlaswp_(integer * n, doublereal * a, integer * lda, integer * k1,
		   integer * k2, integer * ipiv, integer * incx);
extern int dlasy2_(logical * ltranl, logical * ltranr, integer * isgn,
		   integer * n1, integer * n2, doublereal * tl,
		   integer * ldtl, doublereal * tr, integer * ldtr,
		   doublereal * b, integer * ldb, doublereal * scale,
		   doublereal * x, integer * ldx, doublereal * xnorm,
		   integer * info);
extern int dlasyf_(char *uplo, integer * n, integer * nb, integer * kb,
		   doublereal * a, integer * lda, integer * ipiv,
		   doublereal * w, integer * ldw, integer * info,
		   ftnlen uplo_len);
extern int dlatbs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, integer * kd, doublereal * ab, integer * ldab,
		   doublereal * x, doublereal * scale, doublereal * cnorm,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len, ftnlen normin_len);
extern int dlatdf_(integer * ijob, integer * n, doublereal * z__,
		   integer * ldz, doublereal * rhs, doublereal * rdsum,
		   doublereal * rdscal, integer * ipiv, integer * jpiv);
extern int dlatps_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, doublereal * ap, doublereal * x,
		   doublereal * scale, doublereal * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int dlatrd_(char *uplo, integer * n, integer * nb, doublereal * a,
		   integer * lda, doublereal * e, doublereal * tau,
		   doublereal * w, integer * ldw, ftnlen uplo_len);
extern int dlatrs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, doublereal * a, integer * lda, doublereal * x,
		   doublereal * scale, doublereal * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int dlatrz_(integer * m, integer * n, integer * l, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work);
extern int dlatzm_(char *side, integer * m, integer * n, doublereal * v,
		   integer * incv, doublereal * tau, doublereal * c1,
		   doublereal * c2, integer * ldc, doublereal * work,
		   ftnlen side_len);
extern int dlauu2_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int dlauum_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int dopgtr_(char *uplo, integer * n, doublereal * ap, doublereal * tau,
		   doublereal * q, integer * ldq, doublereal * work,
		   integer * info, ftnlen uplo_len);
extern int dopmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, doublereal * ap, doublereal * tau,
		   doublereal * c__, integer * ldc, doublereal * work,
		   integer * info, ftnlen side_len, ftnlen uplo_len,
		   ftnlen trans_len);
extern int dorg2l_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * info);
extern int dorg2r_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * info);
extern int dorgbr_(char *vect, integer * m, integer * n, integer * k,
		   doublereal * a, integer * lda, doublereal * tau,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen vect_len);
extern int dorghr_(integer * n, integer * ilo, integer * ihi, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dorgl2_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * info);
extern int dorglq_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dorgql_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dorgqr_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dorgr2_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * info);
extern int dorgrq_(integer * m, integer * n, integer * k, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * work,
		   integer * lwork, integer * info);
extern int dorgtr_(char *uplo, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int dorm2l_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int dorm2r_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int dormbr_(char *vect, char *side, char *trans, integer * m,
		   integer * n, integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen vect_len, ftnlen side_len, ftnlen trans_len);
extern int dormhr_(char *side, char *trans, integer * m, integer * n,
		   integer * ilo, integer * ihi, doublereal * a,
		   integer * lda, doublereal * tau, doublereal * c__,
		   integer * ldc, doublereal * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int dorml2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int dormlq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int dormql_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int dormqr_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int dormr2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int dormr3_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int dormrq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int dormrz_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int dormtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * c__, integer * ldc,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen uplo_len, ftnlen trans_len);
extern int dpbcon_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, doublereal * anorm, doublereal * rcond,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int dpbequ_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, doublereal * s, doublereal * scond,
		   doublereal * amax, integer * info, ftnlen uplo_len);
extern int dpbrfs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   doublereal * ab, integer * ldab, doublereal * afb,
		   integer * ldafb, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int dpbstf_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int dpbsv_(char *uplo, integer * n, integer * kd, integer * nrhs,
		  doublereal * ab, integer * ldab, doublereal * b,
		  integer * ldb, integer * info, ftnlen uplo_len);
extern int dpbsvx_(char *fact, char *uplo, integer * n, integer * kd,
		   integer * nrhs, doublereal * ab, integer * ldab,
		   doublereal * afb, integer * ldafb, char *equed,
		   doublereal * s, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublereal * work,
		   integer * iwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int dpbtf2_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int dpbtrf_(char *uplo, integer * n, integer * kd, doublereal * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int dpbtrs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   doublereal * ab, integer * ldab, doublereal * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int dpocon_(char *uplo, integer * n, doublereal * a, integer * lda,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int dpoequ_(integer * n, doublereal * a, integer * lda, doublereal * s,
		   doublereal * scond, doublereal * amax, integer * info);
extern int dporfs_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * af, integer * ldaf,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int dposv_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		  integer * lda, doublereal * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int dposvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublereal * a, integer * lda, doublereal * af,
		   integer * ldaf, char *equed, doublereal * s,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len,
		   ftnlen equed_len);
extern int dpotf2_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int dpotrf_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int dpotri_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int dpotrs_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int dppcon_(char *uplo, integer * n, doublereal * ap,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int dppequ_(char *uplo, integer * n, doublereal * ap, doublereal * s,
		   doublereal * scond, doublereal * amax, integer * info,
		   ftnlen uplo_len);
extern int dpprfs_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		   doublereal * afp, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int dppsv_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		  doublereal * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int dppsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublereal * ap, doublereal * afp, char *equed,
		   doublereal * s, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublereal * work,
		   integer * iwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int dpptrf_(char *uplo, integer * n, doublereal * ap, integer * info,
		   ftnlen uplo_len);
extern int dpptri_(char *uplo, integer * n, doublereal * ap, integer * info,
		   ftnlen uplo_len);
extern int dpptrs_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		   doublereal * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int dptcon_(integer * n, doublereal * d__, doublereal * e,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * info);
extern int dpteqr_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * info, ftnlen compz_len);
extern int dptrfs_(integer * n, integer * nrhs, doublereal * d__,
		   doublereal * e, doublereal * df, doublereal * ef,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * info);
extern int dptsv_(integer * n, integer * nrhs, doublereal * d__,
		  doublereal * e, doublereal * b, integer * ldb,
		  integer * info);
extern int dptsvx_(char *fact, integer * n, integer * nrhs, doublereal * d__,
		   doublereal * e, doublereal * df, doublereal * ef,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * info,
		   ftnlen fact_len);
extern int dpttrf_(integer * n, doublereal * d__, doublereal * e,
		   integer * info);
extern int dpttrs_(integer * n, integer * nrhs, doublereal * d__,
		   doublereal * e, doublereal * b, integer * ldb,
		   integer * info);
extern int dptts2_(integer * n, integer * nrhs, doublereal * d__,
		   doublereal * e, doublereal * b, integer * ldb);
extern int drscl_(integer * n, doublereal * sa, doublereal * sx,
		  integer * incx);
extern int dsbev_(char *jobz, char *uplo, integer * n, integer * kd,
		  doublereal * ab, integer * ldab, doublereal * w,
		  doublereal * z__, integer * ldz, doublereal * work,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsbevd_(char *jobz, char *uplo, integer * n, integer * kd,
		   doublereal * ab, integer * ldab, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsbevx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * kd, doublereal * ab, integer * ldab,
		   doublereal * q, integer * ldq, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int dsbgst_(char *vect, char *uplo, integer * n, integer * ka,
		   integer * kb, doublereal * ab, integer * ldab,
		   doublereal * bb, integer * ldbb, doublereal * x,
		   integer * ldx, doublereal * work, integer * info,
		   ftnlen vect_len, ftnlen uplo_len);
extern int dsbgv_(char *jobz, char *uplo, integer * n, integer * ka,
		  integer * kb, doublereal * ab, integer * ldab,
		  doublereal * bb, integer * ldbb, doublereal * w,
		  doublereal * z__, integer * ldz, doublereal * work,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsbgvd_(char *jobz, char *uplo, integer * n, integer * ka,
		   integer * kb, doublereal * ab, integer * ldab,
		   doublereal * bb, integer * ldbb, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsbgvx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * ka, integer * kb, doublereal * ab,
		   integer * ldab, doublereal * bb, integer * ldbb,
		   doublereal * q, integer * ldq, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int dsbtrd_(char *vect, char *uplo, integer * n, integer * kd,
		   doublereal * ab, integer * ldab, doublereal * d__,
		   doublereal * e, doublereal * q, integer * ldq,
		   doublereal * work, integer * info, ftnlen vect_len,
		   ftnlen uplo_len);
extern doublereal dsecnd_(void);
extern int dspcon_(char *uplo, integer * n, doublereal * ap, integer * ipiv,
		   doublereal * anorm, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int dspev_(char *jobz, char *uplo, integer * n, doublereal * ap,
		  doublereal * w, doublereal * z__, integer * ldz,
		  doublereal * work, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int dspevd_(char *jobz, char *uplo, integer * n, doublereal * ap,
		   doublereal * w, doublereal * z__, integer * ldz,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int dspevx_(char *jobz, char *range, char *uplo, integer * n,
		   doublereal * ap, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublereal * z__,
		   integer * ldz, doublereal * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int dspgst_(integer * itype, char *uplo, integer * n, doublereal * ap,
		   doublereal * bp, integer * info, ftnlen uplo_len);
extern int dspgv_(integer * itype, char *jobz, char *uplo, integer * n,
		  doublereal * ap, doublereal * bp, doublereal * w,
		  doublereal * z__, integer * ldz, doublereal * work,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dspgvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   doublereal * ap, doublereal * bp, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dspgvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, doublereal * ap, doublereal * bp,
		   doublereal * vl, doublereal * vu, integer * il,
		   integer * iu, doublereal * abstol, integer * m,
		   doublereal * w, doublereal * z__, integer * ldz,
		   doublereal * work, integer * iwork, integer * ifail,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int dsprfs_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		   doublereal * afp, integer * ipiv, doublereal * b,
		   integer * ldb, doublereal * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublereal * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int dspsv_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		  integer * ipiv, doublereal * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int dspsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublereal * ap, doublereal * afp, integer * ipiv,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int dsptrd_(char *uplo, integer * n, doublereal * ap, doublereal * d__,
		   doublereal * e, doublereal * tau, integer * info,
		   ftnlen uplo_len);
extern int dsptrf_(char *uplo, integer * n, doublereal * ap, integer * ipiv,
		   integer * info, ftnlen uplo_len);
extern int dsptri_(char *uplo, integer * n, doublereal * ap, integer * ipiv,
		   doublereal * work, integer * info, ftnlen uplo_len);
extern int dsptrs_(char *uplo, integer * n, integer * nrhs, doublereal * ap,
		   integer * ipiv, doublereal * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int dstebz_(char *range, char *order, integer * n, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, doublereal * d__, doublereal * e,
		   integer * m, integer * nsplit, doublereal * w,
		   integer * iblock, integer * isplit, doublereal * work,
		   integer * iwork, integer * info, ftnlen range_len,
		   ftnlen order_len);
extern int dstedc_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen compz_len);
extern int dstegr_(char *jobz, char *range, integer * n, doublereal * d__,
		   doublereal * e, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublereal * z__,
		   integer * ldz, integer * isuppz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int dstein_(integer * n, doublereal * d__, doublereal * e, integer * m,
		   doublereal * w, integer * iblock, integer * isplit,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * iwork, integer * ifail, integer * info);
extern int dsteqr_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * info, ftnlen compz_len);
extern int dsterf_(integer * n, doublereal * d__, doublereal * e,
		   integer * info);
extern int dstev_(char *jobz, integer * n, doublereal * d__, doublereal * e,
		  doublereal * z__, integer * ldz, doublereal * work,
		  integer * info, ftnlen jobz_len);
extern int dstevd_(char *jobz, integer * n, doublereal * d__, doublereal * e,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len);
extern int dstevr_(char *jobz, char *range, integer * n, doublereal * d__,
		   doublereal * e, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublereal * z__,
		   integer * ldz, integer * isuppz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int dstevx_(char *jobz, char *range, integer * n, doublereal * d__,
		   doublereal * e, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublereal * z__,
		   integer * ldz, doublereal * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len);
extern int dsycon_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, doublereal * anorm, doublereal * rcond,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int dsyev_(char *jobz, char *uplo, integer * n, doublereal * a,
		  integer * lda, doublereal * w, doublereal * work,
		  integer * lwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int dsyevd_(char *jobz, char *uplo, integer * n, doublereal * a,
		   integer * lda, doublereal * w, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsyevr_(char *jobz, char *range, char *uplo, integer * n,
		   doublereal * a, integer * lda, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublereal * z__, integer * ldz, integer * isuppz,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int dsyevx_(char *jobz, char *range, char *uplo, integer * n,
		   doublereal * a, integer * lda, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublereal * z__, integer * ldz, doublereal * work,
		   integer * lwork, integer * iwork, integer * ifail,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int dsygs2_(integer * itype, char *uplo, integer * n, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int dsygst_(integer * itype, char *uplo, integer * n, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int dsygv_(integer * itype, char *jobz, char *uplo, integer * n,
		  doublereal * a, integer * lda, doublereal * b,
		  integer * ldb, doublereal * w, doublereal * work,
		  integer * lwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int dsygvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * w, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int dsygvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublereal * z__,
		   integer * ldz, doublereal * work, integer * lwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int dsyrfs_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, doublereal * af, integer * ldaf,
		   integer * ipiv, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int dsysv_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		  integer * lda, integer * ipiv, doublereal * b,
		  integer * ldb, doublereal * work, integer * lwork,
		  integer * info, ftnlen uplo_len);
extern int dsysvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublereal * a, integer * lda, doublereal * af,
		   integer * ldaf, integer * ipiv, doublereal * b,
		   integer * ldb, doublereal * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int dsytd2_(char *uplo, integer * n, doublereal * a, integer * lda,
		   doublereal * d__, doublereal * e, doublereal * tau,
		   integer * info, ftnlen uplo_len);
extern int dsytf2_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int dsytrd_(char *uplo, integer * n, doublereal * a, integer * lda,
		   doublereal * d__, doublereal * e, doublereal * tau,
		   doublereal * work, integer * lwork, integer * info,
		   ftnlen uplo_len);
extern int dsytrf_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, doublereal * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int dsytri_(char *uplo, integer * n, doublereal * a, integer * lda,
		   integer * ipiv, doublereal * work, integer * info,
		   ftnlen uplo_len);
extern int dsytrs_(char *uplo, integer * n, integer * nrhs, doublereal * a,
		   integer * lda, integer * ipiv, doublereal * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int dtbcon_(char *norm, char *uplo, char *diag, integer * n,
		   integer * kd, doublereal * ab, integer * ldab,
		   doublereal * rcond, doublereal * work, integer * iwork,
		   integer * info, ftnlen norm_len, ftnlen uplo_len,
		   ftnlen diag_len);
extern int dtbrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, doublereal * ab,
		   integer * ldab, doublereal * b, integer * ldb,
		   doublereal * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublereal * work, integer * iwork,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int dtbtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, doublereal * ab,
		   integer * ldab, doublereal * b, integer * ldb,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int dtgevc_(char *side, char *howmny, logical * select, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * vl, integer * ldvl,
		   doublereal * vr, integer * ldvr, integer * mm, integer * m,
		   doublereal * work, integer * info, ftnlen side_len,
		   ftnlen howmny_len);
extern int dtgex2_(logical * wantq, logical * wantz, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * q, integer * ldq,
		   doublereal * z__, integer * ldz, integer * j1,
		   integer * n1, integer * n2, doublereal * work,
		   integer * lwork, integer * info);
extern int dtgexc_(logical * wantq, logical * wantz, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * q, integer * ldq,
		   doublereal * z__, integer * ldz, integer * ifst,
		   integer * ilst, doublereal * work, integer * lwork,
		   integer * info);
extern int dtgsen_(integer * ijob, logical * wantq, logical * wantz,
		   logical * select, integer * n, doublereal * a,
		   integer * lda, doublereal * b, integer * ldb,
		   doublereal * alphar, doublereal * alphai,
		   doublereal * beta, doublereal * q, integer * ldq,
		   doublereal * z__, integer * ldz, integer * m,
		   doublereal * pl, doublereal * pr, doublereal * dif,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info);
extern int dtgsja_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, integer * k, integer * l,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * tola, doublereal * tolb,
		   doublereal * alpha, doublereal * beta, doublereal * u,
		   integer * ldu, doublereal * v, integer * ldv,
		   doublereal * q, integer * ldq, doublereal * work,
		   integer * ncycle, integer * info, ftnlen jobu_len,
		   ftnlen jobv_len, ftnlen jobq_len);
extern int dtgsna_(char *job, char *howmny, logical * select, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * vl, integer * ldvl,
		   doublereal * vr, integer * ldvr, doublereal * s,
		   doublereal * dif, integer * mm, integer * m,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen job_len, ftnlen howmny_len);
extern int dtgsy2_(char *trans, integer * ijob, integer * m, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * c__, integer * ldc,
		   doublereal * d__, integer * ldd, doublereal * e,
		   integer * lde, doublereal * f, integer * ldf,
		   doublereal * scale, doublereal * rdsum,
		   doublereal * rdscal, integer * iwork, integer * pq,
		   integer * info, ftnlen trans_len);
extern int dtgsyl_(char *trans, integer * ijob, integer * m, integer * n,
		   doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * c__, integer * ldc,
		   doublereal * d__, integer * ldd, doublereal * e,
		   integer * lde, doublereal * f, integer * ldf,
		   doublereal * scale, doublereal * dif, doublereal * work,
		   integer * lwork, integer * iwork, integer * info,
		   ftnlen trans_len);
extern int dtpcon_(char *norm, char *uplo, char *diag, integer * n,
		   doublereal * ap, doublereal * rcond, doublereal * work,
		   integer * iwork, integer * info, ftnlen norm_len,
		   ftnlen uplo_len, ftnlen diag_len);
extern int dtprfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublereal * ap, doublereal * b,
		   integer * ldb, doublereal * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublereal * work,
		   integer * iwork, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int dtptri_(char *uplo, char *diag, integer * n, doublereal * ap,
		   integer * info, ftnlen uplo_len, ftnlen diag_len);
extern int dtptrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublereal * ap, doublereal * b,
		   integer * ldb, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int dtrcon_(char *norm, char *uplo, char *diag, integer * n,
		   doublereal * a, integer * lda, doublereal * rcond,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int dtrevc_(char *side, char *howmny, logical * select, integer * n,
		   doublereal * t, integer * ldt, doublereal * vl,
		   integer * ldvl, doublereal * vr, integer * ldvr,
		   integer * mm, integer * m, doublereal * work,
		   integer * info, ftnlen side_len, ftnlen howmny_len);
extern int dtrexc_(char *compq, integer * n, doublereal * t, integer * ldt,
		   doublereal * q, integer * ldq, integer * ifst,
		   integer * ilst, doublereal * work, integer * info,
		   ftnlen compq_len);
extern int dtrrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, doublereal * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublereal * work, integer * iwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int dtrsen_(char *job, char *compq, logical * select, integer * n,
		   doublereal * t, integer * ldt, doublereal * q,
		   integer * ldq, doublereal * wr, doublereal * wi,
		   integer * m, doublereal * s, doublereal * sep,
		   doublereal * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen job_len,
		   ftnlen compq_len);
extern int dtrsna_(char *job, char *howmny, logical * select, integer * n,
		   doublereal * t, integer * ldt, doublereal * vl,
		   integer * ldvl, doublereal * vr, integer * ldvr,
		   doublereal * s, doublereal * sep, integer * mm,
		   integer * m, doublereal * work, integer * ldwork,
		   integer * iwork, integer * info, ftnlen job_len,
		   ftnlen howmny_len);
extern int dtrsyl_(char *trana, char *tranb, integer * isgn, integer * m,
		   integer * n, doublereal * a, integer * lda, doublereal * b,
		   integer * ldb, doublereal * c__, integer * ldc,
		   doublereal * scale, integer * info, ftnlen trana_len,
		   ftnlen tranb_len);
extern int dtrti2_(char *uplo, char *diag, integer * n, doublereal * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int dtrtri_(char *uplo, char *diag, integer * n, doublereal * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int dtrtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublereal * a, integer * lda,
		   doublereal * b, integer * ldb, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int dtzrqf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, integer * info);
extern int dtzrzf_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublereal * tau, doublereal * work, integer * lwork,
		   integer * info);
extern doublereal dzsum1_(integer * n, doublecomplex * cx, integer * incx);
extern integer icmax1_(integer * n, complex * cx, integer * incx);
extern integer ieeeck_(integer * ispec, real * zero, real * one);
extern integer ilaenv_(integer * ispec, char *name__, char *opts,
		       integer * n1, integer * n2, integer * n3, integer * n4,
		       ftnlen name_len, ftnlen opts_len);
extern integer izmax1_(integer * n, doublecomplex * cx, integer * incx);
extern logical lsame_(char *ca, char *cb, ftnlen ca_len, ftnlen cb_len);
extern logical lsamen_(integer * n, char *ca, char *cb, ftnlen ca_len,
		       ftnlen cb_len);
extern int sbdsdc_(char *uplo, char *compq, integer * n, real * d__, real * e,
		   real * u, integer * ldu, real * vt, integer * ldvt,
		   real * q, integer * iq, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len, ftnlen compq_len);
extern int sbdsqr_(char *uplo, integer * n, integer * ncvt, integer * nru,
		   integer * ncc, real * d__, real * e, real * vt,
		   integer * ldvt, real * u, integer * ldu, real * c__,
		   integer * ldc, real * work, integer * info,
		   ftnlen uplo_len);
extern E_f scsum1_(integer * n, complex * cx, integer * incx);
extern int sdisna_(char *job, integer * m, integer * n, real * d__,
		   real * sep, integer * info, ftnlen job_len);
extern E_f second_(void);
extern int sgbbrd_(char *vect, integer * m, integer * n, integer * ncc,
		   integer * kl, integer * ku, real * ab, integer * ldab,
		   real * d__, real * e, real * q, integer * ldq, real * pt,
		   integer * ldpt, real * c__, integer * ldc, real * work,
		   integer * info, ftnlen vect_len);
extern int sgbcon_(char *norm, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, integer * ipiv, real * anorm,
		   real * rcond, real * work, integer * iwork, integer * info,
		   ftnlen norm_len);
extern int sgbequ_(integer * m, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, real * r__, real * c__,
		   real * rowcnd, real * colcnd, real * amax, integer * info);
extern int sgbrfs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, real * ab, integer * ldab, real * afb,
		   integer * ldafb, integer * ipiv, real * b, integer * ldb,
		   real * x, integer * ldx, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen trans_len);
extern int sgbsv_(integer * n, integer * kl, integer * ku, integer * nrhs,
		  real * ab, integer * ldab, integer * ipiv, real * b,
		  integer * ldb, integer * info);
extern int sgbsvx_(char *fact, char *trans, integer * n, integer * kl,
		   integer * ku, integer * nrhs, real * ab, integer * ldab,
		   real * afb, integer * ldafb, integer * ipiv, char *equed,
		   real * r__, real * c__, real * b, integer * ldb, real * x,
		   integer * ldx, real * rcond, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen fact_len, ftnlen trans_len, ftnlen equed_len);
extern int sgbtf2_(integer * m, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, integer * ipiv, integer * info);
extern int sgbtrf_(integer * m, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, integer * ipiv, integer * info);
extern int sgbtrs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, real * ab, integer * ldab, integer * ipiv,
		   real * b, integer * ldb, integer * info, ftnlen trans_len);
extern int sgebak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, real * scale, integer * m, real * v,
		   integer * ldv, integer * info, ftnlen job_len,
		   ftnlen side_len);
extern int sgebal_(char *job, integer * n, real * a, integer * lda,
		   integer * ilo, integer * ihi, real * scale, integer * info,
		   ftnlen job_len);
extern int sgebd2_(integer * m, integer * n, real * a, integer * lda,
		   real * d__, real * e, real * tauq, real * taup,
		   real * work, integer * info);
extern int sgebrd_(integer * m, integer * n, real * a, integer * lda,
		   real * d__, real * e, real * tauq, real * taup,
		   real * work, integer * lwork, integer * info);
extern int sgecon_(char *norm, integer * n, real * a, integer * lda,
		   real * anorm, real * rcond, real * work, integer * iwork,
		   integer * info, ftnlen norm_len);
extern int sgeequ_(integer * m, integer * n, real * a, integer * lda,
		   real * r__, real * c__, real * rowcnd, real * colcnd,
		   real * amax, integer * info);
extern int sgees_(char *jobvs, char *sort, L_fp select, integer * n, real * a,
		  integer * lda, integer * sdim, real * wr, real * wi,
		  real * vs, integer * ldvs, real * work, integer * lwork,
		  logical * bwork, integer * info, ftnlen jobvs_len,
		  ftnlen sort_len);
extern int sgeesx_(char *jobvs, char *sort, L_fp select, char *sense,
		   integer * n, real * a, integer * lda, integer * sdim,
		   real * wr, real * wi, real * vs, integer * ldvs,
		   real * rconde, real * rcondv, real * work, integer * lwork,
		   integer * iwork, integer * liwork, logical * bwork,
		   integer * info, ftnlen jobvs_len, ftnlen sort_len,
		   ftnlen sense_len);
extern int sgeev_(char *jobvl, char *jobvr, integer * n, real * a,
		  integer * lda, real * wr, real * wi, real * vl,
		  integer * ldvl, real * vr, integer * ldvr, real * work,
		  integer * lwork, integer * info, ftnlen jobvl_len,
		  ftnlen jobvr_len);
extern int sgeevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, real * a, integer * lda, real * wr, real * wi,
		   real * vl, integer * ldvl, real * vr, integer * ldvr,
		   integer * ilo, integer * ihi, real * scale, real * abnrm,
		   real * rconde, real * rcondv, real * work, integer * lwork,
		   integer * iwork, integer * info, ftnlen balanc_len,
		   ftnlen jobvl_len, ftnlen jobvr_len, ftnlen sense_len);
extern int sgegs_(char *jobvsl, char *jobvsr, integer * n, real * a,
		  integer * lda, real * b, integer * ldb, real * alphar,
		  real * alphai, real * beta, real * vsl, integer * ldvsl,
		  real * vsr, integer * ldvsr, real * work, integer * lwork,
		  integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len);
extern int sgegv_(char *jobvl, char *jobvr, integer * n, real * a,
		  integer * lda, real * b, integer * ldb, real * alphar,
		  real * alphai, real * beta, real * vl, integer * ldvl,
		  real * vr, integer * ldvr, real * work, integer * lwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int sgehd2_(integer * n, integer * ilo, integer * ihi, real * a,
		   integer * lda, real * tau, real * work, integer * info);
extern int sgehrd_(integer * n, integer * ilo, integer * ihi, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sgelq2_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * info);
extern int sgelqf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info);
extern int sgels_(char *trans, integer * m, integer * n, integer * nrhs,
		  real * a, integer * lda, real * b, integer * ldb,
		  real * work, integer * lwork, integer * info,
		  ftnlen trans_len);
extern int sgelsd_(integer * m, integer * n, integer * nrhs, real * a,
		   integer * lda, real * b, integer * ldb, real * s,
		   real * rcond, integer * rank, real * work, integer * lwork,
		   integer * iwork, integer * info);
extern int sgelss_(integer * m, integer * n, integer * nrhs, real * a,
		   integer * lda, real * b, integer * ldb, real * s,
		   real * rcond, integer * rank, real * work, integer * lwork,
		   integer * info);
extern int sgelsx_(integer * m, integer * n, integer * nrhs, real * a,
		   integer * lda, real * b, integer * ldb, integer * jpvt,
		   real * rcond, integer * rank, real * work, integer * info);
extern int sgelsy_(integer * m, integer * n, integer * nrhs, real * a,
		   integer * lda, real * b, integer * ldb, integer * jpvt,
		   real * rcond, integer * rank, real * work, integer * lwork,
		   integer * info);
extern int sgeql2_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * info);
extern int sgeqlf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info);
extern int sgeqp3_(integer * m, integer * n, real * a, integer * lda,
		   integer * jpvt, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sgeqpf_(integer * m, integer * n, real * a, integer * lda,
		   integer * jpvt, real * tau, real * work, integer * info);
extern int sgeqr2_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * info);
extern int sgeqrf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info);
extern int sgerfs_(char *trans, integer * n, integer * nrhs, real * a,
		   integer * lda, real * af, integer * ldaf, integer * ipiv,
		   real * b, integer * ldb, real * x, integer * ldx,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen trans_len);
extern int sgerq2_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * info);
extern int sgerqf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info);
extern int sgesc2_(integer * n, real * a, integer * lda, real * rhs,
		   integer * ipiv, integer * jpiv, real * scale);
extern int sgesdd_(char *jobz, integer * m, integer * n, real * a,
		   integer * lda, real * s, real * u, integer * ldu,
		   real * vt, integer * ldvt, real * work, integer * lwork,
		   integer * iwork, integer * info, ftnlen jobz_len);
extern int sgesv_(integer * n, integer * nrhs, real * a, integer * lda,
		  integer * ipiv, real * b, integer * ldb, integer * info);
extern int sgesvd_(char *jobu, char *jobvt, integer * m, integer * n,
		   real * a, integer * lda, real * s, real * u, integer * ldu,
		   real * vt, integer * ldvt, real * work, integer * lwork,
		   integer * info, ftnlen jobu_len, ftnlen jobvt_len);
extern int sgesvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   real * a, integer * lda, real * af, integer * ldaf,
		   integer * ipiv, char *equed, real * r__, real * c__,
		   real * b, integer * ldb, real * x, integer * ldx,
		   real * rcond, real * ferr, real * berr, real * work,
		   integer * iwork, integer * info, ftnlen fact_len,
		   ftnlen trans_len, ftnlen equed_len);
extern int sgetc2_(integer * n, real * a, integer * lda, integer * ipiv,
		   integer * jpiv, integer * info);
extern int sgetf2_(integer * m, integer * n, real * a, integer * lda,
		   integer * ipiv, integer * info);
extern int sgetrf_(integer * m, integer * n, real * a, integer * lda,
		   integer * ipiv, integer * info);
extern int sgetri_(integer * n, real * a, integer * lda, integer * ipiv,
		   real * work, integer * lwork, integer * info);
extern int sgetrs_(char *trans, integer * n, integer * nrhs, real * a,
		   integer * lda, integer * ipiv, real * b, integer * ldb,
		   integer * info, ftnlen trans_len);
extern int sggbak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, real * lscale, real * rscale, integer * m,
		   real * v, integer * ldv, integer * info, ftnlen job_len,
		   ftnlen side_len);
extern int sggbal_(char *job, integer * n, real * a, integer * lda, real * b,
		   integer * ldb, integer * ilo, integer * ihi, real * lscale,
		   real * rscale, real * work, integer * info,
		   ftnlen job_len);
extern int sgges_(char *jobvsl, char *jobvsr, char *sort, L_fp selctg,
		  integer * n, real * a, integer * lda, real * b,
		  integer * ldb, integer * sdim, real * alphar, real * alphai,
		  real * beta, real * vsl, integer * ldvsl, real * vsr,
		  integer * ldvsr, real * work, integer * lwork,
		  logical * bwork, integer * info, ftnlen jobvsl_len,
		  ftnlen jobvsr_len, ftnlen sort_len);
extern int sggesx_(char *jobvsl, char *jobvsr, char *sort, L_fp selctg,
		   char *sense, integer * n, real * a, integer * lda,
		   real * b, integer * ldb, integer * sdim, real * alphar,
		   real * alphai, real * beta, real * vsl, integer * ldvsl,
		   real * vsr, integer * ldvsr, real * rconde, real * rcondv,
		   real * work, integer * lwork, integer * iwork,
		   integer * liwork, logical * bwork, integer * info,
		   ftnlen jobvsl_len, ftnlen jobvsr_len, ftnlen sort_len,
		   ftnlen sense_len);
extern int sggev_(char *jobvl, char *jobvr, integer * n, real * a,
		  integer * lda, real * b, integer * ldb, real * alphar,
		  real * alphai, real * beta, real * vl, integer * ldvl,
		  real * vr, integer * ldvr, real * work, integer * lwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int sggevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, real * a, integer * lda, real * b,
		   integer * ldb, real * alphar, real * alphai, real * beta,
		   real * vl, integer * ldvl, real * vr, integer * ldvr,
		   integer * ilo, integer * ihi, real * lscale, real * rscale,
		   real * abnrm, real * bbnrm, real * rconde, real * rcondv,
		   real * work, integer * lwork, integer * iwork,
		   logical * bwork, integer * info, ftnlen balanc_len,
		   ftnlen jobvl_len, ftnlen jobvr_len, ftnlen sense_len);
extern int sggglm_(integer * n, integer * m, integer * p, real * a,
		   integer * lda, real * b, integer * ldb, real * d__,
		   real * x, real * y, real * work, integer * lwork,
		   integer * info);
extern int sgghrd_(char *compq, char *compz, integer * n, integer * ilo,
		   integer * ihi, real * a, integer * lda, real * b,
		   integer * ldb, real * q, integer * ldq, real * z__,
		   integer * ldz, integer * info, ftnlen compq_len,
		   ftnlen compz_len);
extern int sgglse_(integer * m, integer * n, integer * p, real * a,
		   integer * lda, real * b, integer * ldb, real * c__,
		   real * d__, real * x, real * work, integer * lwork,
		   integer * info);
extern int sggqrf_(integer * n, integer * m, integer * p, real * a,
		   integer * lda, real * taua, real * b, integer * ldb,
		   real * taub, real * work, integer * lwork, integer * info);
extern int sggrqf_(integer * m, integer * p, integer * n, real * a,
		   integer * lda, real * taua, real * b, integer * ldb,
		   real * taub, real * work, integer * lwork, integer * info);
extern int sggsvd_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * n, integer * p, integer * k, integer * l,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * alpha, real * beta, real * u, integer * ldu,
		   real * v, integer * ldv, real * q, integer * ldq,
		   real * work, integer * iwork, integer * info,
		   ftnlen jobu_len, ftnlen jobv_len, ftnlen jobq_len);
extern int sggsvp_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, real * a, integer * lda,
		   real * b, integer * ldb, real * tola, real * tolb,
		   integer * k, integer * l, real * u, integer * ldu,
		   real * v, integer * ldv, real * q, integer * ldq,
		   integer * iwork, real * tau, real * work, integer * info,
		   ftnlen jobu_len, ftnlen jobv_len, ftnlen jobq_len);
extern int sgtcon_(char *norm, integer * n, real * dl, real * d__, real * du,
		   real * du2, integer * ipiv, real * anorm, real * rcond,
		   real * work, integer * iwork, integer * info,
		   ftnlen norm_len);
extern int sgtrfs_(char *trans, integer * n, integer * nrhs, real * dl,
		   real * d__, real * du, real * dlf, real * df, real * duf,
		   real * du2, integer * ipiv, real * b, integer * ldb,
		   real * x, integer * ldx, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen trans_len);
extern int sgtsv_(integer * n, integer * nrhs, real * dl, real * d__,
		  real * du, real * b, integer * ldb, integer * info);
extern int sgtsvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   real * dl, real * d__, real * du, real * dlf, real * df,
		   real * duf, real * du2, integer * ipiv, real * b,
		   integer * ldb, real * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen trans_len);
extern int sgttrf_(integer * n, real * dl, real * d__, real * du, real * du2,
		   integer * ipiv, integer * info);
extern int sgttrs_(char *trans, integer * n, integer * nrhs, real * dl,
		   real * d__, real * du, real * du2, integer * ipiv,
		   real * b, integer * ldb, integer * info, ftnlen trans_len);
extern int sgtts2_(integer * itrans, integer * n, integer * nrhs, real * dl,
		   real * d__, real * du, real * du2, integer * ipiv,
		   real * b, integer * ldb);
extern int shgeqz_(char *job, char *compq, char *compz, integer * n,
		   integer * ilo, integer * ihi, real * a, integer * lda,
		   real * b, integer * ldb, real * alphar, real * alphai,
		   real * beta, real * q, integer * ldq, real * z__,
		   integer * ldz, real * work, integer * lwork,
		   integer * info, ftnlen job_len, ftnlen compq_len,
		   ftnlen compz_len);
extern int shsein_(char *side, char *eigsrc, char *initv, logical * select,
		   integer * n, real * h__, integer * ldh, real * wr,
		   real * wi, real * vl, integer * ldvl, real * vr,
		   integer * ldvr, integer * mm, integer * m, real * work,
		   integer * ifaill, integer * ifailr, integer * info,
		   ftnlen side_len, ftnlen eigsrc_len, ftnlen initv_len);
extern int shseqr_(char *job, char *compz, integer * n, integer * ilo,
		   integer * ihi, real * h__, integer * ldh, real * wr,
		   real * wi, real * z__, integer * ldz, real * work,
		   integer * lwork, integer * info, ftnlen job_len,
		   ftnlen compz_len);
extern int slabad_(real * small, real * large);
extern int slabrd_(integer * m, integer * n, integer * nb, real * a,
		   integer * lda, real * d__, real * e, real * tauq,
		   real * taup, real * x, integer * ldx, real * y,
		   integer * ldy);
extern int slacon_(integer * n, real * v, real * x, integer * isgn,
		   real * est, integer * kase);
extern int slacpy_(char *uplo, integer * m, integer * n, real * a,
		   integer * lda, real * b, integer * ldb, ftnlen uplo_len);
extern int sladiv_(real * a, real * b, real * c__, real * d__, real * p,
		   real * q);
extern int slae2_(real * a, real * b, real * c__, real * rt1, real * rt2);
extern int slaebz_(integer * ijob, integer * nitmax, integer * n,
		   integer * mmax, integer * minp, integer * nbmin,
		   real * abstol, real * reltol, real * pivmin, real * d__,
		   real * e, real * e2, integer * nval, real * ab, real * c__,
		   integer * mout, integer * nab, real * work,
		   integer * iwork, integer * info);
extern int slaed0_(integer * icompq, integer * qsiz, integer * n, real * d__,
		   real * e, real * q, integer * ldq, real * qstore,
		   integer * ldqs, real * work, integer * iwork,
		   integer * info);
extern int slaed1_(integer * n, real * d__, real * q, integer * ldq,
		   integer * indxq, real * rho, integer * cutpnt, real * work,
		   integer * iwork, integer * info);
extern int slaed2_(integer * k, integer * n, integer * n1, real * d__,
		   real * q, integer * ldq, integer * indxq, real * rho,
		   real * z__, real * dlamda, real * w, real * q2,
		   integer * indx, integer * indxc, integer * indxp,
		   integer * coltyp, integer * info);
extern int slaed3_(integer * k, integer * n, integer * n1, real * d__,
		   real * q, integer * ldq, real * rho, real * dlamda,
		   real * q2, integer * indx, integer * ctot, real * w,
		   real * s, integer * info);
extern int slaed4_(integer * n, integer * i__, real * d__, real * z__,
		   real * delta, real * rho, real * dlam, integer * info);
extern int slaed5_(integer * i__, real * d__, real * z__, real * delta,
		   real * rho, real * dlam);
extern int slaed6_(integer * kniter, logical * orgati, real * rho, real * d__,
		   real * z__, real * finit, real * tau, integer * info);
extern int slaed7_(integer * icompq, integer * n, integer * qsiz,
		   integer * tlvls, integer * curlvl, integer * curpbm,
		   real * d__, real * q, integer * ldq, integer * indxq,
		   real * rho, integer * cutpnt, real * qstore,
		   integer * qptr, integer * prmptr, integer * perm,
		   integer * givptr, integer * givcol, real * givnum,
		   real * work, integer * iwork, integer * info);
extern int slaed8_(integer * icompq, integer * k, integer * n, integer * qsiz,
		   real * d__, real * q, integer * ldq, integer * indxq,
		   real * rho, integer * cutpnt, real * z__, real * dlamda,
		   real * q2, integer * ldq2, real * w, integer * perm,
		   integer * givptr, integer * givcol, real * givnum,
		   integer * indxp, integer * indx, integer * info);
extern int slaed9_(integer * k, integer * kstart, integer * kstop,
		   integer * n, real * d__, real * q, integer * ldq,
		   real * rho, real * dlamda, real * w, real * s,
		   integer * lds, integer * info);
extern int slaeda_(integer * n, integer * tlvls, integer * curlvl,
		   integer * curpbm, integer * prmptr, integer * perm,
		   integer * givptr, integer * givcol, real * givnum,
		   real * q, integer * qptr, real * z__, real * ztemp,
		   integer * info);
extern int slaein_(logical * rightv, logical * noinit, integer * n,
		   real * h__, integer * ldh, real * wr, real * wi, real * vr,
		   real * vi, real * b, integer * ldb, real * work,
		   real * eps3, real * smlnum, real * bignum, integer * info);
extern int slaev2_(real * a, real * b, real * c__, real * rt1, real * rt2,
		   real * cs1, real * sn1);
extern int slaexc_(logical * wantq, integer * n, real * t, integer * ldt,
		   real * q, integer * ldq, integer * j1, integer * n1,
		   integer * n2, real * work, integer * info);
extern int slag2_(real * a, integer * lda, real * b, integer * ldb,
		  real * safmin, real * scale1, real * scale2, real * wr1,
		  real * wr2, real * wi);
extern int slags2_(logical * upper, real * a1, real * a2, real * a3,
		   real * b1, real * b2, real * b3, real * csu, real * snu,
		   real * csv, real * snv, real * csq, real * snq);
extern int slagtf_(integer * n, real * a, real * lambda, real * b, real * c__,
		   real * tol, real * d__, integer * in, integer * info);
extern int slagtm_(char *trans, integer * n, integer * nrhs, real * alpha,
		   real * dl, real * d__, real * du, real * x, integer * ldx,
		   real * beta, real * b, integer * ldb, ftnlen trans_len);
extern int slagts_(integer * job, integer * n, real * a, real * b, real * c__,
		   real * d__, integer * in, real * y, real * tol,
		   integer * info);
extern int slagv2_(real * a, integer * lda, real * b, integer * ldb,
		   real * alphar, real * alphai, real * beta, real * csl,
		   real * snl, real * csr, real * snr);
extern int slahqr_(logical * wantt, logical * wantz, integer * n,
		   integer * ilo, integer * ihi, real * h__, integer * ldh,
		   real * wr, real * wi, integer * iloz, integer * ihiz,
		   real * z__, integer * ldz, integer * info);
extern int slahrd_(integer * n, integer * k, integer * nb, real * a,
		   integer * lda, real * tau, real * t, integer * ldt,
		   real * y, integer * ldy);
extern int slaic1_(integer * job, integer * j, real * x, real * sest,
		   real * w, real * gamma, real * sestpr, real * s,
		   real * c__);
extern int slaln2_(logical * ltrans, integer * na, integer * nw, real * smin,
		   real * ca, real * a, integer * lda, real * d1, real * d2,
		   real * b, integer * ldb, real * wr, real * wi, real * x,
		   integer * ldx, real * scale, real * xnorm, integer * info);
extern int slals0_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * nrhs, real * b, integer * ldb,
		   real * bx, integer * ldbx, integer * perm,
		   integer * givptr, integer * givcol, integer * ldgcol,
		   real * givnum, integer * ldgnum, real * poles, real * difl,
		   real * difr, real * z__, integer * k, real * c__, real * s,
		   real * work, integer * info);
extern int slalsa_(integer * icompq, integer * smlsiz, integer * n,
		   integer * nrhs, real * b, integer * ldb, real * bx,
		   integer * ldbx, real * u, integer * ldu, real * vt,
		   integer * k, real * difl, real * difr, real * z__,
		   real * poles, integer * givptr, integer * givcol,
		   integer * ldgcol, integer * perm, real * givnum,
		   real * c__, real * s, real * work, integer * iwork,
		   integer * info);
extern int slalsd_(char *uplo, integer * smlsiz, integer * n, integer * nrhs,
		   real * d__, real * e, real * b, integer * ldb,
		   real * rcond, integer * rank, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern E_f slamch_(char *cmach, ftnlen cmach_len);
extern int slamc1_(integer * beta, integer * t, logical * rnd,
		   logical * ieee1);
extern int slamc2_(integer * beta, integer * t, logical * rnd, real * eps,
		   integer * emin, real * rmin, integer * emax, real * rmax);
extern E_f slamc3_(real * a, real * b);
extern int slamc4_(integer * emin, real * start, integer * base);
extern int slamc5_(integer * beta, integer * p, integer * emin,
		   logical * ieee, integer * emax, real * rmax);
extern int slamrg_(integer * n1, integer * n2, real * a, integer * strd1,
		   integer * strd2, integer * index);
extern E_f slangb_(char *norm, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, real * work, ftnlen norm_len);
extern E_f slange_(char *norm, integer * m, integer * n, real * a,
		   integer * lda, real * work, ftnlen norm_len);
extern E_f slangt_(char *norm, integer * n, real * dl, real * d__, real * du,
		   ftnlen norm_len);
extern E_f slanhs_(char *norm, integer * n, real * a, integer * lda,
		   real * work, ftnlen norm_len);
extern E_f slansb_(char *norm, char *uplo, integer * n, integer * k,
		   real * ab, integer * ldab, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f slansp_(char *norm, char *uplo, integer * n, real * ap,
		   real * work, ftnlen norm_len, ftnlen uplo_len);
extern E_f slanst_(char *norm, integer * n, real * d__, real * e,
		   ftnlen norm_len);
extern E_f slansy_(char *norm, char *uplo, integer * n, real * a,
		   integer * lda, real * work, ftnlen norm_len,
		   ftnlen uplo_len);
extern E_f slantb_(char *norm, char *uplo, char *diag, integer * n,
		   integer * k, real * ab, integer * ldab, real * work,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern E_f slantp_(char *norm, char *uplo, char *diag, integer * n, real * ap,
		   real * work, ftnlen norm_len, ftnlen uplo_len,
		   ftnlen diag_len);
extern E_f slantr_(char *norm, char *uplo, char *diag, integer * m,
		   integer * n, real * a, integer * lda, real * work,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int slanv2_(real * a, real * b, real * c__, real * d__, real * rt1r,
		   real * rt1i, real * rt2r, real * rt2i, real * cs,
		   real * sn);
extern int slapll_(integer * n, real * x, integer * incx, real * y,
		   integer * incy, real * ssmin);
extern int slapmt_(logical * forwrd, integer * m, integer * n, real * x,
		   integer * ldx, integer * k);
extern E_f slapy2_(real * x, real * y);
extern E_f slapy3_(real * x, real * y, real * z__);
extern int slaqgb_(integer * m, integer * n, integer * kl, integer * ku,
		   real * ab, integer * ldab, real * r__, real * c__,
		   real * rowcnd, real * colcnd, real * amax, char *equed,
		   ftnlen equed_len);
extern int slaqge_(integer * m, integer * n, real * a, integer * lda,
		   real * r__, real * c__, real * rowcnd, real * colcnd,
		   real * amax, char *equed, ftnlen equed_len);
extern int slaqp2_(integer * m, integer * n, integer * offset, real * a,
		   integer * lda, integer * jpvt, real * tau, real * vn1,
		   real * vn2, real * work);
extern int slaqps_(integer * m, integer * n, integer * offset, integer * nb,
		   integer * kb, real * a, integer * lda, integer * jpvt,
		   real * tau, real * vn1, real * vn2, real * auxv, real * f,
		   integer * ldf);
extern int slaqsb_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, real * s, real * scond, real * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int slaqsp_(char *uplo, integer * n, real * ap, real * s, real * scond,
		   real * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int slaqsy_(char *uplo, integer * n, real * a, integer * lda, real * s,
		   real * scond, real * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int slaqtr_(logical * ltran, logical * lreal, integer * n, real * t,
		   integer * ldt, real * b, real * w, real * scale, real * x,
		   real * work, integer * info);
extern int slar1v_(integer * n, integer * b1, integer * bn, real * sigma,
		   real * d__, real * l, real * ld, real * lld, real * gersch,
		   real * z__, real * ztz, real * mingma, integer * r__,
		   integer * isuppz, real * work);
extern int slar2v_(integer * n, real * x, real * y, real * z__,
		   integer * incx, real * c__, real * s, integer * incc);
extern int slarf_(char *side, integer * m, integer * n, real * v,
		  integer * incv, real * tau, real * c__, integer * ldc,
		  real * work, ftnlen side_len);
extern int slarfb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, real * v,
		   integer * ldv, real * t, integer * ldt, real * c__,
		   integer * ldc, real * work, integer * ldwork,
		   ftnlen side_len, ftnlen trans_len, ftnlen direct_len,
		   ftnlen storev_len);
extern int slarfg_(integer * n, real * alpha, real * x, integer * incx,
		   real * tau);
extern int slarft_(char *direct, char *storev, integer * n, integer * k,
		   real * v, integer * ldv, real * tau, real * t,
		   integer * ldt, ftnlen direct_len, ftnlen storev_len);
extern int slarfx_(char *side, integer * m, integer * n, real * v, real * tau,
		   real * c__, integer * ldc, real * work, ftnlen side_len);
extern int slargv_(integer * n, real * x, integer * incx, real * y,
		   integer * incy, real * c__, integer * incc);
extern int slarnv_(integer * idist, integer * iseed, integer * n, real * x);
extern int slarrb_(integer * n, real * d__, real * l, real * ld, real * lld,
		   integer * ifirst, integer * ilast, real * sigma,
		   real * reltol, real * w, real * wgap, real * werr,
		   real * work, integer * iwork, integer * info);
extern int slarre_(integer * n, real * d__, real * e, real * tol,
		   integer * nsplit, integer * isplit, integer * m, real * w,
		   real * woff, real * gersch, real * work, integer * info);
extern int slarrf_(integer * n, real * d__, real * l, real * ld, real * lld,
		   integer * ifirst, integer * ilast, real * w, real * dplus,
		   real * lplus, real * work, integer * iwork,
		   integer * info);
extern int slarrv_(integer * n, real * d__, real * l, integer * isplit,
		   integer * m, real * w, integer * iblock, real * gersch,
		   real * tol, real * z__, integer * ldz, integer * isuppz,
		   real * work, integer * iwork, integer * info);
extern int slartg_(real * f, real * g, real * cs, real * sn, real * r__);
extern int slartv_(integer * n, real * x, integer * incx, real * y,
		   integer * incy, real * c__, real * s, integer * incc);
extern int slaruv_(integer * iseed, integer * n, real * x);
extern int slarz_(char *side, integer * m, integer * n, integer * l, real * v,
		  integer * incv, real * tau, real * c__, integer * ldc,
		  real * work, ftnlen side_len);
extern int slarzb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, integer * l,
		   real * v, integer * ldv, real * t, integer * ldt,
		   real * c__, integer * ldc, real * work, integer * ldwork,
		   ftnlen side_len, ftnlen trans_len, ftnlen direct_len,
		   ftnlen storev_len);
extern int slarzt_(char *direct, char *storev, integer * n, integer * k,
		   real * v, integer * ldv, real * tau, real * t,
		   integer * ldt, ftnlen direct_len, ftnlen storev_len);
extern int slas2_(real * f, real * g, real * h__, real * ssmin, real * ssmax);
extern int slascl_(char *type__, integer * kl, integer * ku, real * cfrom,
		   real * cto, integer * m, integer * n, real * a,
		   integer * lda, integer * info, ftnlen type_len);
extern int slasd0_(integer * n, integer * sqre, real * d__, real * e,
		   real * u, integer * ldu, real * vt, integer * ldvt,
		   integer * smlsiz, integer * iwork, real * work,
		   integer * info);
extern int slasd1_(integer * nl, integer * nr, integer * sqre, real * d__,
		   real * alpha, real * beta, real * u, integer * ldu,
		   real * vt, integer * ldvt, integer * idxq, integer * iwork,
		   real * work, integer * info);
extern int slasd2_(integer * nl, integer * nr, integer * sqre, integer * k,
		   real * d__, real * z__, real * alpha, real * beta,
		   real * u, integer * ldu, real * vt, integer * ldvt,
		   real * dsigma, real * u2, integer * ldu2, real * vt2,
		   integer * ldvt2, integer * idxp, integer * idx,
		   integer * idxc, integer * idxq, integer * coltyp,
		   integer * info);
extern int slasd3_(integer * nl, integer * nr, integer * sqre, integer * k,
		   real * d__, real * q, integer * ldq, real * dsigma,
		   real * u, integer * ldu, real * u2, integer * ldu2,
		   real * vt, integer * ldvt, real * vt2, integer * ldvt2,
		   integer * idxc, integer * ctot, real * z__,
		   integer * info);
extern int slasd4_(integer * n, integer * i__, real * d__, real * z__,
		   real * delta, real * rho, real * sigma, real * work,
		   integer * info);
extern int slasd5_(integer * i__, real * d__, real * z__, real * delta,
		   real * rho, real * dsigma, real * work);
extern int slasd6_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, real * d__, real * vf, real * vl,
		   real * alpha, real * beta, integer * idxq, integer * perm,
		   integer * givptr, integer * givcol, integer * ldgcol,
		   real * givnum, integer * ldgnum, real * poles, real * difl,
		   real * difr, real * z__, integer * k, real * c__, real * s,
		   real * work, integer * iwork, integer * info);
extern int slasd7_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * k, real * d__, real * z__,
		   real * zw, real * vf, real * vfw, real * vl, real * vlw,
		   real * alpha, real * beta, real * dsigma, integer * idx,
		   integer * idxp, integer * idxq, integer * perm,
		   integer * givptr, integer * givcol, integer * ldgcol,
		   real * givnum, integer * ldgnum, real * c__, real * s,
		   integer * info);
extern int slasd8_(integer * icompq, integer * k, real * d__, real * z__,
		   real * vf, real * vl, real * difl, real * difr,
		   integer * lddifr, real * dsigma, real * work,
		   integer * info);
extern int slasd9_(integer * icompq, integer * ldu, integer * k, real * d__,
		   real * z__, real * vf, real * vl, real * difl, real * difr,
		   real * dsigma, real * work, integer * info);
extern int slasda_(integer * icompq, integer * smlsiz, integer * n,
		   integer * sqre, real * d__, real * e, real * u,
		   integer * ldu, real * vt, integer * k, real * difl,
		   real * difr, real * z__, real * poles, integer * givptr,
		   integer * givcol, integer * ldgcol, integer * perm,
		   real * givnum, real * c__, real * s, real * work,
		   integer * iwork, integer * info);
extern int slasdq_(char *uplo, integer * sqre, integer * n, integer * ncvt,
		   integer * nru, integer * ncc, real * d__, real * e,
		   real * vt, integer * ldvt, real * u, integer * ldu,
		   real * c__, integer * ldc, real * work, integer * info,
		   ftnlen uplo_len);
extern int slasdt_(integer * n, integer * lvl, integer * nd, integer * inode,
		   integer * ndiml, integer * ndimr, integer * msub);
extern int slaset_(char *uplo, integer * m, integer * n, real * alpha,
		   real * beta, real * a, integer * lda, ftnlen uplo_len);
extern int slasq1_(integer * n, real * d__, real * e, real * work,
		   integer * info);
extern int slasq2_(integer * n, real * z__, integer * info);
extern int slasq3_(integer * i0, integer * n0, real * z__, integer * pp,
		   real * dmin__, real * sigma, real * desig, real * qmax,
		   integer * nfail, integer * iter, integer * ndiv);
extern int slasq4_(integer * i0, integer * n0, real * z__, integer * pp,
		   integer * n0in, real * dmin__, real * dmin1, real * dmin2,
		   real * dn, real * dn1, real * dn2, real * tau,
		   integer * ttype);
extern int slasq5_(integer * i0, integer * n0, real * z__, integer * pp,
		   real * tau, real * dmin__, real * dmin1, real * dmin2,
		   real * dn, real * dnm1, real * dnm2);
extern int slasq6_(integer * i0, integer * n0, real * z__, integer * pp,
		   real * dmin__, real * dmin1, real * dmin2, real * dn,
		   real * dnm1, real * dnm2);
extern int slasr_(char *side, char *pivot, char *direct, integer * m,
		  integer * n, real * c__, real * s, real * a, integer * lda,
		  ftnlen side_len, ftnlen pivot_len, ftnlen direct_len);
extern int slasrt_(char *id, integer * n, real * d__, integer * info,
		   ftnlen id_len);
extern int slassq_(integer * n, real * x, integer * incx, real * scale,
		   real * sumsq);
extern int slasv2_(real * f, real * g, real * h__, real * ssmin, real * ssmax,
		   real * snr, real * csr, real * snl, real * csl);
extern int slaswp_(integer * n, real * a, integer * lda, integer * k1,
		   integer * k2, integer * ipiv, integer * incx);
extern int slasy2_(logical * ltranl, logical * ltranr, integer * isgn,
		   integer * n1, integer * n2, real * tl, integer * ldtl,
		   real * tr, integer * ldtr, real * b, integer * ldb,
		   real * scale, real * x, integer * ldx, real * xnorm,
		   integer * info);
extern int slasyf_(char *uplo, integer * n, integer * nb, integer * kb,
		   real * a, integer * lda, integer * ipiv, real * w,
		   integer * ldw, integer * info, ftnlen uplo_len);
extern int slatbs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, integer * kd, real * ab, integer * ldab,
		   real * x, real * scale, real * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int slatdf_(integer * ijob, integer * n, real * z__, integer * ldz,
		   real * rhs, real * rdsum, real * rdscal, integer * ipiv,
		   integer * jpiv);
extern int slatps_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, real * ap, real * x, real * scale,
		   real * cnorm, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len, ftnlen normin_len);
extern int slatrd_(char *uplo, integer * n, integer * nb, real * a,
		   integer * lda, real * e, real * tau, real * w,
		   integer * ldw, ftnlen uplo_len);
extern int slatrs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, real * a, integer * lda, real * x,
		   real * scale, real * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int slatrz_(integer * m, integer * n, integer * l, real * a,
		   integer * lda, real * tau, real * work);
extern int slatzm_(char *side, integer * m, integer * n, real * v,
		   integer * incv, real * tau, real * c1, real * c2,
		   integer * ldc, real * work, ftnlen side_len);
extern int slauu2_(char *uplo, integer * n, real * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int slauum_(char *uplo, integer * n, real * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int sopgtr_(char *uplo, integer * n, real * ap, real * tau, real * q,
		   integer * ldq, real * work, integer * info,
		   ftnlen uplo_len);
extern int sopmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, real * ap, real * tau, real * c__,
		   integer * ldc, real * work, integer * info,
		   ftnlen side_len, ftnlen uplo_len, ftnlen trans_len);
extern int sorg2l_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * info);
extern int sorg2r_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * info);
extern int sorgbr_(char *vect, integer * m, integer * n, integer * k,
		   real * a, integer * lda, real * tau, real * work,
		   integer * lwork, integer * info, ftnlen vect_len);
extern int sorghr_(integer * n, integer * ilo, integer * ihi, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sorgl2_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * info);
extern int sorglq_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sorgql_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sorgqr_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sorgr2_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * info);
extern int sorgrq_(integer * m, integer * n, integer * k, real * a,
		   integer * lda, real * tau, real * work, integer * lwork,
		   integer * info);
extern int sorgtr_(char *uplo, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info,
		   ftnlen uplo_len);
extern int sorm2l_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int sorm2r_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int sormbr_(char *vect, char *side, char *trans, integer * m,
		   integer * n, integer * k, real * a, integer * lda,
		   real * tau, real * c__, integer * ldc, real * work,
		   integer * lwork, integer * info, ftnlen vect_len,
		   ftnlen side_len, ftnlen trans_len);
extern int sormhr_(char *side, char *trans, integer * m, integer * n,
		   integer * ilo, integer * ihi, real * a, integer * lda,
		   real * tau, real * c__, integer * ldc, real * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int sorml2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int sormlq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int sormql_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int sormqr_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int sormr2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int sormr3_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, real * a, integer * lda,
		   real * tau, real * c__, integer * ldc, real * work,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int sormrq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int sormrz_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, real * a, integer * lda,
		   real * tau, real * c__, integer * ldc, real * work,
		   integer * lwork, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int sormtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, real * a, integer * lda, real * tau,
		   real * c__, integer * ldc, real * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen uplo_len,
		   ftnlen trans_len);
extern int spbcon_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, real * anorm, real * rcond, real * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int spbequ_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, real * s, real * scond, real * amax,
		   integer * info, ftnlen uplo_len);
extern int spbrfs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   real * ab, integer * ldab, real * afb, integer * ldafb,
		   real * b, integer * ldb, real * x, integer * ldx,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int spbstf_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int spbsv_(char *uplo, integer * n, integer * kd, integer * nrhs,
		  real * ab, integer * ldab, real * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int spbsvx_(char *fact, char *uplo, integer * n, integer * kd,
		   integer * nrhs, real * ab, integer * ldab, real * afb,
		   integer * ldafb, char *equed, real * s, real * b,
		   integer * ldb, real * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len,
		   ftnlen equed_len);
extern int spbtf2_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int spbtrf_(char *uplo, integer * n, integer * kd, real * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int spbtrs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   real * ab, integer * ldab, real * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int spocon_(char *uplo, integer * n, real * a, integer * lda,
		   real * anorm, real * rcond, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int spoequ_(integer * n, real * a, integer * lda, real * s,
		   real * scond, real * amax, integer * info);
extern int sporfs_(char *uplo, integer * n, integer * nrhs, real * a,
		   integer * lda, real * af, integer * ldaf, real * b,
		   integer * ldb, real * x, integer * ldx, real * ferr,
		   real * berr, real * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int sposv_(char *uplo, integer * n, integer * nrhs, real * a,
		  integer * lda, real * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int sposvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   real * a, integer * lda, real * af, integer * ldaf,
		   char *equed, real * s, real * b, integer * ldb, real * x,
		   integer * ldx, real * rcond, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen fact_len, ftnlen uplo_len, ftnlen equed_len);
extern int spotf2_(char *uplo, integer * n, real * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int spotrf_(char *uplo, integer * n, real * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int spotri_(char *uplo, integer * n, real * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int spotrs_(char *uplo, integer * n, integer * nrhs, real * a,
		   integer * lda, real * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int sppcon_(char *uplo, integer * n, real * ap, real * anorm,
		   real * rcond, real * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int sppequ_(char *uplo, integer * n, real * ap, real * s, real * scond,
		   real * amax, integer * info, ftnlen uplo_len);
extern int spprfs_(char *uplo, integer * n, integer * nrhs, real * ap,
		   real * afp, real * b, integer * ldb, real * x,
		   integer * ldx, real * ferr, real * berr, real * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int sppsv_(char *uplo, integer * n, integer * nrhs, real * ap,
		  real * b, integer * ldb, integer * info, ftnlen uplo_len);
extern int sppsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   real * ap, real * afp, char *equed, real * s, real * b,
		   integer * ldb, real * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len,
		   ftnlen equed_len);
extern int spptrf_(char *uplo, integer * n, real * ap, integer * info,
		   ftnlen uplo_len);
extern int spptri_(char *uplo, integer * n, real * ap, integer * info,
		   ftnlen uplo_len);
extern int spptrs_(char *uplo, integer * n, integer * nrhs, real * ap,
		   real * b, integer * ldb, integer * info, ftnlen uplo_len);
extern int sptcon_(integer * n, real * d__, real * e, real * anorm,
		   real * rcond, real * work, integer * info);
extern int spteqr_(char *compz, integer * n, real * d__, real * e, real * z__,
		   integer * ldz, real * work, integer * info,
		   ftnlen compz_len);
extern int sptrfs_(integer * n, integer * nrhs, real * d__, real * e,
		   real * df, real * ef, real * b, integer * ldb, real * x,
		   integer * ldx, real * ferr, real * berr, real * work,
		   integer * info);
extern int sptsv_(integer * n, integer * nrhs, real * d__, real * e, real * b,
		  integer * ldb, integer * info);
extern int sptsvx_(char *fact, integer * n, integer * nrhs, real * d__,
		   real * e, real * df, real * ef, real * b, integer * ldb,
		   real * x, integer * ldx, real * rcond, real * ferr,
		   real * berr, real * work, integer * info, ftnlen fact_len);
extern int spttrf_(integer * n, real * d__, real * e, integer * info);
extern int spttrs_(integer * n, integer * nrhs, real * d__, real * e,
		   real * b, integer * ldb, integer * info);
extern int sptts2_(integer * n, integer * nrhs, real * d__, real * e,
		   real * b, integer * ldb);
extern int srscl_(integer * n, real * sa, real * sx, integer * incx);
extern int ssbev_(char *jobz, char *uplo, integer * n, integer * kd,
		  real * ab, integer * ldab, real * w, real * z__,
		  integer * ldz, real * work, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int ssbevd_(char *jobz, char *uplo, integer * n, integer * kd,
		   real * ab, integer * ldab, real * w, real * z__,
		   integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int ssbevx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * kd, real * ab, integer * ldab, real * q,
		   integer * ldq, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   real * z__, integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int ssbgst_(char *vect, char *uplo, integer * n, integer * ka,
		   integer * kb, real * ab, integer * ldab, real * bb,
		   integer * ldbb, real * x, integer * ldx, real * work,
		   integer * info, ftnlen vect_len, ftnlen uplo_len);
extern int ssbgv_(char *jobz, char *uplo, integer * n, integer * ka,
		  integer * kb, real * ab, integer * ldab, real * bb,
		  integer * ldbb, real * w, real * z__, integer * ldz,
		  real * work, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int ssbgvd_(char *jobz, char *uplo, integer * n, integer * ka,
		   integer * kb, real * ab, integer * ldab, real * bb,
		   integer * ldbb, real * w, real * z__, integer * ldz,
		   real * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int ssbgvx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * ka, integer * kb, real * ab, integer * ldab,
		   real * bb, integer * ldbb, real * q, integer * ldq,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, real * z__,
		   integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int ssbtrd_(char *vect, char *uplo, integer * n, integer * kd,
		   real * ab, integer * ldab, real * d__, real * e, real * q,
		   integer * ldq, real * work, integer * info,
		   ftnlen vect_len, ftnlen uplo_len);
extern int sspcon_(char *uplo, integer * n, real * ap, integer * ipiv,
		   real * anorm, real * rcond, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int sspev_(char *jobz, char *uplo, integer * n, real * ap, real * w,
		  real * z__, integer * ldz, real * work, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int sspevd_(char *jobz, char *uplo, integer * n, real * ap, real * w,
		   real * z__, integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int sspevx_(char *jobz, char *range, char *uplo, integer * n,
		   real * ap, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   real * z__, integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int sspgst_(integer * itype, char *uplo, integer * n, real * ap,
		   real * bp, integer * info, ftnlen uplo_len);
extern int sspgv_(integer * itype, char *jobz, char *uplo, integer * n,
		  real * ap, real * bp, real * w, real * z__, integer * ldz,
		  real * work, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int sspgvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   real * ap, real * bp, real * w, real * z__, integer * ldz,
		   real * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int sspgvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, real * ap, real * bp, real * vl, real * vu,
		   integer * il, integer * iu, real * abstol, integer * m,
		   real * w, real * z__, integer * ldz, real * work,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int ssprfs_(char *uplo, integer * n, integer * nrhs, real * ap,
		   real * afp, integer * ipiv, real * b, integer * ldb,
		   real * x, integer * ldx, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen uplo_len);
extern int sspsv_(char *uplo, integer * n, integer * nrhs, real * ap,
		  integer * ipiv, real * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int sspsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   real * ap, real * afp, integer * ipiv, real * b,
		   integer * ldb, real * x, integer * ldx, real * rcond,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int ssptrd_(char *uplo, integer * n, real * ap, real * d__, real * e,
		   real * tau, integer * info, ftnlen uplo_len);
extern int ssptrf_(char *uplo, integer * n, real * ap, integer * ipiv,
		   integer * info, ftnlen uplo_len);
extern int ssptri_(char *uplo, integer * n, real * ap, integer * ipiv,
		   real * work, integer * info, ftnlen uplo_len);
extern int ssptrs_(char *uplo, integer * n, integer * nrhs, real * ap,
		   integer * ipiv, real * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int sstebz_(char *range, char *order, integer * n, real * vl,
		   real * vu, integer * il, integer * iu, real * abstol,
		   real * d__, real * e, integer * m, integer * nsplit,
		   real * w, integer * iblock, integer * isplit, real * work,
		   integer * iwork, integer * info, ftnlen range_len,
		   ftnlen order_len);
extern int sstedc_(char *compz, integer * n, real * d__, real * e, real * z__,
		   integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen compz_len);
extern int sstegr_(char *jobz, char *range, integer * n, real * d__, real * e,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, real * z__,
		   integer * ldz, integer * isuppz, real * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int sstein_(integer * n, real * d__, real * e, integer * m, real * w,
		   integer * iblock, integer * isplit, real * z__,
		   integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info);
extern int ssteqr_(char *compz, integer * n, real * d__, real * e, real * z__,
		   integer * ldz, real * work, integer * info,
		   ftnlen compz_len);
extern int ssterf_(integer * n, real * d__, real * e, integer * info);
extern int sstev_(char *jobz, integer * n, real * d__, real * e, real * z__,
		  integer * ldz, real * work, integer * info,
		  ftnlen jobz_len);
extern int sstevd_(char *jobz, integer * n, real * d__, real * e, real * z__,
		   integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len);
extern int sstevr_(char *jobz, char *range, integer * n, real * d__, real * e,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, real * z__,
		   integer * ldz, integer * isuppz, real * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int sstevx_(char *jobz, char *range, integer * n, real * d__, real * e,
		   real * vl, real * vu, integer * il, integer * iu,
		   real * abstol, integer * m, real * w, real * z__,
		   integer * ldz, real * work, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len);
extern int ssycon_(char *uplo, integer * n, real * a, integer * lda,
		   integer * ipiv, real * anorm, real * rcond, real * work,
		   integer * iwork, integer * info, ftnlen uplo_len);
extern int ssyev_(char *jobz, char *uplo, integer * n, real * a,
		  integer * lda, real * w, real * work, integer * lwork,
		  integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int ssyevd_(char *jobz, char *uplo, integer * n, real * a,
		   integer * lda, real * w, real * work, integer * lwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int ssyevr_(char *jobz, char *range, char *uplo, integer * n, real * a,
		   integer * lda, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   real * z__, integer * ldz, integer * isuppz, real * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int ssyevx_(char *jobz, char *range, char *uplo, integer * n, real * a,
		   integer * lda, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   real * z__, integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int ssygs2_(integer * itype, char *uplo, integer * n, real * a,
		   integer * lda, real * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int ssygst_(integer * itype, char *uplo, integer * n, real * a,
		   integer * lda, real * b, integer * ldb, integer * info,
		   ftnlen uplo_len);
extern int ssygv_(integer * itype, char *jobz, char *uplo, integer * n,
		  real * a, integer * lda, real * b, integer * ldb, real * w,
		  real * work, integer * lwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int ssygvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   real * a, integer * lda, real * b, integer * ldb, real * w,
		   real * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen jobz_len,
		   ftnlen uplo_len);
extern int ssygvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, real * a, integer * lda, real * b,
		   integer * ldb, real * vl, real * vu, integer * il,
		   integer * iu, real * abstol, integer * m, real * w,
		   real * z__, integer * ldz, real * work, integer * lwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int ssyrfs_(char *uplo, integer * n, integer * nrhs, real * a,
		   integer * lda, real * af, integer * ldaf, integer * ipiv,
		   real * b, integer * ldb, real * x, integer * ldx,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern int ssysv_(char *uplo, integer * n, integer * nrhs, real * a,
		  integer * lda, integer * ipiv, real * b, integer * ldb,
		  real * work, integer * lwork, integer * info,
		  ftnlen uplo_len);
extern int ssysvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   real * a, integer * lda, real * af, integer * ldaf,
		   integer * ipiv, real * b, integer * ldb, real * x,
		   integer * ldx, real * rcond, real * ferr, real * berr,
		   real * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int ssytd2_(char *uplo, integer * n, real * a, integer * lda,
		   real * d__, real * e, real * tau, integer * info,
		   ftnlen uplo_len);
extern int ssytf2_(char *uplo, integer * n, real * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int ssytrd_(char *uplo, integer * n, real * a, integer * lda,
		   real * d__, real * e, real * tau, real * work,
		   integer * lwork, integer * info, ftnlen uplo_len);
extern int ssytrf_(char *uplo, integer * n, real * a, integer * lda,
		   integer * ipiv, real * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int ssytri_(char *uplo, integer * n, real * a, integer * lda,
		   integer * ipiv, real * work, integer * info,
		   ftnlen uplo_len);
extern int ssytrs_(char *uplo, integer * n, integer * nrhs, real * a,
		   integer * lda, integer * ipiv, real * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int stbcon_(char *norm, char *uplo, char *diag, integer * n,
		   integer * kd, real * ab, integer * ldab, real * rcond,
		   real * work, integer * iwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int stbrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, real * ab, integer * ldab,
		   real * b, integer * ldb, real * x, integer * ldx,
		   real * ferr, real * berr, real * work, integer * iwork,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int stbtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, real * ab, integer * ldab,
		   real * b, integer * ldb, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int stgevc_(char *side, char *howmny, logical * select, integer * n,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * vl, integer * ldvl, real * vr, integer * ldvr,
		   integer * mm, integer * m, real * work, integer * info,
		   ftnlen side_len, ftnlen howmny_len);
extern int stgex2_(logical * wantq, logical * wantz, integer * n, real * a,
		   integer * lda, real * b, integer * ldb, real * q,
		   integer * ldq, real * z__, integer * ldz, integer * j1,
		   integer * n1, integer * n2, real * work, integer * lwork,
		   integer * info);
extern int stgexc_(logical * wantq, logical * wantz, integer * n, real * a,
		   integer * lda, real * b, integer * ldb, real * q,
		   integer * ldq, real * z__, integer * ldz, integer * ifst,
		   integer * ilst, real * work, integer * lwork,
		   integer * info);
extern int stgsen_(integer * ijob, logical * wantq, logical * wantz,
		   logical * select, integer * n, real * a, integer * lda,
		   real * b, integer * ldb, real * alphar, real * alphai,
		   real * beta, real * q, integer * ldq, real * z__,
		   integer * ldz, integer * m, real * pl, real * pr,
		   real * dif, real * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info);
extern int stgsja_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, integer * k, integer * l,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * tola, real * tolb, real * alpha, real * beta,
		   real * u, integer * ldu, real * v, integer * ldv, real * q,
		   integer * ldq, real * work, integer * ncycle,
		   integer * info, ftnlen jobu_len, ftnlen jobv_len,
		   ftnlen jobq_len);
extern int stgsna_(char *job, char *howmny, logical * select, integer * n,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * vl, integer * ldvl, real * vr, integer * ldvr,
		   real * s, real * dif, integer * mm, integer * m,
		   real * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen job_len, ftnlen howmny_len);
extern int stgsy2_(char *trans, integer * ijob, integer * m, integer * n,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * c__, integer * ldc, real * d__, integer * ldd,
		   real * e, integer * lde, real * f, integer * ldf,
		   real * scale, real * rdsum, real * rdscal, integer * iwork,
		   integer * pq, integer * info, ftnlen trans_len);
extern int stgsyl_(char *trans, integer * ijob, integer * m, integer * n,
		   real * a, integer * lda, real * b, integer * ldb,
		   real * c__, integer * ldc, real * d__, integer * ldd,
		   real * e, integer * lde, real * f, integer * ldf,
		   real * scale, real * dif, real * work, integer * lwork,
		   integer * iwork, integer * info, ftnlen trans_len);
extern int stpcon_(char *norm, char *uplo, char *diag, integer * n, real * ap,
		   real * rcond, real * work, integer * iwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int stprfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, real * ap, real * b, integer * ldb,
		   real * x, integer * ldx, real * ferr, real * berr,
		   real * work, integer * iwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int stptri_(char *uplo, char *diag, integer * n, real * ap,
		   integer * info, ftnlen uplo_len, ftnlen diag_len);
extern int stptrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, real * ap, real * b, integer * ldb,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int strcon_(char *norm, char *uplo, char *diag, integer * n, real * a,
		   integer * lda, real * rcond, real * work, integer * iwork,
		   integer * info, ftnlen norm_len, ftnlen uplo_len,
		   ftnlen diag_len);
extern int strevc_(char *side, char *howmny, logical * select, integer * n,
		   real * t, integer * ldt, real * vl, integer * ldvl,
		   real * vr, integer * ldvr, integer * mm, integer * m,
		   real * work, integer * info, ftnlen side_len,
		   ftnlen howmny_len);
extern int strexc_(char *compq, integer * n, real * t, integer * ldt,
		   real * q, integer * ldq, integer * ifst, integer * ilst,
		   real * work, integer * info, ftnlen compq_len);
extern int strrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, real * a, integer * lda, real * b,
		   integer * ldb, real * x, integer * ldx, real * ferr,
		   real * berr, real * work, integer * iwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int strsen_(char *job, char *compq, logical * select, integer * n,
		   real * t, integer * ldt, real * q, integer * ldq,
		   real * wr, real * wi, integer * m, real * s, real * sep,
		   real * work, integer * lwork, integer * iwork,
		   integer * liwork, integer * info, ftnlen job_len,
		   ftnlen compq_len);
extern int strsna_(char *job, char *howmny, logical * select, integer * n,
		   real * t, integer * ldt, real * vl, integer * ldvl,
		   real * vr, integer * ldvr, real * s, real * sep,
		   integer * mm, integer * m, real * work, integer * ldwork,
		   integer * iwork, integer * info, ftnlen job_len,
		   ftnlen howmny_len);
extern int strsyl_(char *trana, char *tranb, integer * isgn, integer * m,
		   integer * n, real * a, integer * lda, real * b,
		   integer * ldb, real * c__, integer * ldc, real * scale,
		   integer * info, ftnlen trana_len, ftnlen tranb_len);
extern int strti2_(char *uplo, char *diag, integer * n, real * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int strtri_(char *uplo, char *diag, integer * n, real * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int strtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, real * a, integer * lda, real * b,
		   integer * ldb, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int stzrqf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, integer * info);
extern int stzrzf_(integer * m, integer * n, real * a, integer * lda,
		   real * tau, real * work, integer * lwork, integer * info);
extern int xerbla_(char *srname, integer * info, ftnlen srname_len);
extern int zbdsqr_(char *uplo, integer * n, integer * ncvt, integer * nru,
		   integer * ncc, doublereal * d__, doublereal * e,
		   doublecomplex * vt, integer * ldvt, doublecomplex * u,
		   integer * ldu, doublecomplex * c__, integer * ldc,
		   doublereal * rwork, integer * info, ftnlen uplo_len);
extern int zdrot_(integer * n, doublecomplex * cx, integer * incx,
		  doublecomplex * cy, integer * incy, doublereal * c__,
		  doublereal * s);
extern int zdrscl_(integer * n, doublereal * sa, doublecomplex * sx,
		   integer * incx);
extern int zgbbrd_(char *vect, integer * m, integer * n, integer * ncc,
		   integer * kl, integer * ku, doublecomplex * ab,
		   integer * ldab, doublereal * d__, doublereal * e,
		   doublecomplex * q, integer * ldq, doublecomplex * pt,
		   integer * ldpt, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen vect_len);
extern int zgbcon_(char *norm, integer * n, integer * kl, integer * ku,
		   doublecomplex * ab, integer * ldab, integer * ipiv,
		   doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen norm_len);
extern int zgbequ_(integer * m, integer * n, integer * kl, integer * ku,
		   doublecomplex * ab, integer * ldab, doublereal * r__,
		   doublereal * c__, doublereal * rowcnd, doublereal * colcnd,
		   doublereal * amax, integer * info);
extern int zgbrfs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, doublecomplex * ab, integer * ldab,
		   doublecomplex * afb, integer * ldafb, integer * ipiv,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen trans_len);
extern int zgbsv_(integer * n, integer * kl, integer * ku, integer * nrhs,
		  doublecomplex * ab, integer * ldab, integer * ipiv,
		  doublecomplex * b, integer * ldb, integer * info);
extern int zgbsvx_(char *fact, char *trans, integer * n, integer * kl,
		   integer * ku, integer * nrhs, doublecomplex * ab,
		   integer * ldab, doublecomplex * afb, integer * ldafb,
		   integer * ipiv, char *equed, doublereal * r__,
		   doublereal * c__, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen trans_len, ftnlen equed_len);
extern int zgbtf2_(integer * m, integer * n, integer * kl, integer * ku,
		   doublecomplex * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int zgbtrf_(integer * m, integer * n, integer * kl, integer * ku,
		   doublecomplex * ab, integer * ldab, integer * ipiv,
		   integer * info);
extern int zgbtrs_(char *trans, integer * n, integer * kl, integer * ku,
		   integer * nrhs, doublecomplex * ab, integer * ldab,
		   integer * ipiv, doublecomplex * b, integer * ldb,
		   integer * info, ftnlen trans_len);
extern int zgebak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, doublereal * scale, integer * m,
		   doublecomplex * v, integer * ldv, integer * info,
		   ftnlen job_len, ftnlen side_len);
extern int zgebal_(char *job, integer * n, doublecomplex * a, integer * lda,
		   integer * ilo, integer * ihi, doublereal * scale,
		   integer * info, ftnlen job_len);
extern int zgebd2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublereal * d__, doublereal * e, doublecomplex * tauq,
		   doublecomplex * taup, doublecomplex * work,
		   integer * info);
extern int zgebrd_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublereal * d__, doublereal * e, doublecomplex * tauq,
		   doublecomplex * taup, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zgecon_(char *norm, integer * n, doublecomplex * a, integer * lda,
		   doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen norm_len);
extern int zgeequ_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublereal * r__, doublereal * c__, doublereal * rowcnd,
		   doublereal * colcnd, doublereal * amax, integer * info);
extern int zgees_(char *jobvs, char *sort, L_fp select, integer * n,
		  doublecomplex * a, integer * lda, integer * sdim,
		  doublecomplex * w, doublecomplex * vs, integer * ldvs,
		  doublecomplex * work, integer * lwork, doublereal * rwork,
		  logical * bwork, integer * info, ftnlen jobvs_len,
		  ftnlen sort_len);
extern int zgeesx_(char *jobvs, char *sort, L_fp select, char *sense,
		   integer * n, doublecomplex * a, integer * lda,
		   integer * sdim, doublecomplex * w, doublecomplex * vs,
		   integer * ldvs, doublereal * rconde, doublereal * rcondv,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   logical * bwork, integer * info, ftnlen jobvs_len,
		   ftnlen sort_len, ftnlen sense_len);
extern int zgeev_(char *jobvl, char *jobvr, integer * n, doublecomplex * a,
		  integer * lda, doublecomplex * w, doublecomplex * vl,
		  integer * ldvl, doublecomplex * vr, integer * ldvr,
		  doublecomplex * work, integer * lwork, doublereal * rwork,
		  integer * info, ftnlen jobvl_len, ftnlen jobvr_len);
extern int zgeevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * w, doublecomplex * vl, integer * ldvl,
		   doublecomplex * vr, integer * ldvr, integer * ilo,
		   integer * ihi, doublereal * scale, doublereal * abnrm,
		   doublereal * rconde, doublereal * rcondv,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * info, ftnlen balanc_len, ftnlen jobvl_len,
		   ftnlen jobvr_len, ftnlen sense_len);
extern int zgegs_(char *jobvsl, char *jobvsr, integer * n, doublecomplex * a,
		  integer * lda, doublecomplex * b, integer * ldb,
		  doublecomplex * alpha, doublecomplex * beta,
		  doublecomplex * vsl, integer * ldvsl, doublecomplex * vsr,
		  integer * ldvsr, doublecomplex * work, integer * lwork,
		  doublereal * rwork, integer * info, ftnlen jobvsl_len,
		  ftnlen jobvsr_len);
extern int zgegv_(char *jobvl, char *jobvr, integer * n, doublecomplex * a,
		  integer * lda, doublecomplex * b, integer * ldb,
		  doublecomplex * alpha, doublecomplex * beta,
		  doublecomplex * vl, integer * ldvl, doublecomplex * vr,
		  integer * ldvr, doublecomplex * work, integer * lwork,
		  doublereal * rwork, integer * info, ftnlen jobvl_len,
		  ftnlen jobvr_len);
extern int zgehd2_(integer * n, integer * ilo, integer * ihi,
		   doublecomplex * a, integer * lda, doublecomplex * tau,
		   doublecomplex * work, integer * info);
extern int zgehrd_(integer * n, integer * ilo, integer * ihi,
		   doublecomplex * a, integer * lda, doublecomplex * tau,
		   doublecomplex * work, integer * lwork, integer * info);
extern int zgelq2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * info);
extern int zgelqf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zgels_(char *trans, integer * m, integer * n, integer * nrhs,
		  doublecomplex * a, integer * lda, doublecomplex * b,
		  integer * ldb, doublecomplex * work, integer * lwork,
		  integer * info, ftnlen trans_len);
extern int zgelsd_(integer * m, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublereal * s, doublereal * rcond,
		   integer * rank, doublecomplex * work, integer * lwork,
		   doublereal * rwork, integer * iwork, integer * info);
extern int zgelss_(integer * m, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublereal * s, doublereal * rcond,
		   integer * rank, doublecomplex * work, integer * lwork,
		   doublereal * rwork, integer * info);
extern int zgelsx_(integer * m, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, integer * jpvt, doublereal * rcond,
		   integer * rank, doublecomplex * work, doublereal * rwork,
		   integer * info);
extern int zgelsy_(integer * m, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, integer * jpvt, doublereal * rcond,
		   integer * rank, doublecomplex * work, integer * lwork,
		   doublereal * rwork, integer * info);
extern int zgeql2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * info);
extern int zgeqlf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zgeqp3_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   integer * jpvt, doublecomplex * tau, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * info);
extern int zgeqpf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   integer * jpvt, doublecomplex * tau, doublecomplex * work,
		   doublereal * rwork, integer * info);
extern int zgeqr2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * info);
extern int zgeqrf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zgerfs_(char *trans, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * af,
		   integer * ldaf, integer * ipiv, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen trans_len);
extern int zgerq2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * info);
extern int zgerqf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zgesc2_(integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * rhs, integer * ipiv, integer * jpiv,
		   doublereal * scale);
extern int zgesdd_(char *jobz, integer * m, integer * n, doublecomplex * a,
		   integer * lda, doublereal * s, doublecomplex * u,
		   integer * ldu, doublecomplex * vt, integer * ldvt,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * iwork, integer * info, ftnlen jobz_len);
extern int zgesv_(integer * n, integer * nrhs, doublecomplex * a,
		  integer * lda, integer * ipiv, doublecomplex * b,
		  integer * ldb, integer * info);
extern int zgesvd_(char *jobu, char *jobvt, integer * m, integer * n,
		   doublecomplex * a, integer * lda, doublereal * s,
		   doublecomplex * u, integer * ldu, doublecomplex * vt,
		   integer * ldvt, doublecomplex * work, integer * lwork,
		   doublereal * rwork, integer * info, ftnlen jobu_len,
		   ftnlen jobvt_len);
extern int zgesvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * af,
		   integer * ldaf, integer * ipiv, char *equed,
		   doublereal * r__, doublereal * c__, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen fact_len, ftnlen trans_len, ftnlen equed_len);
extern int zgetc2_(integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, integer * jpiv, integer * info);
extern int zgetf2_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, integer * info);
extern int zgetrf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, integer * info);
extern int zgetri_(integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zgetrs_(char *trans, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, integer * ipiv,
		   doublecomplex * b, integer * ldb, integer * info,
		   ftnlen trans_len);
extern int zggbak_(char *job, char *side, integer * n, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   integer * m, doublecomplex * v, integer * ldv,
		   integer * info, ftnlen job_len, ftnlen side_len);
extern int zggbal_(char *job, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   doublereal * work, integer * info, ftnlen job_len);
extern int zgges_(char *jobvsl, char *jobvsr, char *sort, L_fp delctg,
		  integer * n, doublecomplex * a, integer * lda,
		  doublecomplex * b, integer * ldb, integer * sdim,
		  doublecomplex * alpha, doublecomplex * beta,
		  doublecomplex * vsl, integer * ldvsl, doublecomplex * vsr,
		  integer * ldvsr, doublecomplex * work, integer * lwork,
		  doublereal * rwork, logical * bwork, integer * info,
		  ftnlen jobvsl_len, ftnlen jobvsr_len, ftnlen sort_len);
extern int zggesx_(char *jobvsl, char *jobvsr, char *sort, L_fp delctg,
		   char *sense, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, integer * sdim,
		   doublecomplex * alpha, doublecomplex * beta,
		   doublecomplex * vsl, integer * ldvsl, doublecomplex * vsr,
		   integer * ldvsr, doublereal * rconde, doublereal * rcondv,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * iwork, integer * liwork, logical * bwork,
		   integer * info, ftnlen jobvsl_len, ftnlen jobvsr_len,
		   ftnlen sort_len, ftnlen sense_len);
extern int zggev_(char *jobvl, char *jobvr, integer * n, doublecomplex * a,
		  integer * lda, doublecomplex * b, integer * ldb,
		  doublecomplex * alpha, doublecomplex * beta,
		  doublecomplex * vl, integer * ldvl, doublecomplex * vr,
		  integer * ldvr, doublecomplex * work, integer * lwork,
		  doublereal * rwork, integer * info, ftnlen jobvl_len,
		  ftnlen jobvr_len);
extern int zggevx_(char *balanc, char *jobvl, char *jobvr, char *sense,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * alpha,
		   doublecomplex * beta, doublecomplex * vl, integer * ldvl,
		   doublecomplex * vr, integer * ldvr, integer * ilo,
		   integer * ihi, doublereal * lscale, doublereal * rscale,
		   doublereal * abnrm, doublereal * bbnrm,
		   doublereal * rconde, doublereal * rcondv,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * iwork, logical * bwork, integer * info,
		   ftnlen balanc_len, ftnlen jobvl_len, ftnlen jobvr_len,
		   ftnlen sense_len);
extern int zggglm_(integer * n, integer * m, integer * p, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   doublecomplex * d__, doublecomplex * x, doublecomplex * y,
		   doublecomplex * work, integer * lwork, integer * info);
extern int zgghrd_(char *compq, char *compz, integer * n, integer * ilo,
		   integer * ihi, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * q,
		   integer * ldq, doublecomplex * z__, integer * ldz,
		   integer * info, ftnlen compq_len, ftnlen compz_len);
extern int zgglse_(integer * m, integer * n, integer * p, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   doublecomplex * c__, doublecomplex * d__,
		   doublecomplex * x, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zggqrf_(integer * n, integer * m, integer * p, doublecomplex * a,
		   integer * lda, doublecomplex * taua, doublecomplex * b,
		   integer * ldb, doublecomplex * taub, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zggrqf_(integer * m, integer * p, integer * n, doublecomplex * a,
		   integer * lda, doublecomplex * taua, doublecomplex * b,
		   integer * ldb, doublecomplex * taub, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zggsvd_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * n, integer * p, integer * k, integer * l,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublereal * alpha, doublereal * beta,
		   doublecomplex * u, integer * ldu, doublecomplex * v,
		   integer * ldv, doublecomplex * q, integer * ldq,
		   doublecomplex * work, doublereal * rwork, integer * iwork,
		   integer * info, ftnlen jobu_len, ftnlen jobv_len,
		   ftnlen jobq_len);
extern int zggsvp_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublereal * tola,
		   doublereal * tolb, integer * k, integer * l,
		   doublecomplex * u, integer * ldu, doublecomplex * v,
		   integer * ldv, doublecomplex * q, integer * ldq,
		   integer * iwork, doublereal * rwork, doublecomplex * tau,
		   doublecomplex * work, integer * info, ftnlen jobu_len,
		   ftnlen jobv_len, ftnlen jobq_len);
extern int zgtcon_(char *norm, integer * n, doublecomplex * dl,
		   doublecomplex * d__, doublecomplex * du,
		   doublecomplex * du2, integer * ipiv, doublereal * anorm,
		   doublereal * rcond, doublecomplex * work, integer * info,
		   ftnlen norm_len);
extern int zgtrfs_(char *trans, integer * n, integer * nrhs,
		   doublecomplex * dl, doublecomplex * d__,
		   doublecomplex * du, doublecomplex * dlf,
		   doublecomplex * df, doublecomplex * duf,
		   doublecomplex * du2, integer * ipiv, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen trans_len);
extern int zgtsv_(integer * n, integer * nrhs, doublecomplex * dl,
		  doublecomplex * d__, doublecomplex * du, doublecomplex * b,
		  integer * ldb, integer * info);
extern int zgtsvx_(char *fact, char *trans, integer * n, integer * nrhs,
		   doublecomplex * dl, doublecomplex * d__,
		   doublecomplex * du, doublecomplex * dlf,
		   doublecomplex * df, doublecomplex * duf,
		   doublecomplex * du2, integer * ipiv, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen fact_len, ftnlen trans_len);
extern int zgttrf_(integer * n, doublecomplex * dl, doublecomplex * d__,
		   doublecomplex * du, doublecomplex * du2, integer * ipiv,
		   integer * info);
extern int zgttrs_(char *trans, integer * n, integer * nrhs,
		   doublecomplex * dl, doublecomplex * d__,
		   doublecomplex * du, doublecomplex * du2, integer * ipiv,
		   doublecomplex * b, integer * ldb, integer * info,
		   ftnlen trans_len);
extern int zgtts2_(integer * itrans, integer * n, integer * nrhs,
		   doublecomplex * dl, doublecomplex * d__,
		   doublecomplex * du, doublecomplex * du2, integer * ipiv,
		   doublecomplex * b, integer * ldb);
extern int zhbev_(char *jobz, char *uplo, integer * n, integer * kd,
		  doublecomplex * ab, integer * ldab, doublereal * w,
		  doublecomplex * z__, integer * ldz, doublecomplex * work,
		  doublereal * rwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int zhbevd_(char *jobz, char *uplo, integer * n, integer * kd,
		   doublecomplex * ab, integer * ldab, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int zhbevx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * kd, doublecomplex * ab, integer * ldab,
		   doublecomplex * q, integer * ldq, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   doublereal * rwork, integer * iwork, integer * ifail,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int zhbgst_(char *vect, char *uplo, integer * n, integer * ka,
		   integer * kb, doublecomplex * ab, integer * ldab,
		   doublecomplex * bb, integer * ldbb, doublecomplex * x,
		   integer * ldx, doublecomplex * work, doublereal * rwork,
		   integer * info, ftnlen vect_len, ftnlen uplo_len);
extern int zhbgv_(char *jobz, char *uplo, integer * n, integer * ka,
		  integer * kb, doublecomplex * ab, integer * ldab,
		  doublecomplex * bb, integer * ldbb, doublereal * w,
		  doublecomplex * z__, integer * ldz, doublecomplex * work,
		  doublereal * rwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int zhbgvd_(char *jobz, char *uplo, integer * n, integer * ka,
		   integer * kb, doublecomplex * ab, integer * ldab,
		   doublecomplex * bb, integer * ldbb, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int zhbgvx_(char *jobz, char *range, char *uplo, integer * n,
		   integer * ka, integer * kb, doublecomplex * ab,
		   integer * ldab, doublecomplex * bb, integer * ldbb,
		   doublecomplex * q, integer * ldq, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   doublereal * rwork, integer * iwork, integer * ifail,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int zhbtrd_(char *vect, char *uplo, integer * n, integer * kd,
		   doublecomplex * ab, integer * ldab, doublereal * d__,
		   doublereal * e, doublecomplex * q, integer * ldq,
		   doublecomplex * work, integer * info, ftnlen vect_len,
		   ftnlen uplo_len);
extern int zhecon_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, integer * info, ftnlen uplo_len);
extern int zheev_(char *jobz, char *uplo, integer * n, doublecomplex * a,
		  integer * lda, doublereal * w, doublecomplex * work,
		  integer * lwork, doublereal * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int zheevd_(char *jobz, char *uplo, integer * n, doublecomplex * a,
		   integer * lda, doublereal * w, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int zheevr_(char *jobz, char *range, char *uplo, integer * n,
		   doublecomplex * a, integer * lda, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublecomplex * z__, integer * ldz, integer * isuppz,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * lrwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len,
		   ftnlen uplo_len);
extern int zheevx_(char *jobz, char *range, char *uplo, integer * n,
		   doublecomplex * a, integer * lda, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int zhegs2_(integer * itype, char *uplo, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zhegst_(integer * itype, char *uplo, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zhegv_(integer * itype, char *jobz, char *uplo, integer * n,
		  doublecomplex * a, integer * lda, doublecomplex * b,
		  integer * ldb, doublereal * w, doublecomplex * work,
		  integer * lwork, doublereal * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int zhegvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublereal * w, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int zhegvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublereal * vl,
		   doublereal * vu, integer * il, integer * iu,
		   doublereal * abstol, integer * m, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int zherfs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, doublecomplex * af, integer * ldaf,
		   integer * ipiv, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len);
extern int zhesv_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		  integer * lda, integer * ipiv, doublecomplex * b,
		  integer * ldb, doublecomplex * work, integer * lwork,
		  integer * info, ftnlen uplo_len);
extern int zhesvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * af,
		   integer * ldaf, integer * ipiv, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int zhetd2_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublereal * d__, doublereal * e, doublecomplex * tau,
		   integer * info, ftnlen uplo_len);
extern int zhetf2_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int zhetrd_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublereal * d__, doublereal * e, doublecomplex * tau,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen uplo_len);
extern int zhetrf_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublecomplex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int zhetri_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublecomplex * work, integer * info,
		   ftnlen uplo_len);
extern int zhetrs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, integer * ipiv, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zhgeqz_(char *job, char *compq, char *compz, integer * n,
		   integer * ilo, integer * ihi, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   doublecomplex * alpha, doublecomplex * beta,
		   doublecomplex * q, integer * ldq, doublecomplex * z__,
		   integer * ldz, doublecomplex * work, integer * lwork,
		   doublereal * rwork, integer * info, ftnlen job_len,
		   ftnlen compq_len, ftnlen compz_len);
extern int zhpcon_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, integer * info, ftnlen uplo_len);
extern int zhpev_(char *jobz, char *uplo, integer * n, doublecomplex * ap,
		  doublereal * w, doublecomplex * z__, integer * ldz,
		  doublecomplex * work, doublereal * rwork, integer * info,
		  ftnlen jobz_len, ftnlen uplo_len);
extern int zhpevd_(char *jobz, char *uplo, integer * n, doublecomplex * ap,
		   doublereal * w, doublecomplex * z__, integer * ldz,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * lrwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen uplo_len);
extern int zhpevx_(char *jobz, char *range, char *uplo, integer * n,
		   doublecomplex * ap, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublecomplex * z__,
		   integer * ldz, doublecomplex * work, doublereal * rwork,
		   integer * iwork, integer * ifail, integer * info,
		   ftnlen jobz_len, ftnlen range_len, ftnlen uplo_len);
extern int zhpgst_(integer * itype, char *uplo, integer * n,
		   doublecomplex * ap, doublecomplex * bp, integer * info,
		   ftnlen uplo_len);
extern int zhpgv_(integer * itype, char *jobz, char *uplo, integer * n,
		  doublecomplex * ap, doublecomplex * bp, doublereal * w,
		  doublecomplex * z__, integer * ldz, doublecomplex * work,
		  doublereal * rwork, integer * info, ftnlen jobz_len,
		  ftnlen uplo_len);
extern int zhpgvd_(integer * itype, char *jobz, char *uplo, integer * n,
		   doublecomplex * ap, doublecomplex * bp, doublereal * w,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen jobz_len, ftnlen uplo_len);
extern int zhpgvx_(integer * itype, char *jobz, char *range, char *uplo,
		   integer * n, doublecomplex * ap, doublecomplex * bp,
		   doublereal * vl, doublereal * vu, integer * il,
		   integer * iu, doublereal * abstol, integer * m,
		   doublereal * w, doublecomplex * z__, integer * ldz,
		   doublecomplex * work, doublereal * rwork, integer * iwork,
		   integer * ifail, integer * info, ftnlen jobz_len,
		   ftnlen range_len, ftnlen uplo_len);
extern int zhprfs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, integer * ipiv,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zhpsv_(char *uplo, integer * n, integer * nrhs, doublecomplex * ap,
		  integer * ipiv, doublecomplex * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int zhpsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, integer * ipiv,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len);
extern int zhptrd_(char *uplo, integer * n, doublecomplex * ap,
		   doublereal * d__, doublereal * e, doublecomplex * tau,
		   integer * info, ftnlen uplo_len);
extern int zhptrf_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int zhptri_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, doublecomplex * work, integer * info,
		   ftnlen uplo_len);
extern int zhptrs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, integer * ipiv, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zhsein_(char *side, char *eigsrc, char *initv, logical * select,
		   integer * n, doublecomplex * h__, integer * ldh,
		   doublecomplex * w, doublecomplex * vl, integer * ldvl,
		   doublecomplex * vr, integer * ldvr, integer * mm,
		   integer * m, doublecomplex * work, doublereal * rwork,
		   integer * ifaill, integer * ifailr, integer * info,
		   ftnlen side_len, ftnlen eigsrc_len, ftnlen initv_len);
extern int zhseqr_(char *job, char *compz, integer * n, integer * ilo,
		   integer * ihi, doublecomplex * h__, integer * ldh,
		   doublecomplex * w, doublecomplex * z__, integer * ldz,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen job_len, ftnlen compz_len);
extern int zlabrd_(integer * m, integer * n, integer * nb, doublecomplex * a,
		   integer * lda, doublereal * d__, doublereal * e,
		   doublecomplex * tauq, doublecomplex * taup,
		   doublecomplex * x, integer * ldx, doublecomplex * y,
		   integer * ldy);
extern int zlacgv_(integer * n, doublecomplex * x, integer * incx);
extern int zlacon_(integer * n, doublecomplex * v, doublecomplex * x,
		   doublereal * est, integer * kase);
extern int zlacp2_(char *uplo, integer * m, integer * n, doublereal * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   ftnlen uplo_len);
extern int zlacpy_(char *uplo, integer * m, integer * n, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   ftnlen uplo_len);
extern int zlacrm_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublereal * b, integer * ldb, doublecomplex * c__,
		   integer * ldc, doublereal * rwork);
extern int zlacrt_(integer * n, doublecomplex * cx, integer * incx,
		   doublecomplex * cy, integer * incy, doublecomplex * c__,
		   doublecomplex * s);
extern Z_f zladiv_(doublecomplex * ret_val, doublecomplex * x,
		   doublecomplex * y);
extern int zlaed0_(integer * qsiz, integer * n, doublereal * d__,
		   doublereal * e, doublecomplex * q, integer * ldq,
		   doublecomplex * qstore, integer * ldqs, doublereal * rwork,
		   integer * iwork, integer * info);
extern int zlaed7_(integer * n, integer * cutpnt, integer * qsiz,
		   integer * tlvls, integer * curlvl, integer * curpbm,
		   doublereal * d__, doublecomplex * q, integer * ldq,
		   doublereal * rho, integer * indxq, doublereal * qstore,
		   integer * qptr, integer * prmptr, integer * perm,
		   integer * givptr, integer * givcol, doublereal * givnum,
		   doublecomplex * work, doublereal * rwork, integer * iwork,
		   integer * info);
extern int zlaed8_(integer * k, integer * n, integer * qsiz,
		   doublecomplex * q, integer * ldq, doublereal * d__,
		   doublereal * rho, integer * cutpnt, doublereal * z__,
		   doublereal * dlamda, doublecomplex * q2, integer * ldq2,
		   doublereal * w, integer * indxp, integer * indx,
		   integer * indxq, integer * perm, integer * givptr,
		   integer * givcol, doublereal * givnum, integer * info);
extern int zlaein_(logical * rightv, logical * noinit, integer * n,
		   doublecomplex * h__, integer * ldh, doublecomplex * w,
		   doublecomplex * v, doublecomplex * b, integer * ldb,
		   doublereal * rwork, doublereal * eps3, doublereal * smlnum,
		   integer * info);
extern int zlaesy_(doublecomplex * a, doublecomplex * b, doublecomplex * c__,
		   doublecomplex * rt1, doublecomplex * rt2,
		   doublecomplex * evscal, doublecomplex * cs1,
		   doublecomplex * sn1);
extern int zlaev2_(doublecomplex * a, doublecomplex * b, doublecomplex * c__,
		   doublereal * rt1, doublereal * rt2, doublereal * cs1,
		   doublecomplex * sn1);
extern int zlags2_(logical * upper, doublereal * a1, doublecomplex * a2,
		   doublereal * a3, doublereal * b1, doublecomplex * b2,
		   doublereal * b3, doublereal * csu, doublecomplex * snu,
		   doublereal * csv, doublecomplex * snv, doublereal * csq,
		   doublecomplex * snq);
extern int zlagtm_(char *trans, integer * n, integer * nrhs,
		   doublereal * alpha, doublecomplex * dl,
		   doublecomplex * d__, doublecomplex * du, doublecomplex * x,
		   integer * ldx, doublereal * beta, doublecomplex * b,
		   integer * ldb, ftnlen trans_len);
extern int zlahef_(char *uplo, integer * n, integer * nb, integer * kb,
		   doublecomplex * a, integer * lda, integer * ipiv,
		   doublecomplex * w, integer * ldw, integer * info,
		   ftnlen uplo_len);
extern int zlahqr_(logical * wantt, logical * wantz, integer * n,
		   integer * ilo, integer * ihi, doublecomplex * h__,
		   integer * ldh, doublecomplex * w, integer * iloz,
		   integer * ihiz, doublecomplex * z__, integer * ldz,
		   integer * info);
extern int zlahrd_(integer * n, integer * k, integer * nb, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * t,
		   integer * ldt, doublecomplex * y, integer * ldy);
extern int zlaic1_(integer * job, integer * j, doublecomplex * x,
		   doublereal * sest, doublecomplex * w,
		   doublecomplex * gamma, doublereal * sestpr,
		   doublecomplex * s, doublecomplex * c__);
extern int zlals0_(integer * icompq, integer * nl, integer * nr,
		   integer * sqre, integer * nrhs, doublecomplex * b,
		   integer * ldb, doublecomplex * bx, integer * ldbx,
		   integer * perm, integer * givptr, integer * givcol,
		   integer * ldgcol, doublereal * givnum, integer * ldgnum,
		   doublereal * poles, doublereal * difl, doublereal * difr,
		   doublereal * z__, integer * k, doublereal * c__,
		   doublereal * s, doublereal * rwork, integer * info);
extern int zlalsa_(integer * icompq, integer * smlsiz, integer * n,
		   integer * nrhs, doublecomplex * b, integer * ldb,
		   doublecomplex * bx, integer * ldbx, doublereal * u,
		   integer * ldu, doublereal * vt, integer * k,
		   doublereal * difl, doublereal * difr, doublereal * z__,
		   doublereal * poles, integer * givptr, integer * givcol,
		   integer * ldgcol, integer * perm, doublereal * givnum,
		   doublereal * c__, doublereal * s, doublereal * rwork,
		   integer * iwork, integer * info);
extern int zlalsd_(char *uplo, integer * smlsiz, integer * n, integer * nrhs,
		   doublereal * d__, doublereal * e, doublecomplex * b,
		   integer * ldb, doublereal * rcond, integer * rank,
		   doublecomplex * work, doublereal * rwork, integer * iwork,
		   integer * info, ftnlen uplo_len);
extern doublereal zlangb_(char *norm, integer * n, integer * kl, integer * ku,
			  doublecomplex * ab, integer * ldab,
			  doublereal * work, ftnlen norm_len);
extern doublereal zlange_(char *norm, integer * m, integer * n,
			  doublecomplex * a, integer * lda, doublereal * work,
			  ftnlen norm_len);
extern doublereal zlangt_(char *norm, integer * n, doublecomplex * dl,
			  doublecomplex * d__, doublecomplex * du,
			  ftnlen norm_len);
extern doublereal zlanhb_(char *norm, char *uplo, integer * n, integer * k,
			  doublecomplex * ab, integer * ldab,
			  doublereal * work, ftnlen norm_len,
			  ftnlen uplo_len);
extern doublereal zlanhe_(char *norm, char *uplo, integer * n,
			  doublecomplex * a, integer * lda, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len);
extern doublereal zlanhp_(char *norm, char *uplo, integer * n,
			  doublecomplex * ap, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len);
extern doublereal zlanhs_(char *norm, integer * n, doublecomplex * a,
			  integer * lda, doublereal * work, ftnlen norm_len);
extern doublereal zlanht_(char *norm, integer * n, doublereal * d__,
			  doublecomplex * e, ftnlen norm_len);
extern doublereal zlansb_(char *norm, char *uplo, integer * n, integer * k,
			  doublecomplex * ab, integer * ldab,
			  doublereal * work, ftnlen norm_len,
			  ftnlen uplo_len);
extern doublereal zlansp_(char *norm, char *uplo, integer * n,
			  doublecomplex * ap, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len);
extern doublereal zlansy_(char *norm, char *uplo, integer * n,
			  doublecomplex * a, integer * lda, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len);
extern doublereal zlantb_(char *norm, char *uplo, char *diag, integer * n,
			  integer * k, doublecomplex * ab, integer * ldab,
			  doublereal * work, ftnlen norm_len, ftnlen uplo_len,
			  ftnlen diag_len);
extern doublereal zlantp_(char *norm, char *uplo, char *diag, integer * n,
			  doublecomplex * ap, doublereal * work,
			  ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern doublereal zlantr_(char *norm, char *uplo, char *diag, integer * m,
			  integer * n, doublecomplex * a, integer * lda,
			  doublereal * work, ftnlen norm_len, ftnlen uplo_len,
			  ftnlen diag_len);
extern int zlapll_(integer * n, doublecomplex * x, integer * incx,
		   doublecomplex * y, integer * incy, doublereal * ssmin);
extern int zlapmt_(logical * forwrd, integer * m, integer * n,
		   doublecomplex * x, integer * ldx, integer * k);
extern int zlaqgb_(integer * m, integer * n, integer * kl, integer * ku,
		   doublecomplex * ab, integer * ldab, doublereal * r__,
		   doublereal * c__, doublereal * rowcnd, doublereal * colcnd,
		   doublereal * amax, char *equed, ftnlen equed_len);
extern int zlaqge_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublereal * r__, doublereal * c__, doublereal * rowcnd,
		   doublereal * colcnd, doublereal * amax, char *equed,
		   ftnlen equed_len);
extern int zlaqhb_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, doublereal * s, doublereal * scond,
		   doublereal * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int zlaqhe_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int zlaqhp_(char *uplo, integer * n, doublecomplex * ap,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int zlaqp2_(integer * m, integer * n, integer * offset,
		   doublecomplex * a, integer * lda, integer * jpvt,
		   doublecomplex * tau, doublereal * vn1, doublereal * vn2,
		   doublecomplex * work);
extern int zlaqps_(integer * m, integer * n, integer * offset, integer * nb,
		   integer * kb, doublecomplex * a, integer * lda,
		   integer * jpvt, doublecomplex * tau, doublereal * vn1,
		   doublereal * vn2, doublecomplex * auxv, doublecomplex * f,
		   integer * ldf);
extern int zlaqsb_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, doublereal * s, doublereal * scond,
		   doublereal * amax, char *equed, ftnlen uplo_len,
		   ftnlen equed_len);
extern int zlaqsp_(char *uplo, integer * n, doublecomplex * ap,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int zlaqsy_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   char *equed, ftnlen uplo_len, ftnlen equed_len);
extern int zlar1v_(integer * n, integer * b1, integer * bn,
		   doublereal * sigma, doublereal * d__, doublereal * l,
		   doublereal * ld, doublereal * lld, doublereal * gersch,
		   doublecomplex * z__, doublereal * ztz, doublereal * mingma,
		   integer * r__, integer * isuppz, doublereal * work);
extern int zlar2v_(integer * n, doublecomplex * x, doublecomplex * y,
		   doublecomplex * z__, integer * incx, doublereal * c__,
		   doublecomplex * s, integer * incc);
extern int zlarcm_(integer * m, integer * n, doublereal * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * c__,
		   integer * ldc, doublereal * rwork);
extern int zlarf_(char *side, integer * m, integer * n, doublecomplex * v,
		  integer * incv, doublecomplex * tau, doublecomplex * c__,
		  integer * ldc, doublecomplex * work, ftnlen side_len);
extern int zlarfb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, doublecomplex * v,
		   integer * ldv, doublecomplex * t, integer * ldt,
		   doublecomplex * c__, integer * ldc, doublecomplex * work,
		   integer * ldwork, ftnlen side_len, ftnlen trans_len,
		   ftnlen direct_len, ftnlen storev_len);
extern int zlarfg_(integer * n, doublecomplex * alpha, doublecomplex * x,
		   integer * incx, doublecomplex * tau);
extern int zlarft_(char *direct, char *storev, integer * n, integer * k,
		   doublecomplex * v, integer * ldv, doublecomplex * tau,
		   doublecomplex * t, integer * ldt, ftnlen direct_len,
		   ftnlen storev_len);
extern int zlarfx_(char *side, integer * m, integer * n, doublecomplex * v,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, ftnlen side_len);
extern int zlargv_(integer * n, doublecomplex * x, integer * incx,
		   doublecomplex * y, integer * incy, doublereal * c__,
		   integer * incc);
extern int zlarnv_(integer * idist, integer * iseed, integer * n,
		   doublecomplex * x);
extern int zlarrv_(integer * n, doublereal * d__, doublereal * l,
		   integer * isplit, integer * m, doublereal * w,
		   integer * iblock, doublereal * gersch, doublereal * tol,
		   doublecomplex * z__, integer * ldz, integer * isuppz,
		   doublereal * work, integer * iwork, integer * info);
extern int zlartg_(doublecomplex * f, doublecomplex * g, doublereal * cs,
		   doublecomplex * sn, doublecomplex * r__);
extern int zlartv_(integer * n, doublecomplex * x, integer * incx,
		   doublecomplex * y, integer * incy, doublereal * c__,
		   doublecomplex * s, integer * incc);
extern int zlarz_(char *side, integer * m, integer * n, integer * l,
		  doublecomplex * v, integer * incv, doublecomplex * tau,
		  doublecomplex * c__, integer * ldc, doublecomplex * work,
		  ftnlen side_len);
extern int zlarzb_(char *side, char *trans, char *direct, char *storev,
		   integer * m, integer * n, integer * k, integer * l,
		   doublecomplex * v, integer * ldv, doublecomplex * t,
		   integer * ldt, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * ldwork, ftnlen side_len,
		   ftnlen trans_len, ftnlen direct_len, ftnlen storev_len);
extern int zlarzt_(char *direct, char *storev, integer * n, integer * k,
		   doublecomplex * v, integer * ldv, doublecomplex * tau,
		   doublecomplex * t, integer * ldt, ftnlen direct_len,
		   ftnlen storev_len);
extern int zlascl_(char *type__, integer * kl, integer * ku,
		   doublereal * cfrom, doublereal * cto, integer * m,
		   integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen type_len);
extern int zlaset_(char *uplo, integer * m, integer * n,
		   doublecomplex * alpha, doublecomplex * beta,
		   doublecomplex * a, integer * lda, ftnlen uplo_len);
extern int zlasr_(char *side, char *pivot, char *direct, integer * m,
		  integer * n, doublereal * c__, doublereal * s,
		  doublecomplex * a, integer * lda, ftnlen side_len,
		  ftnlen pivot_len, ftnlen direct_len);
extern int zlassq_(integer * n, doublecomplex * x, integer * incx,
		   doublereal * scale, doublereal * sumsq);
extern int zlaswp_(integer * n, doublecomplex * a, integer * lda,
		   integer * k1, integer * k2, integer * ipiv,
		   integer * incx);
extern int zlasyf_(char *uplo, integer * n, integer * nb, integer * kb,
		   doublecomplex * a, integer * lda, integer * ipiv,
		   doublecomplex * w, integer * ldw, integer * info,
		   ftnlen uplo_len);
extern int zlatbs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, doublecomplex * x, doublereal * scale,
		   doublereal * cnorm, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len, ftnlen normin_len);
extern int zlatdf_(integer * ijob, integer * n, doublecomplex * z__,
		   integer * ldz, doublecomplex * rhs, doublereal * rdsum,
		   doublereal * rdscal, integer * ipiv, integer * jpiv);
extern int zlatps_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, doublecomplex * ap, doublecomplex * x,
		   doublereal * scale, doublereal * cnorm, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len,
		   ftnlen normin_len);
extern int zlatrd_(char *uplo, integer * n, integer * nb, doublecomplex * a,
		   integer * lda, doublereal * e, doublecomplex * tau,
		   doublecomplex * w, integer * ldw, ftnlen uplo_len);
extern int zlatrs_(char *uplo, char *trans, char *diag, char *normin,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * x, doublereal * scale, doublereal * cnorm,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len, ftnlen normin_len);
extern int zlatrz_(integer * m, integer * n, integer * l, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work);
extern int zlatzm_(char *side, integer * m, integer * n, doublecomplex * v,
		   integer * incv, doublecomplex * tau, doublecomplex * c1,
		   doublecomplex * c2, integer * ldc, doublecomplex * work,
		   ftnlen side_len);
extern int zlauu2_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int zlauum_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int zpbcon_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zpbequ_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, doublereal * s, doublereal * scond,
		   doublereal * amax, integer * info, ftnlen uplo_len);
extern int zpbrfs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   doublecomplex * ab, integer * ldab, doublecomplex * afb,
		   integer * ldafb, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len);
extern int zpbstf_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int zpbsv_(char *uplo, integer * n, integer * kd, integer * nrhs,
		  doublecomplex * ab, integer * ldab, doublecomplex * b,
		  integer * ldb, integer * info, ftnlen uplo_len);
extern int zpbsvx_(char *fact, char *uplo, integer * n, integer * kd,
		   integer * nrhs, doublecomplex * ab, integer * ldab,
		   doublecomplex * afb, integer * ldafb, char *equed,
		   doublereal * s, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int zpbtf2_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int zpbtrf_(char *uplo, integer * n, integer * kd, doublecomplex * ab,
		   integer * ldab, integer * info, ftnlen uplo_len);
extern int zpbtrs_(char *uplo, integer * n, integer * kd, integer * nrhs,
		   doublecomplex * ab, integer * ldab, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zpocon_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zpoequ_(integer * n, doublecomplex * a, integer * lda,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   integer * info);
extern int zporfs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, doublecomplex * af, integer * ldaf,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zposv_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		  integer * lda, doublecomplex * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int zposvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * af,
		   integer * ldaf, char *equed, doublereal * s,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int zpotf2_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int zpotrf_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int zpotri_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * info, ftnlen uplo_len);
extern int zpotrs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int zppcon_(char *uplo, integer * n, doublecomplex * ap,
		   doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zppequ_(char *uplo, integer * n, doublecomplex * ap,
		   doublereal * s, doublereal * scond, doublereal * amax,
		   integer * info, ftnlen uplo_len);
extern int zpprfs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len);
extern int zppsv_(char *uplo, integer * n, integer * nrhs, doublecomplex * ap,
		  doublecomplex * b, integer * ldb, integer * info,
		  ftnlen uplo_len);
extern int zppsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, char *equed,
		   doublereal * s, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * rcond,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len, ftnlen equed_len);
extern int zpptrf_(char *uplo, integer * n, doublecomplex * ap,
		   integer * info, ftnlen uplo_len);
extern int zpptri_(char *uplo, integer * n, doublecomplex * ap,
		   integer * info, ftnlen uplo_len);
extern int zpptrs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int zptcon_(integer * n, doublereal * d__, doublecomplex * e,
		   doublereal * anorm, doublereal * rcond, doublereal * rwork,
		   integer * info);
extern int zpteqr_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublecomplex * z__, integer * ldz, doublereal * work,
		   integer * info, ftnlen compz_len);
extern int zptrfs_(char *uplo, integer * n, integer * nrhs, doublereal * d__,
		   doublecomplex * e, doublereal * df, doublecomplex * ef,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zptsv_(integer * n, integer * nrhs, doublereal * d__,
		  doublecomplex * e, doublecomplex * b, integer * ldb,
		  integer * info);
extern int zptsvx_(char *fact, integer * n, integer * nrhs, doublereal * d__,
		   doublecomplex * e, doublereal * df, doublecomplex * ef,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len);
extern int zpttrf_(integer * n, doublereal * d__, doublecomplex * e,
		   integer * info);
extern int zpttrs_(char *uplo, integer * n, integer * nrhs, doublereal * d__,
		   doublecomplex * e, doublecomplex * b, integer * ldb,
		   integer * info, ftnlen uplo_len);
extern int zptts2_(integer * iuplo, integer * n, integer * nrhs,
		   doublereal * d__, doublecomplex * e, doublecomplex * b,
		   integer * ldb);
extern int zrot_(integer * n, doublecomplex * cx, integer * incx,
		 doublecomplex * cy, integer * incy, doublereal * c__,
		 doublecomplex * s);
extern int zspcon_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, integer * info, ftnlen uplo_len);
extern int zspmv_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * ap, doublecomplex * x, integer * incx,
		  doublecomplex * beta, doublecomplex * y, integer * incy,
		  ftnlen uplo_len);
extern int zspr_(char *uplo, integer * n, doublecomplex * alpha,
		 doublecomplex * x, integer * incx, doublecomplex * ap,
		 ftnlen uplo_len);
extern int zsprfs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, integer * ipiv,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len);
extern int zspsv_(char *uplo, integer * n, integer * nrhs, doublecomplex * ap,
		  integer * ipiv, doublecomplex * b, integer * ldb,
		  integer * info, ftnlen uplo_len);
extern int zspsvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, doublecomplex * afp, integer * ipiv,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * rcond, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen fact_len,
		   ftnlen uplo_len);
extern int zsptrf_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int zsptri_(char *uplo, integer * n, doublecomplex * ap,
		   integer * ipiv, doublecomplex * work, integer * info,
		   ftnlen uplo_len);
extern int zsptrs_(char *uplo, integer * n, integer * nrhs,
		   doublecomplex * ap, integer * ipiv, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int zstedc_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublecomplex * z__, integer * ldz, doublecomplex * work,
		   integer * lwork, doublereal * rwork, integer * lrwork,
		   integer * iwork, integer * liwork, integer * info,
		   ftnlen compz_len);
extern int zstegr_(char *jobz, char *range, integer * n, doublereal * d__,
		   doublereal * e, doublereal * vl, doublereal * vu,
		   integer * il, integer * iu, doublereal * abstol,
		   integer * m, doublereal * w, doublecomplex * z__,
		   integer * ldz, integer * isuppz, doublereal * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info, ftnlen jobz_len, ftnlen range_len);
extern int zstein_(integer * n, doublereal * d__, doublereal * e, integer * m,
		   doublereal * w, integer * iblock, integer * isplit,
		   doublecomplex * z__, integer * ldz, doublereal * work,
		   integer * iwork, integer * ifail, integer * info);
extern int zsteqr_(char *compz, integer * n, doublereal * d__, doublereal * e,
		   doublecomplex * z__, integer * ldz, doublereal * work,
		   integer * info, ftnlen compz_len);
extern int zsycon_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublereal * anorm, doublereal * rcond,
		   doublecomplex * work, integer * info, ftnlen uplo_len);
extern int zsymv_(char *uplo, integer * n, doublecomplex * alpha,
		  doublecomplex * a, integer * lda, doublecomplex * x,
		  integer * incx, doublecomplex * beta, doublecomplex * y,
		  integer * incy, ftnlen uplo_len);
extern int zsyr_(char *uplo, integer * n, doublecomplex * alpha,
		 doublecomplex * x, integer * incx, doublecomplex * a,
		 integer * lda, ftnlen uplo_len);
extern int zsyrfs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, doublecomplex * af, integer * ldaf,
		   integer * ipiv, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len);
extern int zsysv_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		  integer * lda, integer * ipiv, doublecomplex * b,
		  integer * ldb, doublecomplex * work, integer * lwork,
		  integer * info, ftnlen uplo_len);
extern int zsysvx_(char *fact, char *uplo, integer * n, integer * nrhs,
		   doublecomplex * a, integer * lda, doublecomplex * af,
		   integer * ldaf, integer * ipiv, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * rcond, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, integer * lwork, doublereal * rwork,
		   integer * info, ftnlen fact_len, ftnlen uplo_len);
extern int zsytf2_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, integer * info, ftnlen uplo_len);
extern int zsytrf_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublecomplex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int zsytri_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   integer * ipiv, doublecomplex * work, integer * info,
		   ftnlen uplo_len);
extern int zsytrs_(char *uplo, integer * n, integer * nrhs, doublecomplex * a,
		   integer * lda, integer * ipiv, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len);
extern int ztbcon_(char *norm, char *uplo, char *diag, integer * n,
		   integer * kd, doublecomplex * ab, integer * ldab,
		   doublereal * rcond, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen norm_len,
		   ftnlen uplo_len, ftnlen diag_len);
extern int ztbrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, doublecomplex * ab,
		   integer * ldab, doublecomplex * b, integer * ldb,
		   doublecomplex * x, integer * ldx, doublereal * ferr,
		   doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int ztbtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * kd, integer * nrhs, doublecomplex * ab,
		   integer * ldab, doublecomplex * b, integer * ldb,
		   integer * info, ftnlen uplo_len, ftnlen trans_len,
		   ftnlen diag_len);
extern int ztgevc_(char *side, char *howmny, logical * select, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * vl, integer * ldvl,
		   doublecomplex * vr, integer * ldvr, integer * mm,
		   integer * m, doublecomplex * work, doublereal * rwork,
		   integer * info, ftnlen side_len, ftnlen howmny_len);
extern int ztgex2_(logical * wantq, logical * wantz, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * q, integer * ldq,
		   doublecomplex * z__, integer * ldz, integer * j1,
		   integer * info);
extern int ztgexc_(logical * wantq, logical * wantz, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * q, integer * ldq,
		   doublecomplex * z__, integer * ldz, integer * ifst,
		   integer * ilst, integer * info);
extern int ztgsen_(integer * ijob, logical * wantq, logical * wantz,
		   logical * select, integer * n, doublecomplex * a,
		   integer * lda, doublecomplex * b, integer * ldb,
		   doublecomplex * alpha, doublecomplex * beta,
		   doublecomplex * q, integer * ldq, doublecomplex * z__,
		   integer * ldz, integer * m, doublereal * pl,
		   doublereal * pr, doublereal * dif, doublecomplex * work,
		   integer * lwork, integer * iwork, integer * liwork,
		   integer * info);
extern int ztgsja_(char *jobu, char *jobv, char *jobq, integer * m,
		   integer * p, integer * n, integer * k, integer * l,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublereal * tola, doublereal * tolb,
		   doublereal * alpha, doublereal * beta, doublecomplex * u,
		   integer * ldu, doublecomplex * v, integer * ldv,
		   doublecomplex * q, integer * ldq, doublecomplex * work,
		   integer * ncycle, integer * info, ftnlen jobu_len,
		   ftnlen jobv_len, ftnlen jobq_len);
extern int ztgsna_(char *job, char *howmny, logical * select, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * vl, integer * ldvl,
		   doublecomplex * vr, integer * ldvr, doublereal * s,
		   doublereal * dif, integer * mm, integer * m,
		   doublecomplex * work, integer * lwork, integer * iwork,
		   integer * info, ftnlen job_len, ftnlen howmny_len);
extern int ztgsy2_(char *trans, integer * ijob, integer * m, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * c__, integer * ldc,
		   doublecomplex * d__, integer * ldd, doublecomplex * e,
		   integer * lde, doublecomplex * f, integer * ldf,
		   doublereal * scale, doublereal * rdsum,
		   doublereal * rdscal, integer * info, ftnlen trans_len);
extern int ztgsyl_(char *trans, integer * ijob, integer * m, integer * n,
		   doublecomplex * a, integer * lda, doublecomplex * b,
		   integer * ldb, doublecomplex * c__, integer * ldc,
		   doublecomplex * d__, integer * ldd, doublecomplex * e,
		   integer * lde, doublecomplex * f, integer * ldf,
		   doublereal * scale, doublereal * dif, doublecomplex * work,
		   integer * lwork, integer * iwork, integer * info,
		   ftnlen trans_len);
extern int ztpcon_(char *norm, char *uplo, char *diag, integer * n,
		   doublecomplex * ap, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int ztprfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublecomplex * ap, doublecomplex * b,
		   integer * ldb, doublecomplex * x, integer * ldx,
		   doublereal * ferr, doublereal * berr, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int ztptri_(char *uplo, char *diag, integer * n, doublecomplex * ap,
		   integer * info, ftnlen uplo_len, ftnlen diag_len);
extern int ztptrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublecomplex * ap, doublecomplex * b,
		   integer * ldb, integer * info, ftnlen uplo_len,
		   ftnlen trans_len, ftnlen diag_len);
extern int ztrcon_(char *norm, char *uplo, char *diag, integer * n,
		   doublecomplex * a, integer * lda, doublereal * rcond,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen norm_len, ftnlen uplo_len, ftnlen diag_len);
extern int ztrevc_(char *side, char *howmny, logical * select, integer * n,
		   doublecomplex * t, integer * ldt, doublecomplex * vl,
		   integer * ldvl, doublecomplex * vr, integer * ldvr,
		   integer * mm, integer * m, doublecomplex * work,
		   doublereal * rwork, integer * info, ftnlen side_len,
		   ftnlen howmny_len);
extern int ztrexc_(char *compq, integer * n, doublecomplex * t, integer * ldt,
		   doublecomplex * q, integer * ldq, integer * ifst,
		   integer * ilst, integer * info, ftnlen compq_len);
extern int ztrrfs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * x,
		   integer * ldx, doublereal * ferr, doublereal * berr,
		   doublecomplex * work, doublereal * rwork, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ztrsen_(char *job, char *compq, logical * select, integer * n,
		   doublecomplex * t, integer * ldt, doublecomplex * q,
		   integer * ldq, doublecomplex * w, integer * m,
		   doublereal * s, doublereal * sep, doublecomplex * work,
		   integer * lwork, integer * info, ftnlen job_len,
		   ftnlen compq_len);
extern int ztrsna_(char *job, char *howmny, logical * select, integer * n,
		   doublecomplex * t, integer * ldt, doublecomplex * vl,
		   integer * ldvl, doublecomplex * vr, integer * ldvr,
		   doublereal * s, doublereal * sep, integer * mm,
		   integer * m, doublecomplex * work, integer * ldwork,
		   doublereal * rwork, integer * info, ftnlen job_len,
		   ftnlen howmny_len);
extern int ztrsyl_(char *trana, char *tranb, integer * isgn, integer * m,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, doublecomplex * c__,
		   integer * ldc, doublereal * scale, integer * info,
		   ftnlen trana_len, ftnlen tranb_len);
extern int ztrti2_(char *uplo, char *diag, integer * n, doublecomplex * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int ztrtri_(char *uplo, char *diag, integer * n, doublecomplex * a,
		   integer * lda, integer * info, ftnlen uplo_len,
		   ftnlen diag_len);
extern int ztrtrs_(char *uplo, char *trans, char *diag, integer * n,
		   integer * nrhs, doublecomplex * a, integer * lda,
		   doublecomplex * b, integer * ldb, integer * info,
		   ftnlen uplo_len, ftnlen trans_len, ftnlen diag_len);
extern int ztzrqf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, integer * info);
extern int ztzrzf_(integer * m, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info);
extern int zung2l_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * info);
extern int zung2r_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * info);
extern int zungbr_(char *vect, integer * m, integer * n, integer * k,
		   doublecomplex * a, integer * lda, doublecomplex * tau,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen vect_len);
extern int zunghr_(integer * n, integer * ilo, integer * ihi,
		   doublecomplex * a, integer * lda, doublecomplex * tau,
		   doublecomplex * work, integer * lwork, integer * info);
extern int zungl2_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * info);
extern int zunglq_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zungql_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zungqr_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zungr2_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * info);
extern int zungrq_(integer * m, integer * n, integer * k, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * work,
		   integer * lwork, integer * info);
extern int zungtr_(char *uplo, integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * work, integer * lwork,
		   integer * info, ftnlen uplo_len);
extern int zunm2l_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int zunm2r_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int zunmbr_(char *vect, char *side, char *trans, integer * m,
		   integer * n, integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen vect_len, ftnlen side_len, ftnlen trans_len);
extern int zunmhr_(char *side, char *trans, integer * m, integer * n,
		   integer * ilo, integer * ihi, doublecomplex * a,
		   integer * lda, doublecomplex * tau, doublecomplex * c__,
		   integer * ldc, doublecomplex * work, integer * lwork,
		   integer * info, ftnlen side_len, ftnlen trans_len);
extern int zunml2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int zunmlq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int zunmql_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int zunmqr_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int zunmr2_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int zunmr3_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * info, ftnlen side_len,
		   ftnlen trans_len);
extern int zunmrq_(char *side, char *trans, integer * m, integer * n,
		   integer * k, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int zunmrz_(char *side, char *trans, integer * m, integer * n,
		   integer * k, integer * l, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen trans_len);
extern int zunmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, doublecomplex * a, integer * lda,
		   doublecomplex * tau, doublecomplex * c__, integer * ldc,
		   doublecomplex * work, integer * lwork, integer * info,
		   ftnlen side_len, ftnlen uplo_len, ftnlen trans_len);
extern int zupgtr_(char *uplo, integer * n, doublecomplex * ap,
		   doublecomplex * tau, doublecomplex * q, integer * ldq,
		   doublecomplex * work, integer * info, ftnlen uplo_len);
extern int zupmtr_(char *side, char *uplo, char *trans, integer * m,
		   integer * n, doublecomplex * ap, doublecomplex * tau,
		   doublecomplex * c__, integer * ldc, doublecomplex * work,
		   integer * info, ftnlen side_len, ftnlen uplo_len,
		   ftnlen trans_len);

#endif
