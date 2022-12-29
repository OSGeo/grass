#include <grass/config.h>
/* #include <limits.h> */
#include <float.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "tinf.h"

int (*is_null) (void *);
void (*set_null_value) (void *, int);
int (*bpe) ();
void *(*get_max) (void *, void *);
void *(*get_min) (void *, void *);
void (*get_row) (int, void *, int);
void *(*get_buf) ();
void (*put_row) (int, void *);
double (*slope) (void *, void *, double);
void (*set_min) (void *);
void (*set_max) (void *);
void (*diff) (void *, void *);
void (*sum) (void *, void *);
void (*quot) (void *, void *);
void (*prod) (void *, void *);

/* To add a new multitype function, use the function below to initialize
 * the function pointer to each of the three typed functions.  The function
 * pointers and the function prototypes are defined in a header file.   
 * The actual functions follow. */

void set_func_pointers(int in_type)
{
    switch (in_type) {
    case CELL_TYPE:
	is_null = is_null_c;
	bpe = bpe_c;
	get_max = get_max_c;
	get_min = get_min_c;
	get_row = get_row_c;
	get_buf = get_buf_c;
	put_row = put_row_c;
	slope = slope_c;
	set_min = set_min_c;
	set_max = set_max_c;
	diff = diff_c;
	sum = sum_c;
	quot = quot_c;
	prod = prod_c;
	set_null_value = set_null_value_c;

	break;

    case FCELL_TYPE:
	is_null = is_null_f;
	bpe = bpe_f;
	get_max = get_max_f;
	get_min = get_min_f;
	get_row = get_row_f;
	get_buf = get_buf_f;
	put_row = put_row_f;
	slope = slope_f;
	set_min = set_min_f;
	set_max = set_max_f;
	diff = diff_f;
	sum = sum_f;
	quot = quot_f;
	prod = prod_f;
	set_null_value = set_null_value_f;

	break;

    case DCELL_TYPE:
	is_null = is_null_d;
	bpe = bpe_d;
	get_max = get_max_d;
	get_min = get_min_d;
	get_row = get_row_d;
	get_buf = get_buf_d;
	put_row = put_row_d;
	slope = slope_d;
	set_min = set_min_d;
	set_max = set_max_d;
	diff = diff_d;
	sum = sum_d;
	quot = quot_d;
	prod = prod_d;
	set_null_value = set_null_value_d;

    }

    return;

}

/* check for null values */
int is_null_c(void *value)
{
    return Rast_is_c_null_value((CELL *) value);
}
int is_null_f(void *value)
{
    return Rast_is_f_null_value((FCELL *) value);
}
int is_null_d(void *value)
{
    return Rast_is_d_null_value((DCELL *) value);
}

/* set null values in buffer */
void set_null_value_c(void *value, int num)
{
    Rast_set_c_null_value((CELL *) value, num);
}
void set_null_value_f(void *value, int num)
{
    Rast_set_f_null_value((FCELL *) value, num);
}
void set_null_value_d(void *value, int num)
{
    Rast_set_d_null_value((DCELL *) value, num);
}

/* return the size of the current type */
int bpe_c()
{
    return sizeof(CELL);
}

int bpe_f()
{
    return sizeof(FCELL);
}

int bpe_d()
{
    return sizeof(DCELL);
}

/* return the pointer that points to the smaller of two value */
void *get_min_c(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(CELL *) v1 < *(CELL *) v2)
	rc = v1;
    return rc;
}

void *get_min_f(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(FCELL *) v1 < *(FCELL *) v2)
	rc = v1;
    return rc;
}

void *get_min_d(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(DCELL *) v1 < *(DCELL *) v2)
	rc = v1;
    return rc;
}

/* return the pointer that points to the larger value */
void *get_max_c(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(CELL *) v1 > *(CELL *) v2)
	rc = v1;
    return rc;
}

void *get_max_f(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(FCELL *) v1 > *(FCELL *) v2)
	rc = v1;
    return rc;
}

void *get_max_d(void *v1, void *v2)
{
    void *rc;

    rc = v2;
    if (*(DCELL *) v1 > *(DCELL *) v2)
	rc = v1;
    return rc;
}

/* Read one line from a raster map */
void get_row_c(int fd, void *row, int n)
{
    Rast_get_c_row(fd, (CELL *) row, n);
}

void get_row_f(int fd, void *row, int n)
{
    Rast_get_f_row(fd, (FCELL *) row, n);
}

void get_row_d(int fd, void *row, int n)
{
    Rast_get_d_row(fd, (DCELL *) row, n);
}

/* Write one row to a raster map */
void put_row_c(int fd, void *row)
{
    Rast_put_c_row(fd, (CELL *) row);
}

void put_row_f(int fd, void *row)
{
    Rast_put_f_row(fd, (FCELL *) row);
}

void put_row_d(int fd, void *row)
{
    Rast_put_d_row(fd, (DCELL *) row);
}

/* Allocate memory for one line of data */
void *get_buf_c(void)
{
    return (void *)Rast_allocate_c_buf();
}

void *get_buf_f(void)
{
    return (void *)Rast_allocate_f_buf();
}

void *get_buf_d(void)
{
    return (void *)Rast_allocate_d_buf();
}

/* initialize memory to a minimum value */
void set_min_c(void *v)
{
    *(CELL *) v = INT_MIN;
}
void set_min_f(void *v)
{
    *(FCELL *) v = FLT_MIN;
}
void set_min_d(void *v)
{
    *(DCELL *) v = DBL_MIN;
}

/* initialize memory to a maximum value */
void set_max_c(void *v)
{
    *(CELL *) v = INT_MAX;
}
void set_max_f(void *v)
{
    *(FCELL *) v = FLT_MAX;
}
void set_max_d(void *v)
{
    *(DCELL *) v = DBL_MAX;
}

/* get the difference between two values, returned in the first pointer */
void diff_c(void *v1, void *v2)
{
    *(CELL *) v1 -= *(CELL *) v2;
}
void diff_f(void *v1, void *v2)
{
    *(FCELL *) v1 -= *(FCELL *) v2;
}
void diff_d(void *v1, void *v2)
{
    *(DCELL *) v1 -= *(DCELL *) v2;
}

/* get the sum of two values, returned in the first pointer */
void sum_c(void *v1, void *v2)
{
    *(CELL *) v1 += *(CELL *) v2;
}
void sum_f(void *v1, void *v2)
{
    *(FCELL *) v1 += *(FCELL *) v2;
}
void sum_d(void *v1, void *v2)
{
    *(DCELL *) v1 += *(DCELL *) v2;
}

/* get the quotient of two values, returned in the first pointer */
void quot_c(void *v1, void *v2)
{
    *(CELL *) v1 /= *(CELL *) v2;
}
void quot_f(void *v1, void *v2)
{
    *(FCELL *) v1 /= *(FCELL *) v2;
}
void quot_d(void *v1, void *v2)
{
    *(DCELL *) v1 /= *(DCELL *) v2;
}

/* get the product of two values, returned in the first pointer */
void prod_c(void *v1, void *v2)
{
    *(CELL *) v1 *= *(CELL *) v2;
}
void prod_f(void *v1, void *v2)
{
    *(FCELL *) v1 *= *(FCELL *) v2;
}
void prod_d(void *v1, void *v2)
{
    *(DCELL *) v1 *= *(DCELL *) v2;
}

/* probably not a function of general interest */
/* calculate the slope between two cells, returned as a double  */
double slope_c(void *line1, void *line2, double cnst)
{
    double rc;
    CELL *pedge;

    rc = -HUGE_VAL;
    pedge = (CELL *) line2;
    if (!Rast_is_c_null_value(pedge)) {
	rc = (*(CELL *) line1 - *pedge) / cnst;
    }
    return rc;
}

double slope_f(void *line1, void *line2, double cnst)
{
    double rc;
    FCELL *pedge;

    rc = -HUGE_VAL;
    pedge = (FCELL *) line2;
    if (!Rast_is_f_null_value(pedge)) {
	rc = (*(FCELL *) line1 - *pedge) / cnst;
    }
    return rc;
}

double slope_d(void *line1, void *line2, double cnst)
{
    double rc;
    DCELL *pedge;

    rc = -HUGE_VAL;
    pedge = (DCELL *) line2;
    if (!Rast_is_d_null_value(pedge)) {
	rc = (*(DCELL *) line1 - *pedge) / cnst;
    }
    return rc;
}

/* read a line and update a three-line buffer */
/* moving forward through a file */
int advance_band3(int fh, struct band3 *bnd)
{
    int rc;
    void *hold;

    hold = bnd->b[0];
    bnd->b[0] = bnd->b[1];
    bnd->b[1] = bnd->b[2];
    bnd->b[2] = hold;
    if (fh == 0)
	rc = 0;
    else
	rc = read(fh, bnd->b[2], bnd->sz);
    return rc;
}

/* read a line and update a three-line buffer */
/* moving backward through a file */
int retreat_band3(int fh, struct band3 *bnd)
{
    int rc;
    void *hold;

    hold = bnd->b[2];
    bnd->b[2] = bnd->b[1];
    bnd->b[1] = bnd->b[0];
    bnd->b[0] = hold;
    if (fh == 0)
	rc = 0;
    else {
	rc = read(fh, bnd->b[0], bnd->sz);
	lseek(fh, (off_t) - 2 * bnd->sz, SEEK_CUR);
    }
    return rc;
}
