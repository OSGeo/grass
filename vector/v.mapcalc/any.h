#ifndef ANY_H
#define ANY_H

typedef struct Any
{
    struct Any *next;
    STYP type;
    void *any;
    int refcnt;
} ANY;

extern void init_any(void);
extern void setany(SYMBOL * var, SYMBOL * any);
extern SYMBOL *mkanyvar(SYMBOL * var, SYMBOL * any);
extern SYMBOL *anyfunc(SYMBOL * func, SYMBOL * arglist);

#endif
