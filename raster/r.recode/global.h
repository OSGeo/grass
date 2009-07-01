#include <stdio.h>
#include <grass/raster.h>

extern RASTER_MAP_TYPE in_type;
extern RASTER_MAP_TYPE out_type;
extern struct FPReclass rcl_struct;
extern CELL old_min, old_max;
extern DCELL old_dmin, old_dmax;
extern int in_fd, out_fd, no_mask, align_wind, make_dcell, nrules, rule_size;
extern char *name, *result, *title;
extern char **rules;

/* read_rules.c */
int report_range(void);
int read_rules(FILE *);
int update_type(RASTER_MAP_TYPE *, DCELL);
int update_rules(char *);

/* recode.c */
int do_recode(void);
