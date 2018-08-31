#ifndef GRASS_LADEFSDEFS_H
#define GRASS_LADEFSDEFS_H

/* Matrix routines corresponding to BLAS Level III */

mat_struct *G_matrix_init(int, int, int);
int G_matrix_zero(mat_struct *);
int G_matrix_set(mat_struct *, int, int, int);
mat_struct *G_matrix_copy(const mat_struct *);
mat_struct *G_matrix_add(mat_struct *, mat_struct *);
mat_struct *G_matrix_subtract(mat_struct *, mat_struct *);
mat_struct *G_matrix_scale(mat_struct *, const double);
mat_struct *G__matrix_add(mat_struct *, mat_struct *, const double,
			  const double);
mat_struct *G_matrix_product(mat_struct *, mat_struct *);
mat_struct *G_matrix_transpose(mat_struct *);
int G_matrix_LU_solve(const mat_struct *, mat_struct **, const mat_struct *,
		      mat_type);
mat_struct *G_matrix_inverse(mat_struct *);
void G_matrix_free(mat_struct *);
void G_matrix_print(mat_struct *);
int G_matrix_set_element(mat_struct *, int, int, double);
double G_matrix_get_element(mat_struct *, int, int);


/* Matrix-vector routines corresponding to BLAS Level II */

vec_struct *G_matvect_get_column(mat_struct *, int);
vec_struct *G_matvect_get_row(mat_struct *, int);
vec_struct *G_matvect_product(mat_struct *, vec_struct *, vec_struct *);
int G_matvect_extract_vector(mat_struct *, vtype, int);
int G_matvect_retrieve_matrix(vec_struct *);


/* Vector routines corresponding to BLAS Level I */

vec_struct *G_vector_init(int, int, vtype);
int G_vector_set(vec_struct *, int, int, vtype, int);
double G_vector_norm_euclid(vec_struct *);
double G_vector_norm_maxval(vec_struct *, int);
vec_struct *G_vector_copy(const vec_struct *, int);
vec_struct *G_vector_product (vec_struct *, vec_struct *, vec_struct *);

/* Matrix and vector routines corresponding to ?? */

void G_vector_free(vec_struct *);
vec_struct *G_vector_sub(vec_struct *, vec_struct *, vec_struct *);
double G_vector_norm1(vec_struct *);
int G_matrix_read(FILE *, mat_struct *);
int G_matrix_stdin(mat_struct *);
int G_matrix_eigen_sort(vec_struct *, mat_struct *);
mat_struct *G_matrix_scalar_mul(double, mat_struct *, mat_struct *);
mat_struct *G_matrix_resize(mat_struct *, int, int);

#endif /* GRASS_LADEFS_H */
