/* matrixdefs.h */

/*
 *   Matrix defs for orthoref.c
 */

/* m_add.c */
int m_add(MATRIX *, MATRIX *, MATRIX *);

/* m_copy.c */
int m_copy(MATRIX *, MATRIX *);

/* m_inverse.c */
int inverse(MATRIX *, MATRIX *);
int isnull(MATRIX *);

/* m_mult.c */
int m_mult(MATRIX *, MATRIX *, MATRIX *);

/* m_transpose.c */
int transpose(MATRIX *, MATRIX *);

/* m_zero.c */
int zero(MATRIX *);
