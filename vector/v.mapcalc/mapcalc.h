#ifndef MAPCALC_H
#define MAPCALC_H

typedef enum Styp		/* Symbol Type */
{
    st_none = 0,
    st_map,
    st_any,
    st_num,
    st_str,
    st_arg,
    st_pnt,
    st_nfunc,
    st_mfunc,
    st_pfunc,
    st_afunc
} STYP;

typedef struct Symbol
{
    struct Symbol *next;
    char *name;
    STYP type;			/* type as seen by the parser */
    STYP itype;			/* when type == st_any or st_arg */
    union
    {
	double d;		/* nummeric value (always double) */
	void *p;		/* pointer value */
    } v;
    char *proto;
    STYP rettype;		/* if function, return type */
} SYMBOL;

extern int parseerror;
extern SYMBOL *symtab;

extern void freesym(const void *elt);
extern SYMBOL *delsym(SYMBOL * head);
extern int cmpsymsym(const void *A, const void *B);
extern int cmpsymname(const void *data, const void *elt);
extern void symcpy(const void *Dst, const void *Src);
extern SYMBOL *getsym(const char *name);
extern SYMBOL *putsym(const char *name);
extern SYMBOL *argapp(SYMBOL * head, SYMBOL * arg);

#endif
