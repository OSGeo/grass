#ifndef NUMBER_H
#define NUMBER_H

extern void init_num(void);
extern void shownum(double d);
extern void setnum(SYMBOL * var, double d);
extern SYMBOL *mknumvar(SYMBOL * var, double d);
extern double numfunc(SYMBOL * func, SYMBOL * arglist);
extern double numop(int op, SYMBOL * opd1, SYMBOL * opd2);
extern SYMBOL *mknum(double d);

#endif
