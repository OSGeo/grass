/* funcdefs.h */

/*
 * Important:  New function names should have at least 2 lower case letters.
 */

int colappend(), m_extract(), m_replace(), csum(), diag();
int identity(), inverse(), issymmetric(), leontief();
int m_abs(), m_exp(), m_putf(), m_read(), m_log(), m_log10(), m_int();
int m_put(), m_recip(), m_sqrt(), modify(), psa(), rowappend();
int rsum(), quit(), unity();
int m_emult();

double determinant(), element(), m_max(), m_min(), norm(), trace();

static struct			/* functions returning doubles */
{
    char *name;
    double (*func) ();
} d_funcs[] = {
"det", determinant,
	"el", element,
	"max", m_max, "min", m_min, "norm", norm, "tr", trace, "\0", 0};

static struct			/* functions returning ints */
{
    char *name;
    int (*func) ();
} m_funcs[] = {
"abs", m_abs,
	"ac", colappend,
	"ar", rowappend,
	"csum", csum,
	"diag", diag,
	"em", m_emult,
	"exp", m_exp,
	"id", identity,
	"int", m_int,
	"inv", inverse,
	"isym", issymmetric,
	"linv", leontief,
	"log", m_log,
	"log10", m_log10,
	"mod", modify,
	"psa", psa,
	"put", m_put,
	"putf", m_putf,
	"quit", quit,
	"read", m_read,
	"recip", m_recip,
	"rsum", rsum,
	"sqrt", m_sqrt,
	"uv", unity, "xm", m_extract, "rm", m_replace, "\0", 0};
