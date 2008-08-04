#include <stdio.h>
#include <grass/gis.h>
#ifdef MAIN
RASTER_MAP_TYPE in_type;
RASTER_MAP_TYPE out_type;
struct FPReclass rcl_struct;
CELL old_min, old_max;
DCELL old_dmin, old_dmax;
int in_fd, out_fd, no_mask, align_wind, make_dcell, nrules, rule_size;
char *name, *mapset, *result, *title;
char **rules;
#else
extern RASTER_MAP_TYPE in_type;
extern RASTER_MAP_TYPE out_type;
extern struct FPReclass rcl_struct;
extern CELL old_min, old_max;
extern DCELL old_dmin, old_dmax;
extern int in_fd, out_fd, no_mask, align_wind, make_dcell, nrules, rule_size;
extern char *name, *mapset, *result, *title;
extern char **rules;
#endif
/* read_rules.c */
int report_range(void);
int read_rules(FILE *);
int update_type(RASTER_MAP_TYPE *, DCELL);
int update_rules(char *);

/* recode.c */
int do_recode(void);
