
#ifndef _MAPCALC_H_
#define _MAPCALC_H_

/****************************************************************************/

#include <stdio.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>

#include "expression.h"

/****************************************************************************/

/* mapcalc.l */

extern void initialize_scanner_string(const char *);
extern void initialize_scanner_stream(FILE *);

/* mapcalc.y */

extern expr_list *parse_string(const char *);
extern expr_list *parse_stream(FILE *);

extern void syntax_error(const char *fmt, ...);

/* column_shift.c */

extern void column_shift(void *buf, int res_type, int col);

/* expression.c */

extern int is_var(const char *);
extern char *format_expression(const expression *);

/* evaluate.c */

extern void execute(expr_list *);
extern void describe_maps(FILE *, expr_list *);

/* map.c/map3.c */

extern void setup_region(void);

extern int map_type(const char *name, int mod);
extern int open_map(const char *name, int mod, int row, int col);
extern void setup_maps(void);
extern void get_map_row(int idx, int mod, int depth, int row, int col,
			void *buf, int res_type);
extern void close_maps(void);
extern void list_maps(FILE *, const char *);

extern int check_output_map(const char *name);
extern int open_output_map(const char *name, int res_type);
extern void put_map_row(int fd, void *buf, int res_type);
extern void close_output_map(int fd);
extern void unopen_output_map(int fd);

extern void copy_cats(const char *dst, int idx);
extern void copy_colors(const char *dst, int idx);
extern void copy_history(const char *dst, int idx);
extern void create_history(const char *dst, expression * e);

extern void prepare_region_from_maps_union(expression **, int);
extern void prepare_region_from_maps_intersect(expression **, int);

/****************************************************************************/

#endif /* _MAPCALC_H_ */
