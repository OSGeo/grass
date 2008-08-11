#include <stdio.h>
#include <grass/gis.h>

void check_ready(void);
int adjcellhd(struct Cell_head *cellhd);
void rdwr_gridatb(void);


extern struct Cell_head cellhd;
extern FCELL *cell;
extern char *file;
extern char *mapset, *iname;
extern char overwr;
