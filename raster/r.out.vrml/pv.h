#include <grass/gis.h>

/*  VRML VERSION - only 1.0 currently supported
#define VRML2
*/

/* These were making editing awkward */
/* open square bracket */
#define OSB "["
/* closed square bracket */
#define CSB "]"

/* open curley bracket */
#define OCB "{"
/* closed curley bracket */
#define CCB "}"

typedef int FILEDESC;

/* main.c */
extern int init_coordcnv(double, struct Cell_head *, double, double);
extern int do_coordcnv(double *, int);

/* vrml.c */
extern void vrml_putline(int, FILE *, char *);
extern void vrml_begin(FILE *);
extern void vrml_end(FILE *);

/* put_grid.c */
extern void vrml_put_grid(
	FILE *,
	struct Cell_head *,
	FILEDESC, FILEDESC,
	struct Colors *,
	int, int, int, int);

/* put_view.c */
extern void vrml_put_view(FILE *, struct G_3dview *);

