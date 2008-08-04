#include <stdio.h>
#include <grass/gis.h>

int check_ready(void);
int adjcellhd(struct Cell_head *cellhd);
void rdwr_gridatb(void);


#ifdef MAIN
#	define	GLOBAL
#else
#	define	GLOBAL	extern
#endif

GLOBAL struct Cell_head cellhd;
GLOBAL FCELL *cell;
GLOBAL char *file;
GLOBAL char *mapset, *oname;
GLOBAL char buf[1024];
