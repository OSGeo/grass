#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vector
{
    struct Vector *next;
    double x;
    double y;
    double z;
    int refcnt;
} VECTOR;

extern double nanval;

extern void init_vec(void);
extern void showvec(SYMBOL * sym);
extern void setpnt(SYMBOL * var, SYMBOL * pnt);
extern SYMBOL *mkpnt(double x, double y, double z);
extern SYMBOL *mkpntvar(SYMBOL * var, SYMBOL * pnt);
extern SYMBOL *pntfunc(SYMBOL * func, SYMBOL * arglist);
extern SYMBOL *pntop(int op, SYMBOL * pnt1, SYMBOL * pnt2);
extern SYMBOL *pntapp(SYMBOL * head, SYMBOL * elt);
extern VECTOR *v_copy(VECTOR * p, VECTOR * p1);
extern VECTOR *v_add(VECTOR * p, VECTOR * p1, VECTOR * p2);
extern VECTOR *v_sub(VECTOR * p, VECTOR * p1, VECTOR * p2);
extern VECTOR *v_abs(VECTOR * p, VECTOR * p1);
extern VECTOR *v_neg(VECTOR * p, VECTOR * p1);
extern double v_eq(VECTOR * p1, VECTOR * p2);
extern double v_eq_epsilon(VECTOR * p1, VECTOR * p2, VECTOR * e);
extern VECTOR *v_mul(VECTOR * p, VECTOR * p1, double d);
extern VECTOR *v_div(VECTOR * p, VECTOR * p1, double d);
extern double v_val(VECTOR * p);
extern VECTOR *v_unit(VECTOR * p, VECTOR * p1);
extern double v_dot(VECTOR * p1, VECTOR * p2);
extern VECTOR *v_cross(VECTOR * p, VECTOR * p1, VECTOR * p2);
extern double v_isortho(VECTOR * p1, VECTOR * p2);
extern double v_ispara(VECTOR * p1, VECTOR * p2);
extern double v_isacute(VECTOR * p1, VECTOR * p2);
extern double v_area(VECTOR * p1, VECTOR * p2);

#endif
